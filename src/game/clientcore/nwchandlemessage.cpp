#include "k2stdint.h"
#include "strref.h"
#include "nwchandlemessage.h"

#ifdef K2_FIX_ENGINE_BUGS
extern "C" void k2_diag_bad_update_record(unsigned char nTag, unsigned int nRecords,
                                          const unsigned char* aTag, const unsigned char* aType,
                                          const unsigned long long* aId, const unsigned int* aPos,
                                          const unsigned int* aMask, unsigned int nPos,
                                          unsigned int nSize, const unsigned char* pMessage);
#endif

#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "guiingame.h"
#include "nwcjournal.h"
#include "nwcareaofeffectobject.h"
#include "clientaimaster.h"
#include "gameobjectarray.h"
#include "nwccreature.h"
#include "nwssoundobject.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwcambientsound.h"
#include "nwcmappin.h"
#include "exostring.h"
#include "exofile.h"
#include "nwctrigger.h"
#include "nwcitem.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwccreatureappearance.h"
#include "exosound.h"
#include "exoresman.h"
#include "nwsobject.h"
#include "exoinput.h"
#include "nwccreaturestats.h"
#include "nwclass.h"
#include "nwspell.h"

#include <stdio.h>

void UpdateScreen(float fDeltaTime, int a, int b);

void SetClearColor(char* pszColor);

int g_bLogConversations;

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"

extern int g_bLoggedIn;


int enablemodelrequest = 1;

extern int RenderQATriggers;

extern CExoSound* g_pExoSound;

float Yaw(const Vector& orientation);
float Pitch(const Vector& orientation);
#include "nwitem.h"
#include "nwcdoor.h"
#include "nwsdoor.h"
#include "AURORA/aurmodel.h"
#include "nwcplaceable.h"
#include "nwsplaceable.h"
#include "serverexoapp.h"
#include "clientoptions.h"
#include "nwcobject.h"
#include "nwsitemproperty.h"
#include "Vector.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
extern CTlkTable* g_pTlkTable;
#include "swguiexamine.h"
#include "netlayer.h"
#include "nwscreature.h"
#include "aurcamera.h"
#include "nwcprojectile.h"
#include "nwcanimbaseprojectile.h"
#include "nwcspellvisual.h"
#include "nwmessage.h"
#include "CSWVisibilityNode.h"
#include "CExoArrayListImpl.h"
#include "nwrace.h"
#include "swparty.h"
#include "nwcanimbase.h"
#include "nwmessagetypes.h"
#include "feedbackmessage.h"
#include "nwscriptconst.h"
#include "serveranimation.h"
#include "soundprioritygroup.h"
#include "tutorial.h"
#include "handmaidenappearance.h"
#if KOTOR1
#include "serverinfo.h"
#endif
#include "exostringinline.h"

// Resource types (CExoResMan, CExoFile and the directory lister).
enum {
    kResTypeMdl = 2002,
    kResTypeBic = 2015,
    kResTypeTpc = 3007,
};

// CSWVisibilityNode::m_flags.
enum {
    kVisibleSeen     = 1 << 0,
    kVisibleHeard    = 1 << 1,
    kVisibleInactive = 1 << 4,
};

// The armor range of baseitems.2da's itemtype column (CSWBaseItem::m_nItemType).
enum {
    kItemTypeFirstArmor = 31,
    kItemTypeLastArmor  = 38,
};

// The type a feedback or combat-log line is filed under (CGuiInGame's message buffers).
enum {
    kMessageTypeTalk     = 0x1,
    kMessageTypeTell     = 0x2,
    kMessageTypeShout    = 0x4,
    kMessageTypeWhisper  = 0x8,
    kMessageTypeParty    = 0x40,
    kMessageTypeFeedback = 0x80,
    kMessageTypeDebug    = 0x10000000,
};

// CSWSCreature::VOICE_CHAT_* id of the sound set's first attack grunt.
enum { kVoiceChatAttackGrunt1 = 10 };

// appearance.2da row Jedi_White_Female_03, the appearance the "c_dummy" stand-in creatures are given.
enum { kAppearanceDummy = 201 };

// CSWCCreature::SetAppearance's rebuild mask: which parts of the model to rebuild. ALL is every
// part but bit 4.
enum {
    kRebuildBody  = 0x1,
    kRebuildHead  = 0x2,
    kRebuildTail  = 0x4,
    kRebuildWings = 0x8,
    kRebuildAll   = 0xef,
};

// The attack results whose line reports a concealment roll.
enum {
    kAttackResultTargetConcealed    = 16,
    kAttackResultAttackerMissChance = 17,
};

// The longest "skill *verb*" text an object's feedback slot shows before it drops the skill.
enum { kFeedbackInfoMaxLength = 20 };

extern int g_bShowIndicator;


void SwitchWeaponEvent(CAurObject* pObject, char* pEvent, void* pData);

CSWCMessage::CSWCMessage()
{
}

CSWCMessage::~CSWCMessage()
{
}

int CSWCMessage::HandleServerToPlayerDialog(unsigned char nType)
{
    K2_UNUSED(nType);
    return 1;
}

int CSWCMessage::UpdatePlayerDialog(CGuiInGame* pGui)
{
    K2_UNUSED(pGui);
    return 0;
}

int CSWCMessage::UpdatePlayerReplies(CGuiInGame* pGui)
{
    K2_UNUSED(pGui);
    return 0;
}

int CSWCMessage::UpdatePlayerReplyChosen(CGuiInGame* pGui)
{
    K2_UNUSED(pGui);
    return 0;
}

int CSWCMessage::UpdateModels(CGuiInGame* pGui)
{
    K2_UNUSED(pGui);
    return 0;
}

int CSWCMessage::UpdateStuntAnimations(CGuiInGame* pGui)
{
    K2_UNUSED(pGui);
    return 0;
}

// Handles the camera channel, which carries yaw, pitch and distance in radians from +Y, or a camera
// mode change.
int CSWCMessage::HandleServerToPlayerCamera(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_CAMERA_CHANGE_LOCATION) {
    unsigned char nMask = ReadBYTE(8);
    if (MessageReadOverflow(1))
        return 0;

    double fYaw = __builtin_nan("");
    double fDist = __builtin_nan("");
    double fPitch = __builtin_nan("");
    if (nMask & 1)
        fYaw = (ReadFLOAT(1.0f, 32) + -1.5707964f) * 57.295776f;
    if (nMask & 2)
        fDist = ReadFLOAT(1.0f, 32);
    if (nMask & 4)
        fPitch = ReadFLOAT(1.0f, 32);
    if (nMask & 8)
        ReadBYTE(8);
    if (MessageReadOverflow(1))
        return 0;

    CAurCamera* pCamera;
#ifdef K2_FIX_ENGINE_BUGS
    pCamera = 0;
#endif
    if (g_pAppManager->m_pClientExoApp->GetModule() != 0)
        pCamera = g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera;
    if (pCamera == 0)
        return 0;

    char szMessage[64];
    if (nMask & 1) {
        sprintf(szMessage, "yaw %f", fYaw);
        pCamera->BehaviorMessage(szMessage);
    }
    if (nMask & 2) {
        sprintf(szMessage, "dist %f", fDist);
        pCamera->BehaviorMessage(szMessage);
    }
    if (nMask & 4) {
        sprintf(szMessage, "pitch %f", fPitch);
        pCamera->BehaviorMessage(szMessage);
    }
    } else if (nType == MSG_CAMERA_SET_MODE) {
        unsigned char nMode = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions == 0)
            return 0;
        pOptions->SetCameraMode(nMode);
    }
    return 0;
}

// Handles the area channel, which applies a fire-and-forget visual effect, drives weather, or
// performs the area transition.
int CSWCMessage::HandleServerToPlayerArea(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_AREA_VISUAL_EFFECT)
    {
        unsigned short nEffect = ReadWORD(16);
        Vector vPos;
        vPos.x = ReadFLOAT(1.0f, 32);
        vPos.y = ReadFLOAT(1.0f, 32);
        vPos.z = ReadFLOAT(1.0f, 32);

        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule == 0)
            return 0;

        pModule->m_pArea->ApplyFireAndForgetVisualEffect(nEffect, vPos);
    }
    else if (nType == MSG_AREA_WEATHER)
    {
        unsigned char nWeather = ReadBYTE(8);
        int bActive = ReadBOOL();
#if !KOTOR1  // K1 has no intensity byte on the wire
        unsigned char nIntensity = ReadBYTE(8);
#endif

        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule == 0)
            return 0;
        CSWCArea* pArea = pModule->m_pArea;
        if (pArea == 0)
            return 0;

        if (bActive)
        {
            pArea->StartWeather(nWeather);

#if !KOTOR1
            if (nIntensity)
                pArea->m_chanceRain = nIntensity;
#endif
        }
        else
        {
            pArea->StopWeather(nWeather);
        }
    }
    else if (nType == MSG_AREA_CLIENT_AREA)
    {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule == 0)
        {
            ClearReadMessage();
            return 0;
        }
        CSWCArea* pArea = pModule->m_pArea;
        if (pArea == 0)
        {
            ClearReadMessage();
            return 0;
        }
        CAurCamera* pCamera = pModule->m_pCamera;
        if (pCamera == 0)
        {
            ClearReadMessage();
            return 0;
        }

        // Release the old area's game object before the unload touches the array.
        CGameObject* pOldArea;
        g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(pArea->m_objectId, &pOldArea);

        CExoString sLoadScreen;
        unsigned char nBmp;
        CGuiAreaLoadscreen* pLoadscreen;
        nBmp = ReadBYTE(8);
        if (nBmp == 1)
            sLoadScreen = ReadCExoString(32);

        // The inner block ends sEntry before EmptyHitcheckIgnoreList runs.
        {
            CExoString sEntry;

            if (nBmp == 0)
            {
                int nRows = g_pRules->m_p2DArrays->m_pAreaTransition->m_nRows;
#if KOTOR1  // K1 has no row-count guard
                nBmp = static_cast<unsigned char>(rand() % (nRows - 2) + 2);
#else
                if (nRows >= 3)
                    nBmp = static_cast<unsigned char>(rand_wincompatible() % (nRows - 2) + 2);
#endif
            }

            if (nBmp == 1)
            {
                pLoadscreen = g_pAppManager->m_pClientExoApp->m_pInternal->StartLoadBar(
                    2, static_cast<char*>(sLoadScreen.CStr()), 1);
            }
            else
            {
                g_pRules->m_p2DArrays->m_pAreaTransition->GetCExoStringEntry(nBmp, "BMPResRef",
                                                                            &sEntry);
                pLoadscreen = g_pAppManager->m_pClientExoApp->m_pInternal->StartLoadBar(
                    2, static_cast<char*>(sEntry.CStr()), 1);
            }

            // Push one frame so the load screen is on screen before the load stalls.
            g_pAppManager->m_pClientExoApp->GetModule()->Update(1.0e-8f);
            SetClearColor(const_cast<char*>("0 0 0"));
            uint64_t nStart = g_pExoBase->m_pTimers->GetHighResolutionTimer();
            int64_t nDelta = g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTimeDifference();
            K2_UNUSED(nStart);
            K2_UNUSED(nDelta);
        }

        g_pAppManager->m_pClientExoApp->EmptyHitcheckIgnoreList();
        UpdateScreen(0.05f, 0, 0);
        pArea->UnloadArea();

        float fX = ReadFLOAT(1.0f, 32);
        float fY = ReadFLOAT(1.0f, 32);
        float fZ = ReadFLOAT(1.0f, 32);
        float fFacing = ReadFLOAT(1.0f, 32);
        int bUnused = ReadBOOL();
        K2_UNUSED(bUnused);

        if (MessageReadOverflow(1))
        {
            ClearReadMessage();
            return 0;
        }

        if (pArea->LoadArea(pCamera, pLoadscreen, 1) == 0)
        {
            ClearReadMessage();
            return 0;
        }

        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pCreature)
        {
            Vector vPos;
            vPos.x = fX;
            vPos.y = fY;
            vPos.z = fZ;
            pCreature->SetPosition(vPos);
            pCreature->m_desiredPosition = vPos;

            float fSin = __builtin_sinf(fFacing);
            float fCos = __builtin_cosf(fFacing);

            Vector vOrientation;
            vOrientation.x = fCos;
            vOrientation.y = fSin;
            vOrientation.z = 0.0f;
            pCreature->SetOrientation(vOrientation);

            pCreature->SetWayPoints(0, 0);
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerCharacterList(unsigned char nType)
{
    K2_UNUSED(nType);
    return 0;
}

// Handles the chat channel, covering talk, shout, whisper, tell, party, server-tell, AI action
// sound and the localized and strref variants.
int CSWCMessage::HandleServerToPlayerChat(unsigned char nType)
{
    CExoString sText;
    CExoString sBubbleText;
    CResRef    rSound;
    CResRef    rDefaultSound;
    CExoString sSpeakerName;
    CExoString sSavedName;
    STR_RES    strRes;
    CClientExoApp* pApp;
    float fBubbleDuration;
    float fBubbleScale;
    int   bHaveName;
    unsigned long long speakerId;
    CGameObject*       pTargetObj;
    unsigned int nStrRef;
    unsigned long long targetId;
    CGameObject* pSoundObj;
    int nVolume;
    CGameObject*  pSpeaker;
    CSWCCreature* pSpeakerCreature;
    CSWCObject*   pSpeakerObj;
    CSWCPlayer* pPlayer;
    CGuiInGame* pGui;
    unsigned int nChannel;
    int          bShowBubble;
    CSWCModule* pModule;
    CSWCCreature* pPC;
    CSWCPlayer* pLocal;

    pApp = g_pAppManager->m_pClientExoApp;

    fBubbleDuration = 0.0f;
    fBubbleScale = 1.0f;
    bHaveName = 0;

    if (MessageReadOverflow(1))
        return 0;

    pTargetObj = 0;
    pSpeakerObj = 0;

    if (nType == MSG_CHAT_SERVER_TELL)
    {
        // ServerTell carries no speaker, so the invalid-object sentinel stands in.
        speakerId = OBJECT_INVALID;
    }
    else if (nType == MSG_CHAT_SERVER_TELL_STRREF)
    {
        speakerId = ReadOBJECTIDClient();
    }
    else
    {
        speakerId = ReadOBJECTIDClient();
    }

    if (nType == MSG_CHAT_SERVER_TELL_STRREF ||
        (nType >= MSG_CHAT_TALK_STRREF && nType <= MSG_CHAT_WHISPER_STRREF))
    {
        // On the strref channels the text is a dialog.tlk row, not wire text.
        STR_RES sr;
        nStrRef = ReadDWORD(32);
        if (nStrRef != 0xffffffff)
        {
            if (g_pTlkTable->Fetch(nStrRef, sr, CTlkTable::GENDER_MALE))
                sText = sr.text;
        }
        // Discarded comparison; kept only for the read.
        static_cast<void>(sText == "");
    }
    else if (nType == MSG_CHAT_MULTI_LANGUAGE_TALK || nType == MSG_CHAT_MULTI_LANGUAGE_SHOUT)
    {
        // The localized channel, the only one carrying a sound and a target.
        targetId = ReadOBJECTIDClient();
        sText = ReadCExoLocStringClient(rDefaultSound, &fBubbleDuration);

        rSound = ReadCResRef(16);
        if (!rSound.IsValid())
            rSound = rDefaultSound;

        pSoundObj = pApp->GetGameObject(ReadOBJECTIDClient());
        // The sound object doubles as pSpeakerObj until the speaker id resolves, so an off-client
        // speaker is still named, voiced and bubbled.
        if (pSoundObj != 0 && pSoundObj->AsSWCObject() != 0)
        {
            pSpeakerObj =static_cast<CSWCObject*>(pSoundObj->AsSWCObject());
            if (pSpeakerObj->m_pVocalization != 0 && pSpeakerObj->m_vocalizationType != 0)
                pSpeakerObj->m_pVocalization->Stop();
        }

        nVolume = ReadINT(32);
        if (nVolume == 1 && sText == "")
            return 1;

        pTargetObj = pApp->GetGameObject(targetId);
        // Overwritten again after the chain below.
        sBubbleText = sText;
    }
    else
    {
        sText = ReadCExoString(32);
    }

    sBubbleText = sText;

    if (nType == MSG_CHAT_MULTI_LANGUAGE_TALK || nType == MSG_CHAT_MULTI_LANGUAGE_SHOUT)
    {
        // Talk-table token expansion runs twice, for the bubble copy with the gender bit forced on
        // and for the logged copy with it off.
        g_pSWTlkTable->ParseStr(sBubbleText, pTargetObj, 1, 1);
        g_pSWTlkTable->ParseStr(sText, pTargetObj, 1, 0);

        bHaveName = 1;
        fBubbleScale = 0.75f;
    }

    sSpeakerName = "";

    if (nType == MSG_CHAT_SHOUT || nType == MSG_CHAT_TELL || nType == MSG_CHAT_PARTY)
    {
        // These senders ship the speaker's position and both name halves.
        ReadFLOAT(1.0f, 32);
        ReadFLOAT(1.0f, 32);
        ReadFLOAT(1.0f, 32);
        sSpeakerName = ReadCExoLocStringClient();
        sSpeakerName = sSpeakerName + " " + ReadCExoLocStringClient();
        bHaveName = 1;
    }

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    pSpeaker = 0;
    pSpeakerCreature = 0;

    if (nType != MSG_CHAT_SERVER_TELL && nType != MSG_CHAT_SERVER_TELL_STRREF)
    {
        pSpeaker = pApp->GetGameObject(speakerId);
        if (!bHaveName && pSpeaker == 0)
            return 0;
    }

    if (pSpeaker != 0)
    {
        pSpeakerCreature = static_cast<CSWCCreature*>(pSpeaker->AsSWCCreature());
        pSpeakerObj      = static_cast<CSWCObject*>(pSpeaker->AsSWCObject());
        if (!bHaveName && pSpeakerCreature == 0 && pSpeakerObj == 0)
            return 0;
    }

    pPlayer = pApp->GetPlayerByGameObjectID(speakerId);

    if (sSpeakerName == "" && nType != MSG_CHAT_SERVER_TELL && nType != MSG_CHAT_SERVER_TELL_STRREF)
    {
        if (pSpeakerCreature != 0)
        {
#if KOTOR1  // K1 uses first and last name
            sSpeakerName = pSpeakerCreature->m_pStats->GetFullName();
#else
            sSpeakerName = pSpeakerCreature->m_pStats->m_name;
#endif
        }
        else if (pSpeakerObj == 0
                 || !pApp->GetObjectName(pSpeakerObj->m_objectId, sSpeakerName))
        {
            // Generic "someone" fallback for a speaker no name source can identify.
            STR_RES sr;
            g_pTlkTable->Fetch(STRREF_UNKNOWN_SPEAKER, sr, CTlkTable::GENDER_MALE);
            sSpeakerName = sr.text;
        }
    }

    pGui = pApp->GetInGameGui();
    if (pGui == 0)
        return 0;
    if (pGui->m_bInitialized == 0)
        return 0;

    sSavedName = sSpeakerName;
    // Deliberate self-assignment.
    sSpeakerName.operator=(sSpeakerName);

    nChannel    = 0;
    bShowBubble = 1;

    switch (nType)
    {
    case MSG_CHAT_SHOUT:
    case MSG_CHAT_SHOUT_STRREF:
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        sSpeakerName = sSpeakerName + ": " + strRes.text.CStr();
        nChannel = kMessageTypeShout;
        break;

    case MSG_CHAT_WHISPER:
    case MSG_CHAT_WHISPER_STRREF:
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        sSpeakerName = sSpeakerName + ": " + strRes.text.CStr();
        nChannel = kMessageTypeWhisper;
        break;

    case MSG_CHAT_TELL:
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        sSpeakerName = sSpeakerName + ": " + strRes.text.CStr();
        nChannel = kMessageTypeTell;
        break;

    case MSG_CHAT_PARTY:
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        sSpeakerName = sSpeakerName + ": " + strRes.text.CStr();
        nChannel = kMessageTypeParty;
        break;

    // The shout half of the multi-language pair renders, but suppresses the bubble.
    case MSG_CHAT_MULTI_LANGUAGE_SHOUT:
        bShowBubble = 0;
        // fall through
    case MSG_CHAT_TALK:
    case MSG_CHAT_TALK_STRREF:
    case MSG_CHAT_MULTI_LANGUAGE_TALK:
        sSpeakerName = sSpeakerName + ": ";
        nChannel = kMessageTypeTalk;
        break;

    case MSG_CHAT_AI_ACTION_PLAY_SOUND: // no prefix, no channel; handled entirely below
        break;

    case MSG_CHAT_SERVER_TELL: // ServerTell and its strref form render nothing and return success.
    case MSG_CHAT_SERVER_TELL_STRREF:
        return 1;

    default: // 13, 14 and anything outside 1..15
        return 0;
    }

    // Speaker's head position for the camera hint and the vocalization.
    Vector vHead;
    if (pSpeakerObj != 0)
    {
        vHead.x = pSpeakerObj->m_position.x;
        vHead.y = pSpeakerObj->m_position.y;
        vHead.z = pSpeakerObj->m_position.z + 2.0f;
    }
    else
    {
        vHead.x = 0.0f;
        vHead.y = 0.0f;
        vHead.z = 0.0f;
    }

    pModule = pApp->GetModule();
    if (pModule == 0)
        return 0;
    if (pModule->m_pCamera == 0)
        return 0;
    pModule->m_pCamera->MapToScreen(vHead, false);

    if (rSound.IsValid() && pSpeakerObj != 0)
    {
        pSpeakerObj->NewVocalization(rSound, 1);
        if (pSpeakerObj->m_pVocalization != 0)
        {
            pSpeakerObj->m_pVocalization->Set3D(0);
            pSpeakerObj->m_pVocalization->SetPosition(pSpeakerObj->m_position, 0.0f);
            if (pSpeakerCreature != 0 && pApp->GetPlayerCreature() == pSpeakerCreature)
                pSpeakerObj->m_pVocalization->SetPriorityGroup(PRIORITY_GROUP_PLAYER_CHAT);
            else
                pSpeakerObj->m_pVocalization->SetPriorityGroup(PRIORITY_GROUP_NON_PLAYER_CHAT);
            pSpeakerObj->m_pVocalization->Play();
            pSpeakerObj->LipSync(rSound);
        }
    }

    if (pSpeakerObj != 0)
        pGui->m_oidDialogSoundSpeaker = pSpeakerObj->m_objectId;

    if (nType == MSG_CHAT_AI_ACTION_PLAY_SOUND)
    {
        // On the AI sound channel the wire text is a sound resref.
        if (pSpeakerObj != 0)
            g_pExoSound->Play3DOneShotSound(CResRef(sText), pSpeakerObj->m_position, 0.0f,
                                            PRIORITY_GROUP_SCRIPTED_PLAY_SOUND, 0, 0, 0.0f, 0.0f);
        else
            g_pExoSound->PlayOneShotSound(CResRef(sText), PRIORITY_GROUP_SCRIPTED_PLAY_SOUND, 0, 0, 0.0f, 0.0f);
        return 1;
    }

    if (g_bLogConversations)
    {
        if (pPlayer != 0)
        {
            if (pSpeakerCreature != 0)
            {
                pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
            else if (pSpeakerObj != 0)
            {
                if (pSpeakerObj->GetPortrait(1).GetResRef()[0] != 0)
                    pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
                else
                    pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
            else
            {
                pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
        }
        else
        {
            if (pSpeakerCreature != 0)
            {
                pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
            else if (pSpeakerObj != 0)
            {
                if (pSpeakerObj->GetPortrait(1).GetResRef()[0] != 0)
                    pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
                else
                    pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
            else
            {
                pGui->AppendToMsgBuffer(sSpeakerName + sText, nChannel, 0);
            }
        }
    }

    if (!bShowBubble)
        return 1;

    pPC = pApp->GetPlayerCreature();
    if (pPlayer != 0 && pPC != 0)
    {
        pLocal = pApp->GetPlayerByGameObjectID(pPC->m_objectId);
        sSpeakerName = "";
        if (pLocal == 0 || pPlayer->m_nPlayerId != pLocal->m_nPlayerId)
        {
            // The formatted player id is vestigial and never read.
            char szPlayerId[16];
            sprintf(szPlayerId, "%u", pPlayer->m_nPlayerId);
            sSpeakerName = "[";
            sSpeakerName = sSpeakerName + sSavedName;
            sSpeakerName = sSpeakerName + "]: ";
        }
    }

    if (pSpeakerObj == 0)
        return 1;

    // Only triggers, placeables and doors get a bracketed speaker prefix on the bubble.
    if (pSpeakerObj->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER
        || pSpeakerObj->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE
        || pSpeakerObj->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
    {
        sSpeakerName = CExoString("[") + sSavedName + "]: ";
    }
    else
    {
        sSpeakerName = "";
    }

    pSpeakerObj->SetPopUpBubbleText(sSpeakerName + sBubbleText, 1, fBubbleScale, fBubbleDuration);
    pSpeakerObj->m_bSpokeChat = 1;
    return 1;
}

int CSWCMessage::HandleServerToPlayerQuickChat(unsigned char nType)
{
    K2_UNUSED(nType);
    return 1;
}

int CSWCMessage::HandleServerToPlayerPlayModuleCharacterList(unsigned char nType)
{
    K2_UNUSED(nType);
    return 1;
}

int CSWCMessage::HandleServerToPlayerDebugInfo_Area(unsigned long long objId)
{
    K2_UNUSED(objId);
    return 1;
}

int CSWCMessage::HandleServerToPlayerExamineGui_CreatureData()
{
    return 0;
}

int CSWCMessage::HandleServerToPlayerExamineGui_ItemData()
{
    return 1;
}

int CSWCMessage::HandleServerToPlayerExamineGui_PlaceableData()
{
    return 0;
}

int CSWCMessage::HandleServerToPlayerExamineGui_MineData()
{
    return 0;
}

int CSWCMessage::HandleServerToPlayerExamineGui_DoorData()
{
    return 0;
}

void CSWCMessage::HandleInventoryFindSourceAndUnghost(CSWCItem* pItem)
{
    K2_UNUSED(pItem);
}

void CSWCMessage::HandleInventoryUnghostSourceItem(CSWCItem* pItem)
{
    K2_UNUSED(pItem);
}

int CSWCMessage::HandleServerToPlayerGuiInfoPopupMessage(unsigned char nType)
{
    return nType == MSG_GUI_INFO_POPUP_ITEM;
}

// Projectile safing channel.
int CSWCMessage::HandleServerToPlayerSafeProjectile(unsigned char nType)
{
    switch (nType)
    {
    case MSG_SAFE_PROJECTILE_SPELL: return HandleServerToPlayerSafeProjectileSpell();
    case MSG_SAFE_PROJECTILE_PROJECTILE: return HandleServerToPlayerSafeProjectileProjectile();
    }
    return 0;
}

int CSWCMessage::HandleServerToPlayerMapPin(unsigned char nType)
{
    switch (nType)
    {
    case MSG_MAP_PIN_ENABLED: return HandleServerToPlayerMapPinEnabled();
    case MSG_MAP_PIN_ADDED: return HandleServerToPlayerMapPinReferenceNumber();
    }
    return 0;
}

int CSWCMessage::HandleServerToPlayerAmbient(unsigned char nType)
{
    switch (nType)
    {
    case MSG_AMBIENT_MUSIC_PLAY: return HandleServerToPlayerAmbientMusicPlay();
    case MSG_AMBIENT_MUSIC_SET_DELAY: return HandleServerToPlayerAmbientMusicSetDelay();
    case MSG_AMBIENT_MUSIC_CHANGE_TRACK: return HandleServerToPlayerAmbientMusicChangeTrack();
    case MSG_AMBIENT_BATTLE_MUSIC_PLAY: return HandleServerToPlayerAmbientBattleMusicPlay();
    case MSG_AMBIENT_BATTLE_MUSIC_CHANGE: return HandleServerToPlayerAmbientBattleMusicChange();
    case MSG_AMBIENT_SOUND_LOOP_PLAY: return HandleServerToPlayerAmbientSoundLoopPlay();
    case MSG_AMBIENT_SOUND_LOOP_CHANGE: return HandleServerToPlayerAmbientSoundLoopChange();
    case MSG_AMBIENT_SOUND_VOLUME: return HandleServerToPlayerAmbientSoundVolume();
    }
    return 0;
}

// Builds an item and resolves its icon and model resrefs, falling back to the base item's
// defaults when the variation's own resref does not exist. The diagnostics are never logged.

CSWCItem* CSWCMessage::CreateItem(unsigned int nBaseItem,
                                  unsigned char nModelVariation,
                                  char nTextureVariation)
{
    CSWCItem* pItem = new CSWCItem(nBaseItem);

    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(nBaseItem);
    if (!pBaseItem)
        return 0;

    CResRef resRef;

    resRef = pBaseItem->GetIconResRef(nModelVariation, nTextureVariation);
    if (!g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0))
        && !g_pExoResMan->Exists(resRef, kResTypeTpc, static_cast<unsigned int*>(0)))
    {
        resRef = CResRef(pBaseItem->m_defaultIconResRef);
        if (!g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0))
            || !g_pExoResMan->Exists(resRef, kResTypeTpc, static_cast<unsigned int*>(0)))
        {
            CExoString sMessage;
            CExoString sWanted;
            CExoString sDefault;

            pBaseItem->GetIconResRef(nModelVariation, nTextureVariation).CopyToString(sWanted);
            resRef.CopyToString(sDefault);
            sMessage.Format(const_cast<char*>("Icon %s nor the default icon %s could be loaded."),
                            sWanted.CStr(), sDefault.CStr());
        }
    }
    pItem->SetIcon(resRef);

    pItem->m_nModelVariation = nModelVariation;
    pItem->m_nTextureVariation = nTextureVariation;

    resRef = pBaseItem->GetModelResRef(nModelVariation, nTextureVariation);
    if (!g_pExoResMan->Exists(resRef, kResTypeMdl, static_cast<unsigned int*>(0)))
    {
        resRef = CResRef(pBaseItem->m_defaultModelResRef);
        if (!g_pExoResMan->Exists(resRef, kResTypeMdl, static_cast<unsigned int*>(0)))
        {
            CExoString sMessage;
            CExoString sWanted;
            CExoString sDefault;

            pBaseItem->GetModelResRef(nModelVariation, nTextureVariation).CopyToString(sWanted);
            resRef.CopyToString(sDefault);
            sMessage.Format(const_cast<char*>("Model %s nor the default model %s could be loaded."),
                            sWanted.CStr(), sDefault.CStr());
        }
    }
    pItem->SetModelResRef(resRef);

    return pItem;
}

// The object id arrives as a parameter, so there is nothing to guard; nSize is unused.

int CSWCMessage::HandleServerToPlayerAreaOfEffectUpdate_Update(unsigned long long objId,
                                                               unsigned int nSize)
{
    K2_UNUSED(nSize);
    return g_pAppManager->m_pClientExoApp->GetAreaOfEffectByGameObjectID(objId) != 0;
}

// General object update. The mask gates a read at the top and the matching apply at the bottom:
// bit 0 position, bit 1 orientation, bit 2 animation speed and appearance, bit 3 looping visual
// effect records (applied last), bit 5 portrait. The vectors start zeroed, so an object whose
// mask omits position or orientation is moved to the origin, and a door's appearance ids map to
// door states before the animation is set.

int CSWCMessage::HandleServerToPlayerGenericObjectUpdate_Update(unsigned long long objId,
                                                                unsigned int nMask)
{
    CResRef rPortrait;
    Vector  vPosition;
    Vector  vOrientation;

    vPosition.x = 0.0f;
    vPosition.y = 0.0f;
    vPosition.z = 0.0f;
    vOrientation.x = 0.0f;
    vOrientation.y = 0.0f;
    vOrientation.z = 0.0f;

    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(objId);

    unsigned short nPortraitId K2_UNSET;
    unsigned short nAppearance K2_UNSET;
    float          fAnimationSpeed K2_UNSET;

    if (nMask & MSG_OBJECT_UPDATE_POSITION)
    {
        vPosition.x = ReadFLOAT(1.0f, 32);
        vPosition.y = ReadFLOAT(1.0f, 32);
        vPosition.z = ReadFLOAT(1.0f, 32);
    }

    if (nMask & MSG_OBJECT_UPDATE_ORIENTATION)
    {
        vOrientation.x = ReadFLOAT(1.0f, 32);
        vOrientation.y = ReadFLOAT(1.0f, 32);
        vOrientation.z = ReadFLOAT(1.0f, 32);
    }

    if (nMask & MSG_OBJECT_UPDATE_PORTRAIT)
    {
        nPortraitId = ReadWORD(16);
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            rPortrait = ReadCResRef(16);
    }

    if (nMask & MSG_OBJECT_UPDATE_ANIMATION)
    {
        fAnimationSpeed = ReadFLOAT(1.0f, 32);
        nAppearance = ReadWORD(16);
    }

    if (MessageReadOverflow(1))
        return 0;

    CSWCObject* pClientObject = 0;

    if (pObject)
    {
        if (pObject->AsSWCObject())
        {
            pClientObject = static_cast<CSWCObject*>(pObject->AsSWCObject());

            if (nMask & MSG_OBJECT_UPDATE_POSITION)
                pClientObject->SetPosition(vPosition);

            if (nMask & MSG_OBJECT_UPDATE_ORIENTATION)
                pClientObject->SetOrientation(vOrientation);

            if (nMask & MSG_OBJECT_UPDATE_PORTRAIT)
            {
                pClientObject->SetPortraitId(nPortraitId);
                if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
                    pClientObject->SetPortrait(rPortrait);
            }

            if (nMask & MSG_OBJECT_UPDATE_ANIMATION)
            {
                pClientObject->m_animationSpeed = fAnimationSpeed;

                if (pClientObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                {
                    if (nAppearance == SERVER_ANIMATION_CLOSED)
                        static_cast<CSWCDoor*>(pClientObject->AsSWCDoor())->SetState(0);
                    else if (nAppearance == SERVER_ANIMATION_DOOR_OPEN1)
                        static_cast<CSWCDoor*>(pClientObject->AsSWCDoor())->SetState(1);
                    else if (nAppearance == SERVER_ANIMATION_DOOR_OPEN2)
                        static_cast<CSWCDoor*>(pClientObject->AsSWCDoor())->SetState(2);
                }

                pClientObject->SetAnimation(nAppearance, 0, 0);
            }
        }
    }

    if (nMask & MSG_OBJECT_UPDATE_VISUAL_EFFECTS)
    {
        if (!HandleServerToPlayerUpdateVisualEffects(pClientObject))
            return 0;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerCreatureUpdate_Add()
{
    Vector vOrientation;

    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

    unsigned long long objId = ReadOBJECTIDClient();
    unsigned char nInParty = ReadBYTE(8);

    float fPositionX = ReadFLOAT(1.0f, 32);
    float fPositionY = ReadFLOAT(1.0f, 32);
    float fPositionZ = ReadFLOAT(1.0f, 32);

    vOrientation.x = ReadFLOAT(1.0f, 32);
    vOrientation.y = ReadFLOAT(1.0f, 32);
    vOrientation.z = ReadFLOAT(1.0f, 32);

    unsigned short nTail = ReadWORD(16);

    if (MessageReadOverflow(1))
        return 0;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    int bAlreadyPresent;

    if (pCreature)
    {
        pCreature->m_bPendingRemoval = 0;

        if (pPlayer)
            pPlayer->UpdateRelativePerception(objId);

        pCreature->SetDesiredFadeState(
            pCreature->m_pArea->GetObjectsAtPositionVisible(pCreature->m_position) == 1, 0, 0, 0);

        pCreature->m_bSnapPositionOnUpdate = 1;
        bAlreadyPresent = 1;
    }
    else
    {
        bAlreadyPresent = 0;
    }

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    if (!bAlreadyPresent)
    {
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);

        if (!pCreature)
        {
            pCreature = new CSWCCreature();

            pCreature->SetId(objId);
            pCreature->SetArea(pArea);
            pCreature->SetInParty(nInParty == 1);

            Vector vPosition;
            vPosition.x = fPositionX;
            vPosition.y = fPositionY;
            vPosition.z = fPositionZ;

            pCreature->SetPosition(vPosition);
            pCreature->SetOrientation(vOrientation);

            pCreature->SetDesiredWalkOrientation(vOrientation);
            pCreature->SetDesiredStationaryOrientation(vOrientation);
            pCreature->m_desiredPosition = vPosition;

            if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddExternalObject(
                    objId, static_cast<CGameObject*>(pCreature), 1) != CGameObjectArray::SUCCESS)
                return 0;

            g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
                pCreature);
        }

        if (pPlayer)
            pPlayer->UpdateRelativePerception(objId);

        pCreature->SetFadeState(1);
        pCreature->SetDesiredFadeState(1, 0, 0, 0);
    }

    pCreature->AttachToScene(pArea->m_pScene);
    pCreature->m_nInitialAnimation = nTail;
    pCreature->m_bPendingRemoval = 0;

    return 1;
}

int CSWCMessage::HandleServerToPlayerExportReply()
{
    unsigned int nSize;
    void* pData;
    CExoFile* pFile;
    CExoString sPath;
    STR_RES    strRes;
    CExoString sUnused2;
    CExoString sFirstName;
    CExoString sLastName;

    sFirstName = ReadCExoLocStringClient();
    sLastName = ReadCExoLocStringClient();

    nSize = ReadDWORD(32);
    pData = ReadVOIDPtr(nSize);

    if (MessageReadOverflow(1) || MessageReadUnderflow(1))
    {
        g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_CHARACTER_EXPORT_FAILED, 0, CExoString(""));
    }
    else
    {
        sPath = "LOCALVAULT:";
        sPath = sPath
                + g_pAppManager->m_pClientExoApp->GetPlayerCreature()
                      ->GetSaveCreatureFilename(sFirstName, sLastName);

        pFile = new CExoFile(sPath, kResTypeBic, CExoString("wb"));
        pFile->Write(pData, nSize, 1);
        g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_CHARACTER_EXPORTED, 0, CExoString(""));
        delete pFile;
    }

    if (g_pAppManager->m_pClientExoApp->m_pInternal->m_nExportRequestState == 1)
        g_pAppManager->m_pClientExoApp->m_pInternal->m_nExportRequestState = 0;

    return 1;
}

int CSWCMessage::HandleServerToPlayerAreaOfEffectUpdate_Add()
{
    unsigned long long objId = ReadOBJECTIDClient();
    int nEffectId = ReadINT(32);
    unsigned long long nCreatorId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    CSWCAreaOfEffectObject* pAoe = new CSWCAreaOfEffectObject();
    if (!pAoe->LoadEffect(nEffectId))
        return 0;

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    pAoe->SetId(objId);
    pAoe->SetArea(pArea);
    pAoe->m_nCreatorId = nCreatorId;
    pAoe->m_pScene = pArea->m_pScene;

    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddExternalObject(
            objId, static_cast<CGameObject*>(pAoe), 1) != CGameObjectArray::SUCCESS)
        return 0;

    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(pAoe);
    return 1;
}

int CSWCMessage::HandleServerToPlayerAreaOfEffectUpdate_Delete()
{
    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;

    CSWCAreaOfEffectObject* pAoe = g_pAppManager->m_pClientExoApp->GetAreaOfEffectByGameObjectID(objId);
    if (pAoe)
        pAoe->StopEffect();

    return 1;
}

int CSWCMessage::HandleServerToPlayerModule_StartStartNewModule()
{
    g_pAppManager->m_pClientExoApp->SetInStartNewModule(1);
    return 1;
}

int CSWCMessage::HandleServerToPlayerModule_EndStartNewModule()
{
    int bLoadCharacter = ReadBOOL();

    g_pAppManager->m_pClientExoApp->SetInStartNewModule(0);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    if (bLoadCharacter == 1)
        SendPlayerToServerLogin_LoginIFOCharacter();

    return 1;
}

// Drains the unused message string.

int CSWCMessage::HandleServerToPlayerEndGame()
{
    ReadCExoString(32);
    return 1;
}

int CSWCMessage::HandleServerToPlayerCheat_DoNasty()
{
    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;

    CGameObject* pObject;
    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(objId, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject->AsSWCCreature() != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObject->AsSWCCreature());

        delete pCreature->m_pNastyVisualGob;
        pCreature->m_pNastyVisualGob =
            static_cast<Gob*>(NewCAurObject(const_cast<char*>("VWP_L_Blood_nast"), const_cast<char*>("blood"), 0, 0));
        if (pCreature->m_pNastyVisualGob == 0)
            return 0;

        pCreature->PlaySoundSetSound(kVoiceChatAttackGrunt1, 1);

        Quaternion qOrient(1.0f, 0.0f, 0.0f, 0.0f);
        Vector vPos = { 0.0f, 0.0f, 0.0f };
        pCreature->m_pNastyVisualGob->GetPartLocalPosition(const_cast<char*>("root"), &vPos, &qOrient);
        pCreature->m_pNastyVisualGob->SetPartLocalPosition(
            const_cast<char*>("root"), vPos, qOrient * Quaternion(Vector{ 1.0f, 0.0f, 0.0f }, -1.5707964f));
        pCreature->m_pNastyVisualGob->GetPartPosition(const_cast<char*>("root"), &vPos, &qOrient);

        vPos.x += pCreature->m_position.x;
        vPos.y += pCreature->m_position.y;
        vPos.z = vPos.z + 0.4f + pCreature->m_position.z;
        pCreature->m_pNastyVisualGob->SetPosition(vPos);

        pCreature->m_pNastyVisualGob->SetOrientation(
            YawPitchRoll(Yaw(pCreature->m_orientation * -1.0f),
                         Pitch(pCreature->m_orientation * -1.0f),
                         0.0f));

        pCreature->m_pNastyVisualGob->AttachToScene(pCreature->m_pArea->m_pScene);
        pCreature->m_pNastyVisualGob->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
    }
    return 1;
}

int CSWCMessage::HandleServerToPlayerCheat_PonyRide()
{
    ReadOBJECTIDClient();
    ReadBOOL();
    return MessageReadOverflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerCheat_DebugMode()
{
    int bDebugMode = ReadBOOL();
    if (MessageReadOverflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->SetDebugMode(bDebugMode);
    return 1;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_DeleteWorld()
{
    CSWCWorldJournal* pJournal = g_pAppManager->m_pClientExoApp->GetWorldJournal();

    if (MessageReadOverflow(1))
        return 0;

    pJournal->DeleteEntry(ReadINT(32));
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_DeleteWorldStrref()
{
    CSWCWorldJournal* pJournal = g_pAppManager->m_pClientExoApp->GetWorldJournal();

    if (MessageReadOverflow(1))
        return 0;

    pJournal->DeleteEntryStrRef(ReadDWORD(32));
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_DeleteWorldAll()
{
    CSWCWorldJournal* pJournal = g_pAppManager->m_pClientExoApp->GetWorldJournal();

    if (MessageReadOverflow(1))
        return 0;

    ReadBYTE(8);
    pJournal->DeleteAllEntries();
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage(unsigned char nType)
{
    switch (nType)
    {
    case MSG_JOURNAL_ADD_WORLD:
        return HandleServerToPlayerJournalMessage_AddWorld();
    case MSG_JOURNAL_ADD_WORLD_STRREF:
        return HandleServerToPlayerJournalMessage_AddWorldStrref();
    case MSG_JOURNAL_DELETE_WORLD:
        return HandleServerToPlayerJournalMessage_DeleteWorld();
    case MSG_JOURNAL_DELETE_WORLD_STRREF:
        return HandleServerToPlayerJournalMessage_DeleteWorldStrref();
    case MSG_JOURNAL_DELETE_WORLD_ALL:
        return HandleServerToPlayerJournalMessage_DeleteWorldAll();
    case MSG_JOURNAL_ADD_QUEST:
        return HandleServerToPlayerJournalMessage_AddQuest();
    case MSG_JOURNAL_REMOVE_QUEST:
        return HandleServerToPlayerJournalMessage_RemoveQuest();
    case MSG_JOURNAL_SET_QUEST_PICTURE:
        return HandleServerToPlayerJournalMessage_SetQuestPicture();
    case MSG_JOURNAL_FULL_UPDATE:
        return HandleServerToPlayerJournalMessage_FullQuestUpdate();
    case MSG_JOURNAL_UPDATED:
        return HandleServerToPlayerJournalMessage_Updated();
    }

    return 0;
}

// EFFECTIVE (K1): ours tail-duplicates the shared `return 0` block into the first overflow check; K1 keeps
// one copy (armv7 -O3 exact, same calls and returns on every path).
int CSWCMessage::HandleServerToPlayerLevelUpMessage(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_LEVEL_UP_CONFIRMED)
    {
        CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        if (pGui)
            pGui->SetLevelUpMode(0);
    }

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

// GetInGameGui() is called for its side effect of forcing the character sheet into existence.
// EFFECTIVE (K1): ours tail-duplicates the shared `return 0` block into the first overflow check; K1 keeps
// one copy (armv7 -O3 exact, same calls and returns on every path).
int CSWCMessage::HandleServerToPlayerGUICharacterSheet(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_GUI_CHARACTER_SHEET_NOT_PERMITTED)
    {
        ReadOBJECTIDClient();
        if (MessageReadOverflow(1))
            return 0;

        g_pAppManager->m_pClientExoApp->GetInGameGui();
    }

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

void CSWCMessage::HandleSaveLoadErrorPanelExit(int nButton)
{
    K2_UNUSED(nButton);
    g_pAppManager->m_pClientExoApp->ShutDownToMainMenu();
}

int CSWCMessage::HandleServerToPlayerUpdate_Appearance()
{
    unsigned char nObjectType = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    if (CGameObject::OBJECT_TYPE_CREATURE != nObjectType)
        return 0;

    return HandleServerToPlayerCreatureUpdate_Appearance();
}

// The ambient-sound handlers set music, battle music, ambient loops and their volumes, all applied
// to the area's ambient sound object.

int CSWCMessage::HandleServerToPlayerAmbientMusicPlay()
{
    if (MessageReadOverflow(1))
        return 0;

    int bPlay = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->PlayMusic(bPlay);
    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientMusicSetDelay()
{
    if (MessageReadOverflow(1))
        return 0;

    int nDelay = ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->SetMusicDelay(nDelay);
    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientBattleMusicPlay()
{
    if (MessageReadOverflow(1))
        return 0;

    int bPlay = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->PlayBattleMusic(bPlay);
    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientBattleMusicChange()
{
    if (MessageReadOverflow(1))
        return 0;

    int nTrack = ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->SetBattleMusicTrack(nTrack);
    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientSoundLoopPlay()
{
    if (MessageReadOverflow(1))
        return 0;

    int bPlay = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound->PlayAmbientSound(bPlay);
    return 1;
}

// The day track ignores the fade flag; only the night track uses it.
int CSWCMessage::HandleServerToPlayerAmbientMusicChangeTrack()
{
    if (MessageReadOverflow(1))
        return 0;

#if KOTOR1  // K1 has no loop flag on the wire
    int bDay = ReadBOOL();
    int nTrack = ReadINT(32);
#else
    int bDay = ReadBOOL();
    int nTrack = ReadINT(32);
    int bFadeIn = ReadBOOL();
#endif

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCAmbientSound* pAmbient = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound;
#if KOTOR1
    if (bDay)
        pAmbient->SetMusicDayTrack(nTrack);
    else
        pAmbient->SetMusicNightTrack(nTrack);
#else
    if (bDay)
        pAmbient->SetMusicDayTrack(nTrack, 0);
    else
        pAmbient->SetMusicNightTrack(nTrack, bFadeIn);
#endif

    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientSoundLoopChange()
{
    if (MessageReadOverflow(1))
        return 0;

    int bDay = ReadBOOL();
    int nTrack = ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCAmbientSound* pAmbient = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound;
    if (bDay)
        pAmbient->SetAmbientDayTrack(nTrack);
    else
        pAmbient->SetAmbientNightTrack(nTrack);

    return 1;
}

int CSWCMessage::HandleServerToPlayerAmbientSoundVolume()
{
    if (MessageReadOverflow(1))
        return 0;

    int bDay = ReadBOOL();
    int nVolume = ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCAmbientSound* pAmbient = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound;
    if (bDay)
        pAmbient->SetAmbientDayVolume(nVolume);
    else
        pAmbient->SetAmbientNightVolume(nVolume);

    return 1;
}

int CSWCMessage::HandleServerToPlayerCreatureUpdate_StripEquippedItems()
{
    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    if (!pCreature)
        return 0;
    pCreature->StripEquippedItems();
    return 1;
}

// Login channel. The two CExoString locals are leftovers from a login that once carried a name
// and a password.

int CSWCMessage::HandleServerToPlayerLogin(unsigned char nType)
{
    CExoString sName;
    CExoString sPassword;
    unsigned char nCount;

    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
    case MSG_LOGIN_CONFIRM:
        if (MessageReadUnderflow(1))
            return 0;

        g_bLoggedIn = 1;
        return 1;

    case 6:
        return MessageReadUnderflow(1) == 0;

    case MSG_LOGIN_CHARACTER_QUERY:
    {
        nCount = ReadBYTE(8);

        for (unsigned char i = 0; i < nCount; i++)
        {
            ReadINT(32);
            ReadBYTE(8);
        }

        ReadDWORD(32);
        return 1;
    }

    case MSG_LOGIN_NEED_CHARACTER:
        g_pAppManager->m_pClientExoApp->EndLoadBar();
        return 1;

    case MSG_LOGIN_FAIL:
        ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;

        return MessageReadUnderflow(1) == 0;
    }

    return 0;
}

// Advances the load bar. The server ships {steps done, steps total}. The current phase is weighted
// by the fraction done and the four complete phases are added whole; a zero total is clamped to 1
// before the divide. The resref copy into sModule exists only for its side effect.
int CSWCMessage::HandleServerToPlayerUpdate_WorkRemaining()
{
    unsigned char nDone = ReadBYTE(8);
    unsigned char nTotal = ReadBYTE(8);

    if (nTotal == 0)
        nTotal = 1;

    if (MessageReadOverflow(1))
        return 0;

    CSWCModule*    pModule = g_pAppManager->m_pClientExoApp->GetModule();
    CClientExoApp* pApp    = g_pAppManager->m_pClientExoApp;

    unsigned char nStep = pApp->GetLoadStep(4);

    float fProgress = static_cast<float>(nDone) / static_cast<float>(nTotal) * static_cast<float>(nStep);
    fProgress += static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(3));
    fProgress += static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(2));
    fProgress += static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(1));
    fProgress += static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(0));

    pApp->SetLoadBarProgress(static_cast<int>(fProgress), 1);

    // One 30 fps frame step.
    UpdateScreen(1.0f / 30.0f, 0, 0);

    if (nDone == nTotal)
    {
#if !KOTOR1  // K1 does not flush the input queue
        if (g_pExoInput)
            g_pExoInput->ClearEvents();
#endif

        CExoString sModule;

        CResRef(pModule->m_cStartMovie).CopyToString(sModule);
        SendPlayerToServerArea_AreaLoaded();
    }

    return 1;
}

// Reads the looping-visual-effect records: a 16-bit count, then that many {tag, effect id} pairs,
// each followed by a beam anchor (object id and body part) when the effect is a beam. The payload
// is consumed even when pObject is null.
int CSWCMessage::HandleServerToPlayerUpdateVisualEffects(CSWCObject* pObject)
{
    unsigned short nCount = ReadWORD(16);

    if (MessageReadOverflow(1))
        return 0;

    while (nCount)
    {
        unsigned char  nTag      = ReadBYTE(8);
        unsigned short nEffectId = ReadWORD(16);

        if (MessageReadOverflow(1))
            return 0;

        unsigned long long anchorId;
        unsigned char      nBodyPart;

        if (CLoopingVisualEffect::GetIsBeam(nEffectId))
        {
            anchorId = ReadOBJECTIDClient();
            nBodyPart = ReadBYTE(8);
        }
        else
        {
            nBodyPart = 0;
            anchorId = OBJECT_INVALID;
        }

        if (MessageReadOverflow(1))
            return 0;

        if (pObject && nTag == 'A')
        {
            Vector vOffset = { 0.0f, 0.0f, 0.0f };
            pObject->AddVisualEffect(nEffectId, 0, anchorId, OBJECT_INVALID, nBodyPart, 0, vOffset);
        }
        else if (pObject && nTag == 'D')
        {
            pObject->DeleteVisualEffect(nEffectId);
        }

        nCount--;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerCreatureUpdate_Delete()
{
    unsigned long long objId = ReadOBJECTIDClient();
    int bFast = ReadBOOL();
    int bImmediate = ReadBOOL();
    unsigned int nFadeParam = ReadDWORD(32);
    int bCorpse = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    unsigned int nFadeDelay = bFast ? 0 : 45000;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    if (pCreature)
    {
        CSWSCreature* pServerCreature = pCreature->GetServerCreature();
        CSWCObject*   pClientObject   = static_cast<CSWCObject*>(pCreature);

        if (bCorpse)
        {
            if (pCreature->m_pArea)
            {
                int nBodyBag;
                int nCorpse;

                int bFound = g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                    pCreature->m_pCreatureAppearance->m_nAppearanceId,
                    CExoString("Body_Bag"), &nBodyBag);

                if (bFound)
                {
                    bFound = g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(
                        nBodyBag, CExoString("Corpse"), &nCorpse);
                    if (bFound)
                        bFound = nCorpse;
                }

                if (!bFound)
                    pCreature->m_pArea->AddCorpse(pCreature->m_objectId);
            }
        }
        else
        {
            if (bImmediate)
            {
                pClientObject->SetFadeState(0);
                pClientObject->SetDesiredFadeState(0, 1, nFadeDelay, 0);
            }
            else
            {
                pClientObject->SetFadeState(1);
                pClientObject->SetDesiredFadeState(0, 1, nFadeDelay, nFadeParam);
            }
        }

        pCreature->KillAllVisualEffects();
        pCreature->SetCombatState(0);
        pCreature->m_bVisibilityEffectApplied = 0;

        if (pServerCreature || !pCreature->m_pArea)
            pClientObject->m_bPendingRemoval = 1;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerSoundObject_Play()
{
    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCSoundObject* pSound = g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(objId);
    if (pSound)
        pSound->Play();

    return 1;
}

int CSWCMessage::HandleServerToPlayerSoundObject_Stop()
{
    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCSoundObject* pSound = g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(objId);
    if (pSound)
        pSound->Stop();

    return 1;
}

// GUI channel. An unrecognised sub-opcode returns 1 here, unlike the other dispatchers.

int CSWCMessage::HandleServerToPlayerGuiInventory(unsigned char nType)
{
    if (nType != MSG_GUI_INVENTORY_STATUS)
        return 1;

    ReadBYTE(8);
    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerGuiQuickbar(unsigned char nType)
{
    switch (nType)
    {
    case MSG_GUI_QUICKBAR_SET_ALL_BUTTONS: return HandleServerToPlayerGuiQuickbar_SetButton(0, 1);
    case MSG_GUI_QUICKBAR_SET_BUTTON: return HandleServerToPlayerGuiQuickbar_SetButton(ReadBYTE(8), 0);
    }
    return 0;
}

// EFFECTIVE (K1): ours tail-duplicates the shared `return 0` block into the first overflow check; K1 keeps
// one copy (armv7 -O3 exact, same calls and returns on every path).
int CSWCMessage::HandleServerToPlayerGuiTimingEvent(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_GUI_TIMING_EVENT_INFO)
        HandleServerToPlayerGuiTimingEvent_Info();

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerMapPinEnabled()
{
    if (MessageReadOverflow(1))
        return 0;

    unsigned long long objId = ReadOBJECTIDClient();
    int bActive = ReadBOOL();

    CSWCMapPin* pMapPin = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->GetMapPin(static_cast<unsigned int>(objId), 0);
    if (pMapPin)
        pMapPin->m_bActive = bActive;

    return MessageReadUnderflow(1) == 0;
}

// Adds a map pin built from the wire, including its reference number.

int CSWCMessage::HandleServerToPlayerMapPinReferenceNumber()
{
    Vector vPos = { 0.0f, 0.0f, 0.0f };
    CExoString sName;
    unsigned int nReferenceNumber;
    CSWCArea* pArea;
    CSWCMapPin* pMapPin;

    if (MessageReadOverflow(1))
        return 0;

    vPos.x = ReadFLOAT(1.0f, 32);
    vPos.y = ReadFLOAT(1.0f, 32);
    vPos.z = ReadFLOAT(1.0f, 32);
    sName = ReadCExoString(32);
    nReferenceNumber = ReadDWORD(32);

    if (MessageReadUnderflow(1))
        return 0;

    pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    pMapPin = new CSWCMapPin;
    pMapPin->SetPosition(vPos);
    pMapPin->m_sName = sName;
    pMapPin->m_nPinId = nReferenceNumber;
    pMapPin->m_nPinType = 1;
    pMapPin->m_bActive = 1;

    pArea->AddMapPin(pMapPin);
    return 1;
}

int CSWCMessage::HandleServerToPlayerSaveLoad(unsigned char nType)
{
    if (nType != MSG_SAVE_LOAD_STATUS)
        return 1;

    unsigned char nSubType = ReadBYTE(8);
    unsigned int nFlags = ReadDWORD(4);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    if (nSubType == 2)
        return 1;
    if (nFlags == 0)
        return 1;

    g_pAppManager->m_pClientExoApp->m_pInternal->ShutDownLoadGameLoadBar();
    return 1;
}

int CSWCMessage::HandleServerToPlayerVoiceChatMessage(unsigned char nType)
{
    if (nType != MSG_VOICE_CHAT_PLAY)
        return 1;

    unsigned long long objId = ReadOBJECTIDClient();
    unsigned char nSound = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    if (pCreature)
        pCreature->PlaySoundSetSound(nSound, 1);

    return 1;
}

int CSWCMessage::HandleServerToPlayerGuiContainerObjectMessage(unsigned char nType)
{
    unsigned long long objId = ReadOBJECTIDClient();
    int nFlags = ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    if (nType != MSG_GUI_CONTAINER_OPEN)
        return 1;
    if (objId == OBJECT_INVALID)
        return 1;

    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowContainerGui(objId, nFlags);
    return 1;
}

// Drains the unused message string and payload byte.

int CSWCMessage::HandleServerToPlayerCombatRound(unsigned char nType)
{
    CExoString sMessage;

    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_COMBAT_ROUND_STARTED)
        ReadBYTE(8);

    return 1;
}

int CSWCMessage::HandleServerToPlayerCheatMessage(unsigned char nType)
{
    if (nType == MSG_CHEAT_NASTY)
        return HandleServerToPlayerCheat_DoNasty();

    if (nType == MSG_CHEAT_PONY_RIDE)
    {
        ReadOBJECTIDClient();
        ReadBOOL();
        // Tests overflow only, unlike the sibling arms.
        return MessageReadOverflow(1) == 0;
    }

    if (nType == MSG_CHEAT_DEBUG_MODE)
    {
        int bDebugMode = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        g_pAppManager->m_pClientExoApp->SetDebugMode(bDebugMode);
        return 1;
    }

    return 0;
}

int CSWCMessage::HandleServerToPlayerTriggerUpdate_Update(unsigned long long objId,
                                                          unsigned int nMask)
{
    CSWCTrigger* pTrigger = g_pAppManager->m_pClientExoApp->GetTriggerByGameObjectID(objId);
    if (!pTrigger)
        return 0;

    if ((nMask & MSG_OBJECT_UPDATE_STATE) && pTrigger->m_bIsTrap)
    {
        int bHostile = ReadBOOL();
#if !KOTOR1  // K1 uses the bool only to pick the colour
        pTrigger->m_bHostile = bHostile;
#endif

        if (MessageReadOverflow(1))
            return 0;

        if (bHostile)
            pTrigger->UpdateTriggerColor(0);
        else
            pTrigger->UpdateTriggerColor(1);
    }

    return 1;
}

// Adding a trigger reads the display name, trap and detected flags, cursor, then trap-only hostile
// flag and traps.2da row, highlight height, transition destination and a vertex list whose Z is
// raised by 0.05 so the geometry does not z-fight the walkmesh. Adding always allocates, so a
// duplicate id is only caught by AddExternalObject failing and the new trigger is then leaked. A
// trap takes its model from the traps.2da Model column, falling back to "v_mnfrag"; anything else
// loads the empty resref.

int CSWCMessage::HandleServerToPlayerTriggerUpdate_Add()
{
    CExoString sName;
    CExoString sTransitionDestination;
    unsigned long long objId;
    int bIsTrap;
    int bDetected;
    unsigned char nCursor;
    int bHostile;
    unsigned char nTrapType K2_UNSET;
    float fHighlightHeight;
    unsigned char nVertices;
    CGameObjectArray* pObjectArray;
    CSWCArea* pArea;
    CSWCTrigger* pTrigger;
    float fX;
    float fY;
    float fZ;

    objId = ReadOBJECTIDClient();

    sName = ReadCExoLocStringClient();

    bIsTrap   = ReadBOOL();
    bDetected = ReadBOOL();

    nCursor = ReadBYTE(8);

    bHostile = 0;

    if (bIsTrap)
    {
        bHostile  = ReadBOOL();
        nTrapType = ReadBYTE(8);
    }

    fHighlightHeight = ReadFLOAT(1.0f, 32);

    sTransitionDestination = ReadCExoLocStringClient();

    nVertices = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    pObjectArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    pTrigger = new CSWCTrigger();

    if (bIsTrap)
    {
        CExoString sModel;

        if (!g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(
                nTrapType, CExoString("Model"), &sModel))
            sModel = "v_mnfrag";

        pTrigger->LoadModel(CResRef(sModel), CSWCAnimBase::ANIM_MODEL_BODY, 9);
    }
    else
    {
        pTrigger->LoadModel(CResRef("EMPTY"), CSWCAnimBase::ANIM_MODEL_BODY, 9);
    }

    pTrigger->SetId(objId);
    pTrigger->SetArea(pArea);

    int nResult = pObjectArray->AddExternalObject(objId, static_cast<CGameObject*>(pTrigger), 1);
    if (nResult != CGameObjectArray::SUCCESS)
        return 0;

    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(static_cast<CSWCObject*>(pTrigger));  // idiom-exempt: cast-this-to-base — CSWCTrigger lacks real CSWCObject inheritance

    pTrigger->m_name = sName;

    if (bIsTrap)
    {
        pTrigger->m_bIsTrap = 1;
        pTrigger->m_nTrapType = nTrapType;
    }
    else if (bDetected)
    {
        pTrigger->m_bAreaTransition = 1;
    }

    pTrigger->m_bHostile = bHostile;

    pTrigger->SetCursor(nCursor);

    pTrigger->m_fHighlightHeight = fHighlightHeight;

    if (pTrigger->m_nVertices < nVertices)
    {
        pTrigger->m_pVertices = new Vector[nVertices]();
        pTrigger->m_nVertices = nVertices;
    }

    for (int i = 0; i < nVertices; i++)
    {
        fX = ReadFLOAT(1.0f, 32);
        fY = ReadFLOAT(1.0f, 32);
        fZ = ReadFLOAT(1.0f, 32);

        if (MessageReadOverflow(1))
            return 0;

        pTrigger->m_pVertices[i].x = fX;
        pTrigger->m_pVertices[i].y = fY;
        pTrigger->m_pVertices[i].z = fZ + 0.05f;
    }

    if (bIsTrap)
    {
        pTrigger->AttachToScene(pArea->m_pScene);
    }
    else if (bDetected)
    {
        pTrigger->AddToRenderList(2);
        pTrigger->DisplayTrigger(1);
    }
    else
    {
        pTrigger->AddToRenderList(1);
    }

    pTrigger->SpecialDisplayTrigger(RenderQATriggers);

    pTrigger->m_transitionDestination = sTransitionDestination;

    return 1;
}

// Deletes a trigger. Mode 1 plays traps.2da's ExplosionSound at the trigger and then its activate
// animation, mode 2 only its deactivate animation, anything else unregisters the id and deletes the
// object.
int CSWCMessage::HandleServerToPlayerTriggerUpdate_Delete()
{
    unsigned long long objId = ReadOBJECTIDClient();
    unsigned char nMode = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    CSWCTrigger* pTrigger = g_pAppManager->m_pClientExoApp->GetTriggerByGameObjectID(objId);
    if (!pTrigger)
        return 0;

    pTrigger->RemoveFromRenderList();

    if (nMode == 1)
    {
        CExoString sSound;

        if (pTrigger->m_nTrapType != 0xff
            && g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(
                   pTrigger->m_nTrapType, CExoString("ExplosionSound"), &sSound))
        {
            g_pExoSound->Play3DOneShotSound(CResRef(sSound), pTrigger->m_position,
                                            0.0f, PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0, 0.0f, 0.0f);
        }

        pTrigger->AddFireAndForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TRIGGER_ACTIVATE, 0, 0);
        return 1;
    }

    if (nMode == 2)
    {
        pTrigger->AddFireAndForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TRIGGER_DEACTIVATE, 0, 0);
        return 1;
    }

    CGameObject* pObject;

    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(objId, &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    if (!pObject)
        return 0;

    delete static_cast<CSWCTrigger*>(pObject->AsSWCTrigger());
    return 1;
}

// Dispatches on a letter: A, B and R share the repository arm (its int argument is never read), C
// the container, I the inventory, S the character sheet, Q the quickbar use count.

int CSWCMessage::HandleServerToPlayerUpdate_GuiElements()
{
    unsigned char nType = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case 'A':
        case 'B':
        case 'R':
            return HandleServerToPlayerUpdate_GuiRepository(nType);
        case 'C':
            return HandleServerToPlayerUpdate_GuiContainer();
        case 'I':
            return HandleServerToPlayerUpdate_GuiInventory();
        case 'S':
            return HandleServerToPlayerUpdate_CharacterSheet();
        case 'Q':
            return HandleServerToPlayerUpdate_GuiQuickbarItemUseCount();
    }

    return 0;
}

// The type parameter is unused, since the sub-opcode arrives again as the leading character of the
// payload. The item arm resolves an item and then discards it.

int CSWCMessage::HandleServerToPlayerUpdate_GuiRepository(int nType)
{
    K2_UNUSED(nType);
    char nOperation = ReadCHAR(8);

    if (MessageReadOverflow(1))
        return 0;

    if (nOperation == 'A')
    {
        CSWCItem* pItem;

        if (MessageReadOverflow(1))
            return 0;

        HandleServerToPlayerItemUpdate_Add(1, &pItem);
        return 1;
    }

    if (nOperation == 'D')
    {
        unsigned long long objId = ReadOBJECTIDClient();

        if (MessageReadOverflow(1))
            return 0;

        g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
        return 1;
    }

    if (nOperation == 'U')
    {
        unsigned long long objId = ReadOBJECTIDClient();
        int nStackSize = ReadDWORD(32);
        int nCost = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;

        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
        if (pItem)
        {
            pItem->SetStackSize(nStackSize);
            pItem->m_nCost = nCost;
        }
        return 1;
    }

    return 0;
}


int CSWCMessage::HandleServerToPlayerItemAppearance(unsigned int* pnBaseItem,
                                                    unsigned char* pnAppearance,
                                                    unsigned char* pnBodyVariation,
                                                    unsigned char* pnTextureVariation)
{
    *pnBaseItem = ReadDWORD(32);

    if (MessageReadOverflow(1))
        return 0;

    *pnAppearance = ReadBYTE(8);

    if (g_pRules->GetBaseItem(*pnBaseItem)->m_nModelType == 1)
    {
        *pnBodyVariation = ReadBYTE(8);
        *pnTextureVariation = ReadBYTE(8);
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerActiveItemProperties(unsigned long long oidItem)
{
    CExoString sDisplayName;
    CSWCItem* pItem;
    short nUpgrade;
    int          bFlag1;
    unsigned int nQuantity;
    unsigned int nStackSize;
    int          bFlag2;
    int          bFlag3;
    unsigned char nNumProperties;
    unsigned short nType;
    unsigned short nSubType;
    unsigned short nCostTableValue;
    unsigned char  nParam;
    unsigned char nUsesMask;
    unsigned char nUses;
    CSWItemProperty* pProperty;

    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidItem);

    if (pItem && pItem->GetBaseItem()->m_nItemType >= kItemTypeFirstArmor &&
        pItem->GetBaseItem()->m_nItemType <= kItemTypeLastArmor)
    {
        nUpgrade = ReadSHORT(16);

        if (MessageReadOverflow(1))
            return 0;

        pItem->m_nBaseArmorClass = nUpgrade;
    }

    sDisplayName = ReadCExoLocStringClient();

    bFlag1 = ReadBOOL();
    nQuantity = ReadDWORD(32);
    nStackSize = ReadDWORD(32);
    bFlag2 = ReadBOOL();
    bFlag3 = ReadBOOL();
    nNumProperties = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    if (nNumProperties != 0)
    {
        if (pItem)
            pItem->SetNumActiveProperties(nNumProperties);

        for (int i = 0; i < nNumProperties; i++)
        {
            nType = ReadWORD(16);
            nSubType = ReadWORD(16);
            nCostTableValue = ReadWORD(16);
            nParam = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            if (pItem)
            {
                pItem->GetActiveProperty(i)->m_type = nType;
                pItem->GetActiveProperty(i)->m_subType = nSubType;
                pItem->GetActiveProperty(i)->m_costTableValue = nCostTableValue;
                pItem->GetActiveProperty(i)->m_nParam1 = nParam;
                pItem->GetActiveProperty(i)->m_bUseable = 1;
                pItem->GetActiveProperty(i)->m_nUsesPerDay = 0;
                pItem->GetActiveProperty(i)->m_nUpgradeType = 0xff;
            }
        }
    }

    if (pItem)
    {
        pItem->m_bIdentified = bFlag1;
        pItem->m_nCost = nQuantity;
        pItem->SetStackSize(nStackSize);
        pItem->m_sDisplayName = sDisplayName;
        pItem->m_bCanUse = bFlag2;
        pItem->m_bStolen = bFlag3;
    }

    unsigned char nActiveMask;
    g_pAppManager->m_pClientExoApp->GetInGameGui();

    nActiveMask = ReadBYTE(8);
    nUsesMask = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    for (int i = 0; i < 8; i++)
    {
        if ((nUsesMask & (1 << i)) == 0)
            continue;

        nUses = ReadBYTE(8);

        if (MessageReadOverflow(1))
            return 0;

        if (pItem)
        {
            pItem->SetItemPropertyUsesLeft(i, nUses);

            pProperty = pItem->GetActiveProperty(i);
            if (pProperty)
                pProperty->m_bUseable = (nActiveMask & (1 << i)) != 0;
        }
    }

    return 1;
}

// Looping visual effect channel. Beam effects carry a second payload: both endpoint ids, two
// bytes, the source position and, when the target id is the invalid-object sentinel, a trailing
// float. An endpoint that is missing or the wrong type gets a throwaway "c_dummy" creature
// spawned at the wire position, registered as an internal object and faded in; the early exits
// inside those blocks leak it.
int CSWCMessage::HandleServerToPlayerCreatureUpdate_VisEffect()
{
    Vector vSourcePos;
    Vector vPos;
    unsigned long long dummyId;

    vSourcePos.x = 0.0f;
    vSourcePos.y = 0.0f;
    vSourcePos.z = 0.0f;
    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;

    unsigned long long targetId  = ReadOBJECTIDClient();
    unsigned short     nEffectId = ReadWORD(16);

    if (MessageReadOverflow(1))
        return 0;

    vPos.x = ReadFLOAT(1.0f, 32);
    vPos.y = ReadFLOAT(1.0f, 32);
    vPos.z = ReadFLOAT(1.0f, 32);

    if (CLoopingVisualEffect::GetIsBeam(nEffectId))
    {
        unsigned long long sourceId  = ReadOBJECTIDClient();
        unsigned long long beamEndId = ReadOBJECTIDClient();
        unsigned char      nA        = ReadBYTE(8);
        unsigned char      nB        = ReadBYTE(8);

        vSourcePos.x = ReadFLOAT(1.0f, 32);
        vSourcePos.y = ReadFLOAT(1.0f, 32);
        vSourcePos.z = ReadFLOAT(1.0f, 32);

        float fDuration = 0.0f;
        if (targetId == OBJECT_INVALID)
            fDuration = ReadFLOAT(1.0f, 32);

        if (MessageReadOverflow(1))
            return 0;

        // Stand-in for a missing or unsuitable source object.
        CGameObject* pSourceObject = g_pAppManager->m_pClientExoApp->GetGameObject(sourceId);

        if (pSourceObject == 0 ||
            (pSourceObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE &&
             pSourceObject->m_objectType != CGameObject::OBJECT_TYPE_PLACEABLE &&
             pSourceObject->m_objectType != CGameObject::OBJECT_TYPE_DOOR))
        {
        CSWCCreature* pDummy = new CSWCCreature();
            pDummy->m_nAppearanceRequestTimer = 0;

            if (pDummy->LoadModel(CResRef("c_dummy"), CSWCAnimBase::ANIM_MODEL_BODY, 0))
            {
                CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

                if (pArea)
                {
                    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(
                            dummyId, static_cast<CGameObject*>(pDummy), 0) == 0)
                        return 0;

                    pDummy->SetId(dummyId);
                    pDummy->SetArea(pArea);
                    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
                        pDummy);
                    pDummy->AttachToScene(pArea->m_pScene);
                    pDummy->SetPosition(vSourcePos);
                    pDummy->m_desiredPosition = vSourcePos;
                    pDummy->m_pCreatureAppearance->m_nAppearanceId = kAppearanceDummy;
                }
            }

            pDummy->SetDesiredFadeState(
                1, 1, fDuration == 0.0f ? 6000 : static_cast<unsigned int>(fDuration * 1000.0f), 0);
            pDummy->m_bPendingRemoval = 1;
        }

        // The beam's target end receives the effect.
        CSWCObject*  pTarget;
        CGameObject* pTargetObject = g_pAppManager->m_pClientExoApp->GetGameObject(targetId);

        if (pTargetObject == 0 ||
            (pTargetObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE &&
             pTargetObject->m_objectType != CGameObject::OBJECT_TYPE_PLACEABLE &&
             pTargetObject->m_objectType != CGameObject::OBJECT_TYPE_DOOR))
        {
        CSWCCreature* pDummy = new CSWCCreature();
            pDummy->m_nAppearanceRequestTimer = 0;

            if (pDummy->LoadModel(CResRef("c_dummy"), CSWCAnimBase::ANIM_MODEL_BODY, 0))
            {
                CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

                if (pArea)
                {
                    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(
                            dummyId, static_cast<CGameObject*>(pDummy), 0) != CGameObjectArray::SUCCESS)
                        return 0;

                    pDummy->SetId(dummyId);
                    pDummy->SetArea(pArea);
                    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
                        pDummy);
                    pDummy->AttachToScene(pArea->m_pScene);
                    pDummy->SetPosition(vPos);
                    pDummy->m_desiredPosition = vPos;
                    pDummy->m_pCreatureAppearance->m_nAppearanceId = kAppearanceDummy;
                }
            }

            pDummy->SetDesiredFadeState(
                1, 1, fDuration == 0.0f ? 6000 : static_cast<unsigned int>(fDuration * 1000.0f), 0);
            pDummy->m_bPendingRemoval = 1;

            pTarget = pDummy;
        }
        else
        {
            pTarget = static_cast<CSWCObject*>(pTargetObject->AsSWCObject());
        }

        // The trailing float also serves as the "anchored to a point" flag. Zero gives the effect
        // the wire position, otherwise the beam tracks its endpoints and a null vector goes out.
        if (fDuration == 0.0f)
        {
            if (targetId == CGameObjectArray::INVALIDOBJID)
                pTarget->AddVisualEffect(nEffectId, 1, sourceId, beamEndId, nA, nB, vPos);
            else
            {
                Vector vNull = {};
                pTarget->AddVisualEffect(nEffectId, 1, sourceId, beamEndId, nA, nB, vNull);
            }
        }
        else
        {
            if (targetId == CGameObjectArray::INVALIDOBJID)
                pTarget->AddVisualEffect(nEffectId, 0, sourceId, beamEndId, nA, nB, vPos);
            else
            {
                Vector vNull = {};
                pTarget->AddVisualEffect(nEffectId, 0, sourceId, beamEndId, nA, nB, vNull);
            }
        }

        return 1;
    }

    if (MessageReadOverflow(1))
        return 0;

    if (MessageReadUnderflow(1))
        return 0;

    // A sentinel target id means there is no object at all, so the effect hangs on a stand-in
    // creature at the wire position, faded in over ten seconds.
    if (targetId == OBJECT_INVALID)
    {
        CSWCCreature* pDummy = new CSWCCreature();
        pDummy->m_nAppearanceRequestTimer = 0;

        if (pDummy->LoadModel(CResRef("c_dummy"), CSWCAnimBase::ANIM_MODEL_BODY, 0))
        {
            CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

            if (pArea)
            {
                if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(
                        dummyId, static_cast<CGameObject*>(pDummy), 0) != CGameObjectArray::SUCCESS)
                    return 0;

                pDummy->SetId(dummyId);
                pDummy->SetArea(pArea);
                g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
                    pDummy);
                pDummy->AttachToScene(pArea->m_pScene);
                pDummy->SetPosition(vPos);
                pDummy->m_desiredPosition = vPos;
                pDummy->m_pCreatureAppearance->m_nAppearanceId = kAppearanceDummy;
            }
        }

        pDummy->SetDesiredFadeState(1, 1, 10000, 0);
        pDummy->m_bPendingRemoval = 1;
        pDummy->AddVisualEffect(nEffectId, 1, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vPos);

        return 1;
    }

    CGameObject* pTargetObject = g_pAppManager->m_pClientExoApp->GetGameObject(targetId);

    if (pTargetObject == 0)
        return 0;

    static_cast<CSWCObject*>(pTargetObject)->AddVisualEffect(nEffectId, 1, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vPos);  // idiom-exempt: cast-this-to-base — CGameObject is CSWCObject's real base at offset 0

    return 1;
}


int CSWCMessage::HandleServerToPlayerJournalMessage_AddWorld()
{
    CExoString sName;
    CExoString sText;
    int nId;
    unsigned int nDate;
    unsigned int nTime;
    CSWCWorldJournal* pJournal = g_pAppManager->m_pClientExoApp->GetWorldJournal();

    if (MessageReadOverflow(1))
        return 0;

    nId = ReadINT(32);
    sName = ReadCExoString(32);
    sText = ReadCExoString(32);
    nDate = ReadDWORD(32);
    nTime = ReadDWORD(32);

    pJournal->AddEntry(nId, sName, sText, nDate, nTime);
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_AddWorldStrref()
{
    CSWCWorldJournal* pJournal = g_pAppManager->m_pClientExoApp->GetWorldJournal();

    if (MessageReadOverflow(1))
        return 0;

    unsigned int nNameStrRef = ReadDWORD(32);
    unsigned int nTextStrRef = ReadDWORD(32);
    unsigned int nDate = ReadDWORD(32);
    unsigned int nTime = ReadDWORD(32);

    pJournal->AddEntryStrRef(nNameStrRef, nTextStrRef, nDate, nTime);
    return MessageReadUnderflow(1) == 0;
}

// Refreshes one item's active properties: which are switched on, and how many uses each has left.
// The uses-left loop walks 64 bit positions over an eight-byte buffer, so positions 32..39 alias
// bits 0..7 and the reads run off the end of the buffer.
int CSWCMessage::HandleServerToPlayerActiveItemPropertyUpdate(unsigned char nType)
{
    unsigned char nUsesLeft[8] K2_UNSET;

    unsigned long long oidItem = ReadOBJECTIDClient();
    unsigned char nActiveMask = ReadBYTE(8);
    unsigned char nUsesMask = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    if (nType != MSG_ITEM_PROPERTY_UPDATE_ACTIVE)
        return 1;

    for (int i = 0; i < 8; i++)
    {
        if ((nUsesMask & (1 << i)) == 0)
            continue;

        unsigned char nUses = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        nUsesLeft[i] = nUses;
    }

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidItem);
    if (!pItem)
        return 1;

    for (int i = 0; i < pItem->m_nNumActiveProperties; i++)
    {
        CSWItemProperty* pProperty = pItem->GetActiveProperty(i);
        if (pProperty)
            pProperty->m_bUseable = (nActiveMask & (1 << i)) != 0;
    }

#ifdef K2_FIX_ENGINE_BUGS
    for (int i = 0; i < 8; i++)
#else
    for (int i = 0; i < 64; i++)
#endif
    {
        if (nUsesMask & (1 << i))
            pItem->SetItemPropertyUsesLeft(i, nUsesLeft[i]);
    }

    return 1;
}

// Fills an SJournalEntry from the wire and adds it to the quest journal. Both localized fields
// arrive as a single already-picked language string, spliced into an otherwise empty CExoLocString.
int CSWCMessage::HandleServerToPlayerJournalMessage_AddQuest()
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    SJournalEntry entry;

    if (MessageReadOverflow(1))
        return 0;

    entry.m_locName.AddString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                              ReadCExoLocStringClient(), 0);
    entry.m_locText.AddString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                              ReadCExoLocStringClient(), 0);
    entry.m_nDate = ReadDWORD(32);
    entry.m_nTime = ReadDWORD(32);
    entry.m_sPlotId = ReadCExoString(32);
    entry.m_nState = ReadINT(32);
    entry.m_nPriority = ReadDWORD(32);
    entry.m_nPicture = ReadWORD(16);
    entry.m_bEnd = ReadBOOL();
    entry.m_bNewUnobserved = 1;
    entry.m_nPlotIndex = ReadINT(32);
    entry.m_nPlanetId = ReadINT(32);
    entry.m_nQuestEntryXP = ReadINT(32);

    pJournal->Add(entry, MSG_JOURNAL_QUEST_ALL);
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_RemoveQuest()
{
    CExoString sPlotId;
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();

    if (MessageReadOverflow(1))
        return 0;

    sPlotId = ReadCExoString(32);
    pJournal->Remove(sPlotId);
    return MessageReadUnderflow(1) == 0;
}


int CSWCMessage::HandleServerToPlayerJournalMessage_SetQuestPicture()
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    CExoString sPlotId;
    int nPicture;

    if (MessageReadOverflow(1))
        return 0;

    sPlotId = ReadCExoString(32);
    nPicture = ReadINT(32);

    pJournal->SetJournalPicture(sPlotId, nPicture);
    return MessageReadUnderflow(1) == 0;
}

// Rebuilds the quest journal in bulk from one wire record per entry, each a bit-masked partial
// update. The loop reuses one SJournalEntry, so a field a record does not ship keeps the previous
// record's value, and every guard is (nMask & bit) || !bPartial. CallFullUpdateComplete() runs on
// every exit, including the failure paths.

int CSWCMessage::HandleServerToPlayerJournalMessage_FullQuestUpdate()
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    SJournalEntry entry;
    int bClearAll;
    int nCount;
    unsigned short nMask;
    int bPartial;

    if (MessageReadOverflow(1))
    {
        pJournal->CallFullUpdateComplete();
        return 0;
    }

    if (!ReadBOOL())
    {
        pJournal->CallFullUpdateComplete();
        return MessageReadUnderflow(1) == 0;
    }

    pJournal->DisableSort();

    bClearAll = ReadBOOL();
    if (bClearAll)
        pJournal->ClearAll();

    nCount = ReadINT(32);

    for (int i = 0; i < nCount; i++)
    {
        nMask = ReadWORD(16);
        entry.m_sPlotId = ReadCExoString(32);

        if (nMask & MSG_JOURNAL_QUEST_REMOVED)
        {
            pJournal->Remove(entry.m_sPlotId);
            continue;
        }

        bPartial = (nMask & MSG_JOURNAL_QUEST_ALL) == 0 && !bClearAll;

        if ((nMask & MSG_JOURNAL_QUEST_NAME) || !bPartial)
            entry.m_locName.AddString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                      ReadCExoLocStringClient(), 0);
        if ((nMask & MSG_JOURNAL_QUEST_TEXT) || !bPartial)
            entry.m_locText.AddString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                      ReadCExoLocStringClient(), 0);
        if ((nMask & MSG_JOURNAL_QUEST_DATE) || !bPartial)
            entry.m_nDate = ReadDWORD(32);
        if ((nMask & MSG_JOURNAL_QUEST_TIME) || !bPartial)
            entry.m_nTime = ReadDWORD(32);
        if ((nMask & MSG_JOURNAL_QUEST_STATE) || !bPartial)
            entry.m_nState = ReadINT(32);
        if ((nMask & MSG_JOURNAL_QUEST_PRIORITY) || !bPartial)
            entry.m_nPriority = ReadDWORD(32);
        if ((nMask & MSG_JOURNAL_QUEST_PICTURE) || !bPartial)
            entry.m_nPicture = ReadWORD(16);
        if ((nMask & MSG_JOURNAL_QUEST_END) || !bPartial)
            entry.m_bEnd = ReadBOOL();
        if ((nMask & MSG_JOURNAL_QUEST_PLOT_INDEX) || !bPartial)
            entry.m_nPlotIndex = ReadINT(32);
        if ((nMask & MSG_JOURNAL_QUEST_PLANET) || !bPartial)
            entry.m_nPlanetId = ReadINT(32);
        if ((nMask & MSG_JOURNAL_QUEST_EXPERIENCE) || !bPartial)
            entry.m_nQuestEntryXP = ReadINT(32);

        if (nMask & MSG_JOURNAL_QUEST_OBSERVED)
            entry.m_bNewUnobserved = 0;
        else
            entry.m_bNewUnobserved = 1;

        pJournal->Add(entry, nMask);
    }

    pJournal->EnableSort();
    pJournal->CallFullUpdateComplete();
    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerJournalMessage_Updated()
{
    g_pAppManager->m_pClientExoApp->GetWorldJournal();

    CExoString sText;
    int bQuest;
    int bWorld;

    if (MessageReadOverflow(1))
        return 0;

    sText = ReadCExoLocStringClient();
    bQuest = ReadBOOL();
    bWorld = ReadBOOL();

    g_pAppManager->m_pClientExoApp->JournalUpdated(bQuest, bWorld, sText);
    return MessageReadUnderflow(1) == 0;
}

// These are shells whose payload handling is gone: they build their scratch locals, drain one
// field and report success.

int CSWCMessage::HandlePlayerToServerShutDownServer(unsigned char nType)
{
    K2_UNUSED(nType);
    STR_RES strRes;

    if (MessageReadOverflow(1))
        return 0;

    ReadDWORD(32);

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

int CSWCMessage::HandleServerToPlayerSaveGameStatus()
{
    STR_RES strRes;
    CExoString sMessage;

    ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}


int CSWCMessage::HandleServerToPlayerDebugInfo(unsigned char nType)
{
    K2_UNUSED(nType);
    unsigned long long objId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
    if (pObject)
    {
        switch (pObject->m_objectType)
        {
            case GAME_OBJECT_TYPE_CREATURE:
                return HandleServerToPlayerDebugInfo_Creature(objId);
            case GAME_OBJECT_TYPE_ITEM:
                return HandleServerToPlayerDebugInfo_Item(objId);
            case GAME_OBJECT_TYPE_PLACEABLE:
                return HandleServerToPlayerDebugInfo_Placeable(objId);
        }
        return 0;
    }

    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_objectId == objId)
        return HandleServerToPlayerDebugInfo_Area(objId);

    return 0;
}

int CSWCMessage::HandleServerToPlayerSound(unsigned char nType)
{
    if (nType == MSG_SOUND_OBJECT_PLAY)
    {
        unsigned long long objId = ReadOBJECTIDClient();

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        CSWCSoundObject* pSound =
            g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(objId);
        if (pSound)
            pSound->Play();

        return 1;
    }

    if (nType == MSG_SOUND_OBJECT_STOP)
    {
        unsigned long long objId = ReadOBJECTIDClient();

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        CSWCSoundObject* pSound =
            g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(objId);
        if (pSound)
            pSound->Stop();

        return 1;
    }

    if (nType == MSG_SOUND_OBJECT_CHANGE_POSITION)
        return HandleServerToPlayerSoundObject_ChangePosition();

    return 0;
}

int CSWCMessage::HandleServerToPlayerSoundObject_ChangePosition()
{
    Vector vPosition;

    unsigned long long objId = ReadOBJECTIDClient();

    vPosition.x = ReadFLOAT(1.0f, 32);
    vPosition.y = ReadFLOAT(1.0f, 32);
    vPosition.z = ReadFLOAT(1.0f, 32);

    if (MessageReadOverflow(1))
        return 0;
    if (MessageReadUnderflow(1))
        return 0;

    CSWCSoundObject* pSound = g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(objId);
    if (pSound)
        pSound->ChangePosition(vPosition);

    return 1;
}

// Adding a door reads the id, scenery flag, doortypes.2da row and, when that row is 0, a
// genericdoors.2da row, then display name, an unused bool, appearance, five flags and the
// transition destination. A nonzero door type reads "Model" and "VisibleModel" from doortypes.2da;
// a zero one reads "ModelName" from genericdoors.2da and leaves VisibleModel at 1. A failed lookup
// loads an empty model. Four appearance ids map to door states 0-3 while the animation is set from
// the raw word, and a door whose VisibleModel column is 0 is forced to state 3.


int CSWCMessage::HandleServerToPlayerDoorUpdate_Add()
{
    CExoString sModel;
    CExoString sName;
    CExoString sTransitionDestination;
    unsigned long long objId;
    int nVisibleModel = 1;
    CGameObjectArray* pObjectArray;
    CSWCArea* pArea;
    int bScenery;
    unsigned char nDoorType;
    // The store below is unconditional, but the value is only assigned in the nDoorType == 0 arm,
    // so a door with a nonzero type ships whatever the local held.
    unsigned char nGenericType;
    unsigned short nAppearance;
    int bActionable;
    int bLockable;
    int bPlot;
#if !KOTOR1
    int bMinesSuppressed;
#endif
    int bAreaTransition;
    CSWCDoor* pDoor;

    pObjectArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    objId = ReadOBJECTIDClient();

    bScenery = ReadBOOL();

    nDoorType = ReadBYTE(8);

#ifdef K2_FIX_ENGINE_BUGS
    nGenericType = 0xff;
#endif

    if (nDoorType == 0)
        nGenericType = ReadBYTE(8);

    sName = ReadCExoLocStringClient();

    ReadBOOL();

    nAppearance = ReadWORD(16);

    bActionable      = ReadBOOL();
    bLockable        = ReadBOOL();
    bPlot            = ReadBOOL();
#if !KOTOR1  // K1 has no door mines and no mines bool on the wire
    bMinesSuppressed = ReadBOOL();
#endif
    bAreaTransition  = ReadBOOL();

    sTransitionDestination = ReadCExoLocStringClient();

    if (MessageReadOverflow(1))
        return 0;

    if (nDoorType != 0)
    {
        g_pRules->m_p2DArrays->m_pDoorTypes->GetCExoStringEntry(
            nDoorType, CExoString("Model"), &sModel);
        g_pRules->m_p2DArrays->m_pDoorTypes->GetINTEntry(
            nDoorType, CExoString("VisibleModel"), &nVisibleModel);
    }
    else
    {
        g_pRules->m_p2DArrays->m_pGenericDoors->GetCExoStringEntry(
            nGenericType, CExoString("ModelName"), &sModel);
    }

    pDoor = g_pAppManager->m_pClientExoApp->GetDoorByGameObjectID(objId);

    if (pDoor)
    {
        pDoor->SetObjectVisibilityOnSetArea(pArea);
    }
    else
    {
        pDoor = new CSWCDoor();

        pDoor->LoadModel(CResRef(sModel), CSWCAnimBase::ANIM_MODEL_BODY, 5);
        pDoor->SetId(objId);
        pDoor->SetArea(pArea);

        if (pObjectArray->AddExternalObject(objId, static_cast<CGameObject*>(pDoor), 1)
            != CGameObjectArray::SUCCESS)
            return 0;

        CClientAIMaster* pAIMaster = g_pAppManager->m_pClientExoApp->GetClientAIMaster();
        pAIMaster->AddObject(pDoor);
    }

    pDoor->AttachToScene(pArea->m_pScene);

    pDoor->m_name = sName;

    switch (nAppearance)
    {
    case SERVER_ANIMATION_CLOSED:
        pDoor->SetState(0);
        break;
    case SERVER_ANIMATION_DOOR_OPEN1:
        pDoor->SetState(1);
        break;
    case SERVER_ANIMATION_DOOR_OPEN2:
        pDoor->SetState(2);
        break;
    case SERVER_ANIMATION_DESTROYED:
        pDoor->SetState(3);
        break;
    }

    pDoor->SetAnimationWithoutTrigger(nAppearance);

    pDoor->m_bLocked = bActionable;
    pDoor->m_bLockable = bLockable;
    pDoor->m_bPlot = bPlot;
#if !KOTOR1
    pDoor->m_bNotBlastable = bMinesSuppressed;
#endif

    pDoor->SetIsAreaTransition(bAreaTransition);

    pDoor->m_nAppearance = nDoorType;
    pDoor->m_nGenericType = nGenericType;

    if (nVisibleModel == 0)
    {
        pDoor->m_bPlot = 1;
        pDoor->SetState(3);
    }

    pDoor->m_bStatic = bScenery;

    if (bScenery)
        g_pAppManager->m_pClientExoApp->AddObjectToHitcheckIgnoreList(pDoor->m_objectId);

    pDoor->m_transitionDestination = sTransitionDestination;

#if !KOTOR1  // K1 has no tweak-colour illumination
    CSWSDoor* pServerDoor = pDoor->GetServerDoor();

    if (pServerDoor && pServerDoor->m_bUseTweakColor)
    {
        unsigned int nColor = pServerDoor->m_tweakColor;

        Gob* pModel = pDoor->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);

        if (pModel)
            pModel->EnableIllumination(1, (nColor & 0xff) / 255.0f,
                                       ((nColor >> 8) & 0xff) / 255.0f,
                                       ((nColor >> 16) & 0xff) / 255.0f);
    }
#endif

    return 1;
}

// Adding a placeable reads the id, display name, a bool, appearance, an animation word that is not
// stored, and eight more bools. The appearance indexes placeables.2da directly. With the gore
// option set the row's "ModelName" is used, otherwise "LowGore" is tried first; a failed lookup
// loads an empty model. When the server placeable is a corpse whose creature resolves, the new
// placeable inherits that creature's animation state and the creature is removed and deleted.

int CSWCMessage::HandleServerToPlayerPlaceableUpdate_Add()
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();

    unsigned long long objId = ReadOBJECTIDClient();

    CExoString sDisplayName = ReadCExoLocStringClient();

    int bStatic = ReadBOOL();

#if KOTOR1  // K1 sends a byte appearance and no mines-gate bool
    int nAppearance = ReadBYTE(8);
#else
    int nAppearance = ReadINT(32);
#endif

    unsigned short nAnimation = ReadWORD(16);

    int bUseable        = ReadBOOL();
    int bIgnoreHitcheck = ReadBOOL();
    int bTrapDisarmable       = ReadBOOL();
    int bLockable       = ReadBOOL();
    int bActionable     = ReadBOOL();
#if !KOTOR1
    int bNotBlastable       = ReadBOOL();
#endif
    int bReserved1       = ReadBOOL();
    int bHasDescription       = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    CSWCPlaceable* pPlaceable =
        g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(objId);

    if (pPlaceable)
    {
        pPlaceable->m_bPendingRemoval = 0;
        pPlaceable->SetDesiredFadeState(1, 0, 0, 0);
        pPlaceable->SetObjectVisibilityOnSetArea(pArea);
    }
    else
    {
        CExoString sModel;

        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel)
        {
            g_pRules->m_p2DArrays->m_pPlaceables->GetCExoStringEntry(
                nAppearance, CExoString("ModelName"), &sModel);
        }
        else if (!g_pRules->m_p2DArrays->m_pPlaceables->GetCExoStringEntry(
                     nAppearance, CExoString("LowGore"), &sModel))
        {
            g_pRules->m_p2DArrays->m_pPlaceables->GetCExoStringEntry(
                nAppearance, CExoString("ModelName"), &sModel);
        }

        if (!g_pAppManager->m_pServerExoApp)
            return 0;

        pPlaceable = new CSWCPlaceable();

        CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;

        CSWSPlaceable* pServerPlaceable = pServerApp->GetPlaceableByGameObjectID(
            pServerApp->ClientToServerObjectId(objId));

        CSWCCreature* pCorpse = 0;

        if (pServerPlaceable && pServerPlaceable->m_corpse.m_bIsCorpse)
            pCorpse = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                    pServerPlaceable->m_oidCorpse));

        if (pCorpse && pCorpse->GetAnimationBase())
        {
            pPlaceable->AcquireAnimationBase(
                pCorpse);
            pPlaceable->SetLoopingAnimation(pCorpse->GetLoopingAnimation());

            g_pAppManager->m_pClientExoApp->GetClientAIMaster()->m_list.RemoveObject(
                pCorpse);

            CGameObject* pDeleted;

            if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(
                    pCorpse->m_objectId, &pDeleted) == CGameObjectArray::SUCCESS
                && pDeleted)
                delete pCorpse;
        }
        else
        {
            pPlaceable->LoadModel(CResRef(sModel), CSWCAnimBase::ANIM_MODEL_BODY, 10);
        }

#if KOTOR1
        pPlaceable->SetAppearance(static_cast<unsigned char>(nAppearance));
#else
        pPlaceable->SetAppearance(nAppearance);
#endif
        pPlaceable->SetId(objId);
        pPlaceable->SetArea(pArea);

        if (pObjectArray->AddExternalObject(objId, static_cast<CGameObject*>(pPlaceable), 1)
            != CGameObjectArray::SUCCESS)
            return 0;

        g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
            static_cast<CSWCObject*>(pPlaceable));  // idiom-exempt: cast-this-to-base — CSWCPlaceable lacks real CSWCObject inheritance

        pPlaceable->LoadLight();

        if (pServerPlaceable && pServerPlaceable->m_bIsBodyBag && pArea
            && !pServerPlaceable->m_bIsBodyBagVisible
            && pArea->ParentCorpseExists(pPlaceable->m_objectId))
        {
            pPlaceable->SetDesiredFadeState(0, 0, 0, 0);
            pPlaceable->SetFadeState(0);
        }
    }

    pPlaceable->m_name = sDisplayName;

#ifdef K2_FIX_ENGINE_BUGS
    if (pArea)
#endif
    pPlaceable->AttachToScene(pArea->m_pScene);

    pPlaceable->m_bPlot        = bUseable;
    pPlaceable->m_bLockable        = bLockable;
    pPlaceable->m_bLocked        = bActionable;
#if !KOTOR1
    pPlaceable->m_bNotBlastable        = bNotBlastable;
#endif
    pPlaceable->m_bHasDescription        = bHasDescription;
    pPlaceable->m_bHostileFaction        = bStatic;
    pPlaceable->m_bIgnoreHitcheck = bIgnoreHitcheck;
    pPlaceable->m_bReserved1        = bReserved1;
    pPlaceable->m_bTrapDisarmable        = bTrapDisarmable;

    pPlaceable->SetAnimation(nAnimation, 0, 0);

    if (pPlaceable->m_bIgnoreHitcheck)
        g_pAppManager->m_pClientExoApp->RemoveObjectFromHitcheckIgnoreList(
            pPlaceable->m_objectId);
    else
        g_pAppManager->m_pClientExoApp->AddObjectToHitcheckIgnoreList(
            pPlaceable->m_objectId);

    pPlaceable->SetEnvMap(1, 1, CResRef("DEFAULT"));

#if !KOTOR1  // K1 has no tweak-colour illumination
    CSWSPlaceable* pServer = pPlaceable->GetServerPlaceable();

    if (pServer && pServer->m_bUseTweakColor)
    {
        unsigned int nColor = pServer->m_tweakColor;

        Gob* pModel = pPlaceable->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);

        if (pModel)
        {
            float fRed   = (nColor & 0xff) / 255.0f;
            float fGreen = ((nColor >> 8) & 0xff) / 255.0f;
            float fBlue  = ((nColor >> 16) & 0xff) / 255.0f;

            // The material keywords are matched against the placeable's own display name.
            if (strstr(pPlaceable->GetName().CStr(), "Chemical"))
            {
                fRed += 0.16f;
                fGreen += 0.16f;
                fBlue += 0.16f;
            }

            if (strstr(pPlaceable->GetName().CStr(), "Plasteel"))
            {
                fRed += 0.1f;
                fGreen += 0.1f;
                fBlue += 0.1f;
            }

            pModel->EnableIllumination(1, fRed, fGreen, fBlue);
        }
    }
#endif

    return 1;
}

// The door and placeable update handlers differ in guard order. The door tests its object before
// the mask bit and returns 0 on a miss; the placeable drains the payload first and null-checks
// after, returning 1.

int CSWCMessage::HandleServerToPlayerDoorUpdate_Update(unsigned long long objId,
                                                       unsigned int nMask)
{
    CSWCDoor* pDoor = g_pAppManager->m_pClientExoApp->GetDoorByGameObjectID(objId);
    if (!pDoor)
        return 0;

    if (!(nMask & MSG_OBJECT_UPDATE_STATE))
        return 1;

    int bLocked = ReadBOOL();
    int bHilight = ReadBOOL();
    int bActionable = ReadBOOL();
    int bBashable = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    pDoor->m_bHostileFaction = bLocked;
    pDoor->m_bLocked = bActionable;
    pDoor->m_bPlot = bBashable;

    if (bHilight)
    {
        if (!pDoor->m_bHilightShown)
            pDoor->HilightDoor(1);
        pDoor->m_bHilighted = 1;
    }
    else
    {
        if (pDoor->m_bHilightShown)
            pDoor->HilightDoor(0);
        pDoor->m_bHilighted = 0;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerPlaceableUpdate_Update(unsigned long long objId,
                                                            unsigned int nMask)
{
    CSWCPlaceable* pPlaceable =
        g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(objId);

    if (!(nMask & MSG_OBJECT_UPDATE_STATE))
        return 1;

    int bLocked = ReadBOOL();
    int bHilight = ReadBOOL();
    int bActionable = ReadBOOL();
    int bBashable = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    if (!pPlaceable)
        return 1;

    pPlaceable->m_bHostileFaction = bLocked;
    pPlaceable->m_bLocked = bActionable;
    pPlaceable->m_bPlot = bBashable;

    if (bHilight)
    {
        if (!pPlaceable->m_bHilightShown)
            pPlaceable->HilightPlaceable(1);
        pPlaceable->m_bHilighted = 1;
    }
    else
    {
        if (pPlaceable->m_bHilightShown)
            pPlaceable->HilightPlaceable(0);
        pPlaceable->m_bHilighted = 0;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerDoorUpdate_Delete()
{
    CGameObject* pObject;

    unsigned long long objId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(objId, &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    if (!pObject)
        return 0;

    if (pObject->AsSWCDoor())
    {
        g_pAppManager->m_pClientExoApp->GetClientAIMaster();
        delete static_cast<CSWCDoor*>(pObject->AsSWCDoor());
    }

    return 1;
}

// A fast fade skips the 45 second delay; an immediate fade also clears the caller's fade
// parameter.

int CSWCMessage::HandleServerToPlayerPlaceableUpdate_Delete()
{
    unsigned long long objId = ReadOBJECTIDClient();
    int bFast = ReadBOOL();
    int bImmediate = ReadBOOL();
    unsigned int nFadeParam = ReadDWORD(32);

    if (MessageReadOverflow(1))
        return 0;

    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
    if (!pObject)
        return 0;

    unsigned int nFadeDelay = bFast ? 0 : 45000;

    if (pObject->AsSWCPlaceable())
    {
        CSWCObject* pClientObject = static_cast<CSWCObject*>(pObject->AsSWCPlaceable());

        if (bImmediate)
        {
            pClientObject->SetFadeState(0);
            nFadeParam = 0;
        }

        pClientObject->SetDesiredFadeState(0, 1, nFadeDelay, nFadeParam);
        pClientObject->m_bPendingRemoval = 1;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerUpdate_Add()
{
    unsigned char nObjectType = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    if (CGameObject::OBJECT_TYPE_CREATURE == nObjectType)
        return HandleServerToPlayerCreatureUpdate_Add();
    if (CGameObject::OBJECT_TYPE_ITEM == nObjectType)
        return HandleServerToPlayerItemUpdate_Add(0, 0);
    if (CGameObject::OBJECT_TYPE_TRIGGER == nObjectType)
        return HandleServerToPlayerTriggerUpdate_Add();
    if (CGameObject::OBJECT_TYPE_DOOR == nObjectType)
        return HandleServerToPlayerDoorUpdate_Add();
    if (CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT == nObjectType)
        return HandleServerToPlayerAreaOfEffectUpdate_Add();
    if (CGameObject::OBJECT_TYPE_PLACEABLE == nObjectType)
        return HandleServerToPlayerPlaceableUpdate_Add();

    return 0;
}



int CSWCMessage::HandleServerToPlayerUpdate_Update()
{
    unsigned char      nObjectType = ReadBYTE(8);
    unsigned long long objId       = ReadOBJECTIDClient();
    unsigned int       nSize       = ReadDWORD(32);

    if (MessageReadOverflow(1))
        return 0;

    if (CGameObject::OBJECT_TYPE_CREATURE == nObjectType)
        return HandleServerToPlayerCreatureUpdate_Update(objId, nSize);

    if (HandleServerToPlayerGenericObjectUpdate_Update(objId, nSize) == 0)
        return 0;

    if (CGameObject::OBJECT_TYPE_ITEM == nObjectType)
    {
        CResRef resRef;

        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
        if (pItem == 0)
            return 0;

        CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

        pItem->AttachToScene(pArea->m_pScene);
        pItem->SetArea(pArea);
        return 1;
    }
    if (CGameObject::OBJECT_TYPE_TRIGGER == nObjectType)
        return HandleServerToPlayerTriggerUpdate_Update(objId, nSize);
    if (CGameObject::OBJECT_TYPE_DOOR == nObjectType)
        return HandleServerToPlayerDoorUpdate_Update(objId, nSize);
    if (CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT == nObjectType)
        return g_pAppManager->m_pClientExoApp->GetAreaOfEffectByGameObjectID(objId) != 0;
    if (CGameObject::OBJECT_TYPE_PLACEABLE == nObjectType)
        return HandleServerToPlayerPlaceableUpdate_Update(objId, nSize);

    return 0;
}

int CSWCMessage::HandleServerToPlayerUpdate_Delete()
{
    unsigned char nObjectType = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    if (CGameObject::OBJECT_TYPE_CREATURE == nObjectType)
        return HandleServerToPlayerCreatureUpdate_Delete();
    if (CGameObject::OBJECT_TYPE_ITEM == nObjectType)
        return HandleServerToPlayerItemUpdate_Delete();
    if (CGameObject::OBJECT_TYPE_TRIGGER == nObjectType)
        return HandleServerToPlayerTriggerUpdate_Delete();
    if (CGameObject::OBJECT_TYPE_DOOR == nObjectType)
        return HandleServerToPlayerDoorUpdate_Delete();
    if (CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT == nObjectType)
        return HandleServerToPlayerAreaOfEffectUpdate_Delete();
    if (CGameObject::OBJECT_TYPE_PLACEABLE == nObjectType)
        return HandleServerToPlayerPlaceableUpdate_Delete();

    return 0;
}

// Updates the client-side item use counts for quickbar items
int CSWCMessage::HandleServerToPlayerUpdate_GuiQuickbarItemUseCount()
{
    unsigned char nRecords = ReadBYTE(8);

    if (MessageReadOverflow(1))
        return 0;

    for (int i = 0; i < nRecords; i++)
    {
        ReadBYTE(8);
        ReadBYTE(8);
        unsigned long long objId = ReadOBJECTIDClient();
        unsigned char nProperty = ReadBYTE(8);
        unsigned short nUsesLeft = ReadWORD(16);

        if (MessageReadOverflow(1))
            return 0;

        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
        if (pItem)
        {
            CSWItemProperty* pProperty = pItem->GetActiveProperty(nProperty);
            if (pProperty == 0 || pProperty->m_costTableValue == IPRP_CHARGECOST_SINGLE_USE)
                pItem->SetStackSize(nUsesLeft);
            else
                pItem->SetItemPropertyUsesLeft(nProperty, nUsesLeft);
        }
    }

    return 1;
}


int CSWCMessage::HandleServerToPlayerPopUpGUIPanel(unsigned char nType)
{
    if (nType == MSG_POPUP_GUI_PANEL_SHOW)
    {
        CExoString sMessage;

        if (MessageReadOverflow(1))
            return 0;

        ReadBOOL();
        ReadBOOL();
        ReadINT(32);
        sMessage = ReadCExoString(32);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        return 1;
    }

    if (nType == MSG_POPUP_GUI_PANEL_DESTROY_DEATH)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->DestroyDeathMenuGUI();
        return 1;
    }

    if (nType == MSG_POPUP_GUI_PANEL_MESSAGE)
    {
        CExoString sMessage = ReadCExoString(32);
        return 1;
    }

    return 1;
}

// Party channel. The game-object array comes straight off the internal object rather than through
// an accessor, and the creature lookup's result is discarded.

int CSWCMessage::HandleServerToPlayerParty(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case MSG_PARTY_LIST:
        case MSG_PARTY_LIST_ADD:
        {
            CGameObjectArray* pObjectArray =
                g_pAppManager->m_pClientExoApp->m_pInternal->m_pGameObjectArray;
            int nMembers = ReadINT(32);

            for (int i = 0; i < nMembers; i++)
            {
                CGameObject* pObject;
                if (pObjectArray->GetGameObject(ReadOBJECTIDClient(), &pObject)
                    == CGameObjectArray::SUCCESS)
                    pObject->AsSWCCreature();
            }
            break;
        }

        case MSG_PARTY_LIST_REMOVE:
        {
            int nMembers = ReadINT(32);

            for (int i = 0; i < nMembers; i++)
                ReadOBJECTIDClient();
            break;
        }

        case MSG_PARTY_INVITE:
            ReadOBJECTIDClient();
            break;

        default:
            return 0;
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerModuleUpdate_Time()
{
    unsigned char nMask = ReadBYTE(8);

    unsigned char nTimeOfDayState K2_UNSET;
    unsigned int nTransitionMs K2_UNSET;
    int nCalendarHour K2_UNSET;
    int nCalendarDay K2_UNSET;
    int nCalendarMonth K2_UNSET;
    int nCalendarYear K2_UNSET;

    if (nMask & MSG_MODULE_TIME_TIME_OF_DAY)
    {
        nTimeOfDayState = ReadBYTE(8);
        if (nTimeOfDayState == 3 || nTimeOfDayState == 4)
            nTransitionMs = ReadDWORD(32);
        else
            nTransitionMs = 0;
    }
    if (nMask & MSG_MODULE_TIME_HOUR)
        nCalendarHour = ReadBYTE(8);
    if (nMask & MSG_MODULE_TIME_DAY)
        nCalendarDay = ReadBYTE(8);
    if (nMask & MSG_MODULE_TIME_MONTH)
        nCalendarMonth = ReadBYTE(8);
    if (nMask & MSG_MODULE_TIME_YEAR)
        nCalendarYear = ReadDWORD(32);

    if (MessageReadOverflow(1))
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule)
    {
        if (nMask & MSG_MODULE_TIME_TIME_OF_DAY)
            pModule->SetTimeOfDayState(nTimeOfDayState, nTransitionMs);
        if (nMask & MSG_MODULE_TIME_HOUR)
            pModule->m_nCalendarHour = nCalendarHour;
        if (nMask & MSG_MODULE_TIME_DAY)
            pModule->m_nCalendarDay = nCalendarDay;
        if (nMask & MSG_MODULE_TIME_MONTH)
            pModule->m_nCalendarMonth = nCalendarMonth;
        if (nMask & MSG_MODULE_TIME_YEAR)
            pModule->m_nCalendarYear = nCalendarYear;
    }

    return 1;
}

// GUI container channel, the C arm of Update_GuiElements. Five letters have arms: A adds an item
// through ItemUpdate_Add and stamps its grid cell, D resolves and discards, M re-stamps an existing
// item's cell, P drains a byte and reports failure, U updates the stack size and a second
// per-stack quantity. The A arm writes through its out-parameter without null-checking it, where M
// and U both check.

int CSWCMessage::HandleServerToPlayerUpdate_GuiInventory()
{
#if KOTOR1  // K1: in the intro module with the party panel up, the update goes to the equip-screen NPC
    unsigned char nOperation = ReadCHAR(8);
    if (MessageReadOverflow(1))
        return 0;
    unsigned long long oidPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    int bSwapped = 0;
    if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->GetPartyAccessPanelUp(1)) {
        int nNPC = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCurrentEquipScreenNPC();
        unsigned long long oidNPC = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nNPC, 0, 1);
        bSwapped = g_pAppManager->m_pClientExoApp->DontUseThisSetPlayerCreature(oidNPC) != 0;
    }
    if (nOperation == 'D') {
        ReadDWORD(32);
        if (!MessageReadOverflow(1)) {
            if (bSwapped)
                g_pAppManager->m_pClientExoApp->DontUseThisSetPlayerCreature(oidPlayer);
            return 1;
        }
    } else if (nOperation == 'A') {
        unsigned int nSlot = ReadDWORD(32);
        CSWCItem* pItem;
        if (!MessageReadOverflow(1) && HandleServerToPlayerItemUpdate_Add(1, &pItem) != 0) {
            pItem->m_nInventorySlot = nSlot;
            pItem->m_nReserved2 = -1;
            CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pCreature != 0 && pCreature->GetEquippedItemID(nSlot) != pItem->m_objectId)
                pCreature->EquipToInventorySlot(nSlot, pItem);
            if (bSwapped)
                g_pAppManager->m_pClientExoApp->DontUseThisSetPlayerCreature(oidPlayer);
            return 1;
        }
    }
    if (bSwapped)
        g_pAppManager->m_pClientExoApp->DontUseThisSetPlayerCreature(oidPlayer);
    return 0;
#else
    unsigned char nOperation = ReadCHAR(8);

    if (MessageReadOverflow(1))
        return 0;

    if (nOperation == 'A')
    {
        unsigned int nSlot = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;

        CSWCItem* pItem;
        if (HandleServerToPlayerItemUpdate_Add(1, &pItem) == 0)
            return 0;

        pItem->m_nInventorySlot = nSlot;
        pItem->m_nReserved2 = -1;

        CSWCCreature* pCreature;
        int nCharacter =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();

        if (nCharacter == -1)
        {
            pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        }
        else
        {
            CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;

            pCreature = static_cast<CSWSCreature*>(pServerApp->GetGameObject(
                             pServerApp->GetPartyTable()->GetNPCObject(nCharacter, 0, 1)))
                            ->GetClientCreature();
        }

        if (pCreature != 0 && pCreature->GetEquippedItemID(nSlot) != pItem->m_objectId)
            pCreature->EquipToInventorySlot(nSlot, pItem, 0);

        return 1;
    }
    if (nOperation == 'D')
    {
        ReadDWORD(32);
        return MessageReadOverflow(1) == 0;
    }

    return 0;
#endif
}

int CSWCMessage::HandleServerToPlayerUpdate_GuiContainer()
{
    unsigned char nOperation = ReadCHAR(8);

    if (MessageReadOverflow(1))
        return 0;

    switch (nOperation)
    {
        case 'A':
        {
            unsigned char nX = ReadBYTE(8);
            unsigned char nY = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            CSWCItem* pItem;
            HandleServerToPlayerItemUpdate_Add(1, &pItem);
            pItem->m_nRepositoryX = nX;
            pItem->m_nRepositoryY = nY;
            pItem->m_bInContainer = 1;
            return 1;
        }

        case 'D':
        {
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;

            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
            return 1;
        }

        case 'M':
        {
            unsigned char nX = ReadBYTE(8);
            unsigned char nY = ReadBYTE(8);
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;

            CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
            if (!pItem)
                return 0;

            pItem->m_nRepositoryX = nX;
            pItem->m_nRepositoryY = nY;
            return 1;
        }

        case 'P':
            ReadBYTE(8);
            return 0;

        case 'U':
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nStackSize = ReadDWORD(32);
            int nCost = ReadDWORD(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
            if (pItem)
            {
                pItem->SetStackSize(nStackSize);
                pItem->m_nCost = nCost;
            }
            return 1;
        }
    }

    return 0;
}

// Sets talk-table custom tokens. Both arms funnel into CTlkTable::SetCustomToken, one setting a
// single token and the other a counted run.
int CSWCMessage::HandleServerToPlayerSetCustomToken(unsigned char nType)
{
    CExoString sValue;
    int nToken;
    unsigned int nCount;

    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_CUSTOM_TOKEN_SET)
    {
        nToken = ReadINT(32);
        sValue = ReadCExoString(32);
        g_pTlkTable->SetCustomToken(nToken, sValue);
    }
    else if (nType == MSG_CUSTOM_TOKEN_SET_LIST)
    {
        nCount = ReadDWORD(32);
        while (nCount)
        {
            nToken = ReadINT(32);
            sValue = ReadCExoString(32);
            g_pTlkTable->SetCustomToken(nToken, sValue);
            nCount--;
        }
    }
    else
    {
        return 0;
    }

    if (MessageReadOverflow(1))
        return 0;

    return MessageReadUnderflow(1) == 0;
}

// Publishes a string as custom token 0, resets the talk table's per-lookup speaker and gender state
// so the next fetch re-resolves <CUSTOM0>, then appends the substituted string.
int CSWCMessage::GetSingleCustomTokenStringPart(int nToken, unsigned int nStrRef, CExoString& sOut)
{
    CExoString sPart;

    if (nToken == 0)
        return 0;

    g_pTlkTable->SetCustomToken(0, CExoString(nToken));

    sPart = g_pSWTlkTable->Fetch(nStrRef, 0, 0, 0);
    sOut = sOut + sPart;

    return 1;
}

// Server run-state notification, carrying no payload: sub-opcode 1 sends the console command
// "sModule.Run" to the server, 2 and 3 report the mode change. GetPlayModulePanel() is called for
// its side effect of realizing the panel.
int CSWCMessage::HandleServerToPlayerServerStatus(unsigned char nType)
{
    CExoString sCommand;

    if (MessageReadOverflow(1))
        return 0;

    if (MessageReadUnderflow(1))
        return 0;

    g_pAppManager->m_pClientExoApp->GetPlayModulePanel();

    if (nType == MSG_SERVER_STATUS_IDLE)
    {
        g_pAppManager->m_pClientExoApp->ServerModeChange(0);
    }
    else if (nType == MSG_SERVER_STATUS_MODULE_RUNNING)
    {
        g_pAppManager->m_pClientExoApp->ServerModeChange(2);

        if (g_pAppManager->m_pClientExoApp->GetLoadFromIFO())
        {
            g_pAppManager->m_pClientExoApp->SetLoadFromIFO(0);
        }
        else if (g_pAppManager->m_pClientExoApp->GetPlayerCharacterSet())
        {
            SendPlayerToServerLogin_LoginLocalCharacter(
                CResRef(*g_pAppManager->m_pClientExoApp->GetPlayerCharacterName()), 0);
        }
    }
    else if (nType == MSG_SERVER_STATUS_MODULE_LOADING)
    {
        g_pAppManager->m_pClientExoApp->ServerModeChange(1);

        sCommand.Format(const_cast<char*>("%c%s.%s"), 's', "Module", "Run");

        g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(
            CNetLayer::PLAYERID_SERVER, reinterpret_cast<unsigned char*>(sCommand.CStr()),
            sCommand.GetLength(), 0);
    }
    else
    {
        return 0;
    }

    return 1;
}

// Module channel. Sub-opcodes 1, 3 and 13 hand off to another handler and return its result.
int CSWCMessage::HandleServerToPlayerModule(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case MSG_MODULE_INFO:
        {
            CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();

            if (!g_bLoggedIn)
            {
                ClearReadMessage();
                return 1;
            }

            if (!pModule)
                pModule = g_pAppManager->m_pClientExoApp->CreateModule();

            pModule->UnloadModule();
            if (!pModule->LoadModule())
                return 0;

            g_pAppManager->m_pClientExoApp->InitializeInGameGUI();
            g_pAppManager->m_pClientExoApp->GetInGameGui()->ClearMapDisplays();
            return SendPlayerToServerModule_ModuleLoaded();
        }

        case MSG_MODULE_TIME:
            return HandleServerToPlayerModuleUpdate_Time();

        case MSG_MODULE_START_START_NEW_MODULE:
            return 1;

        case MSG_MODULE_DUMP_PLAYER:
            g_pAppManager->m_pClientExoApp->SetInStartNewModule(1);
            return 1;

        case MSG_MODULE_LOADING:
            g_pAppManager->m_pClientExoApp->UnloadModule();
            return 1;

        case MSG_MODULE_EXPORT_REPLY:
            HandleServerToPlayerExportReply();
            return 1;

        case MSG_MODULE_END_START_NEW_MODULE:
            return HandleServerToPlayerModule_EndStartNewModule();

        case MSG_MODULE_SAVE_GAME_STATUS:
            return HandleServerToPlayerSaveGameStatus();

        case MSG_MODULE_END_GAME:
            ReadCExoString(32);
            return 1;

        default:
            break;
    }

    return 0;
}

// Drains the timing-event bar's payload and answers from client state whether the bar should still
// show.
int CSWCMessage::HandleServerToPlayerGuiTimingEvent_Info()
{
    if (ReadBOOL())
    {
        ReadDWORD(32);
        ReadBYTE(8);
    }

    if (MessageReadOverflow(1))
        return 0;

    if (!g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bShowTimingEventBar)
        return 1;

    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (!pGui)
        return 0;

    return pGui->m_bInitialized != 0;
}

// Applies a skill rank to the creature, reading it as a signed char out of an unsigned char array.
void CSWCMessage::SetGuiSkillRank(CSWCCreature* pCreature, unsigned short nMaskA,
                                  unsigned short nMaskB, unsigned short nSkill)
{
#if KOTOR1  // K1 reads the stats pointer once, up front
    CSWCCreatureStats* pStats = pCreature->m_pStats;
    char nRank = nSkill < g_pRules->m_nNumSkills
                     ? pStats->m_pSkillRanksBase[nSkill]
                     : 0;

    if (nMaskA & nMaskB)
    {
        if (nRank <= 0)
        {
            if (nSkill < g_pRules->m_nNumSkills)
                pStats->m_pSkillRanksBase[nSkill] = 1;
        }
    }
    else
    {
        if (nRank >= 1)
        {
            if (nSkill < g_pRules->m_nNumSkills)
                pStats->m_pSkillRanksBase[nSkill] = 0;
        }
    }
#else
    char nRank = nSkill < g_pRules->m_nNumSkills
                     ? pCreature->m_pStats->m_pSkillRanksBase[nSkill]
                     : 0;

    if (nMaskA & nMaskB)
    {
        if (nRank <= 0)
        {
            if (nSkill < g_pRules->m_nNumSkills)
                pCreature->m_pStats->m_pSkillRanksBase[nSkill] = 1;
        }
    }
    else
    {
        if (nRank >= 1)
        {
            if (nSkill < g_pRules->m_nNumSkills)
                pCreature->m_pStats->m_pSkillRanksBase[nSkill] = 0;
        }
    }
#endif
}


// Game-object update fan-out. Sub-opcode 1 walks the rest of the message one tag byte at a time
// until the payload runs dry, so its inner arms break back into the loop. The appearance arm
// swallows its own overflow failure and continues, where the loop's leading guard aborts the
// handler.
int CSWCMessage::HandleServerToPlayerGameObjectUpdate(unsigned char nType)
{
#ifdef K2_FIX_ENGINE_BUGS
    // Ring buffer of the last four records (tag, object type, id, start), reported if the stream
    // desyncs.
    unsigned char      aK2Tag[4] = {0, 0, 0, 0};
    unsigned char      aK2Type[4] = {0, 0, 0, 0};
    unsigned long long aK2Id[4] = {0, 0, 0, 0};
    unsigned int       aK2Pos[4] = {0, 0, 0, 0};
    unsigned int       aK2Mask[4] = {0, 0, 0, 0};
    unsigned int       nK2Records = 0;
#endif
    if (MessageReadOverflow(1))
        return 0;

    switch (nType)
    {
        case MSG_GAME_OBJECT_UPDATE_UPDATE:
            while (MessageMoreDataToRead())
            {
#ifdef K2_FIX_ENGINE_BUGS
                unsigned int nK2Start = m_nReadMessagePos;
#endif
                unsigned char nTag = ReadBYTE(8);

                if (MessageReadOverflow(1))
                    return 0;

#ifdef K2_FIX_ENGINE_BUGS
                {
                    unsigned int nK2 = nK2Records++ & 3;
                    aK2Tag[nK2] = nTag;
                    aK2Pos[nK2] = nK2Start;
                    aK2Type[nK2] = 0;
                    aK2Id[nK2] = 0;
                    aK2Mask[nK2] = 0;
                    if (nTag == 'A' || nTag == 'D' || nTag == 'U')
                    {
                        // Sample the record's object type and id (and a 'U' record's update mask),
                        // then rewind the cursor.
                        unsigned int  nPos = m_nReadMessagePos;
                        unsigned int  nFragPos = m_nReadFragmentsPos;
                        unsigned char nBit = m_nReadBitIndex;
                        aK2Type[nK2] = ReadBYTE(8);
                        aK2Id[nK2] = ReadOBJECTIDClient();
                        if (nTag == 'U')
                            aK2Mask[nK2] = ReadDWORD(32);
                        m_nReadMessagePos = nPos;
                        m_nReadFragmentsPos = nFragPos;
                        m_nReadBitIndex = nBit;
                    }
                }
#endif
                switch (nTag)
                {
                    case 'A':
                        HandleServerToPlayerUpdate_Add();
                        break;

                    case 'D':
                        HandleServerToPlayerUpdate_Delete();
                        break;

                    case 'G':
                        HandleServerToPlayerUpdate_GuiElements();
                        break;

                    case 'I':
                        HandleServerToPlayerUpdate_PlayerInfo();
                        break;

                    case 'P':
                    {
                        unsigned char nObjectType = ReadBYTE(8);

                        if (!MessageReadOverflow(1)
                            && nObjectType == CGameObject::OBJECT_TYPE_CREATURE)
                            HandleServerToPlayerCreatureUpdate_Appearance();
                        break;
                    }

                    case 'U':
                        HandleServerToPlayerUpdate_Update();
                        break;

                    case 'W':
                        HandleServerToPlayerUpdate_WorkRemaining();
                        break;

                    default:
#ifdef K2_FIX_ENGINE_BUGS
                        k2_diag_bad_update_record(nTag, nK2Records, aK2Tag, aK2Type, aK2Id, aK2Pos,
                                                  aK2Mask, m_nReadMessagePos, m_nReadMessageSize,
                                                  m_pReadMessage);
#endif
                        return 0;
                }
            }
            return 1;

        case MSG_GAME_OBJECT_UPDATE_OBJECT_CONTROL:
        {
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;
            if (MessageReadUnderflow(1))
                return 0;

            return HandleServerToPlayerCreatureUpdate_ControlObject(objId);
        }

        case MSG_GAME_OBJECT_UPDATE_VISUAL_EFFECT:
            return HandleServerToPlayerCreatureUpdate_VisEffect();

        case MSG_GAME_OBJECT_UPDATE_STRIP_EQUIPPED_ITEMS:
        {
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;

            CSWCCreature* pCreature =
                g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
            if (!pCreature)
                return 0;

            pCreature->StripEquippedItems();
            return 1;
        }

        case MSG_GAME_OBJECT_UPDATE_DESTROY_ITEM:
            return HandleServerToPlayerItemUpdate_DestroyItem();

        case MSG_GAME_OBJECT_UPDATE_ITEM_SCRIPT_ADD:
            return HandleServerToPlayerItemScript_Add();
    }

    return 0;
}

// Adding an item reads the object id, base-item id, one appearance byte and two more only when the
// base item uses the multi-part model scheme. The appearance bytes sit behind their own overflow
// guard, so a truncated message hands an uninitialized model variation to CreateItem. Both modes
// exist twice, for an item that already exists client-side and for a freshly created one, differing
// only in whether the area and scene arguments are real.

int CSWCMessage::HandleServerToPlayerItemUpdate_Add(int nMode, CSWCItem** ppItem)
{
    unsigned long long objId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    unsigned int nBaseItem = ReadDWORD(32);

    int nModelVariation;
#ifdef K2_FIX_ENGINE_BUGS
    nModelVariation = 0;
#endif

    if (MessageReadOverflow(1) == 0)
    {
        nModelVariation = ReadBYTE(8);

        if (g_pRules->GetBaseItem(nBaseItem)->m_nModelType == 1)
        {
            ReadBYTE(8);
            ReadBYTE(8);
        }
    }

    CSWCArea*         pArea  = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();

    CGameObject* pObject;
    CSWCItem*    pItem = 0;

    if (CGameObjectArray::SUCCESS == pArray->GetGameObject(objId, &pObject))
    {
        pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        pItem->m_bInContainer = 0;

        CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
        if (pBaseItem != 0 && pBaseItem->m_nModelType == 1)
            g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    }

    if (pItem != 0 && nMode == 1)
    {
        pItem->SetArea(0);
        *ppItem = pItem;
    }
    else if (pItem != 0 && nMode == 0)
    {
        if (pItem->m_possessorObjectId == OBJECT_INVALID)
        {
            pItem->SetObjectVisibilityOnSetArea(pArea);
            pItem->AttachToScene(pArea->m_pScene);
            g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(pItem);
        }
        else
        {
            CSWCCreature* pCreature = pItem->GetEquippedBy();

            if (pCreature != 0)
            {
                if (pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON) == objId)
                    pCreature->EquipToInventorySlot(MSG_SLOT_RIGHT_WEAPON, 0, 0);
                if (pCreature->GetEquippedItemID(MSG_SLOT_LEFT_WEAPON) == objId)
                    pCreature->EquipToInventorySlot(MSG_SLOT_LEFT_WEAPON, 0, 0);
                if (pCreature->GetEquippedItemID(1) == objId)
                    pCreature->EquipToInventorySlot(1, 0, 0);
            }
            else
            {
                g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(pItem);
            }

            pItem->SetEquippedBy(0);

            CGameObject* pOwner;
            if (CGameObjectArray::SUCCESS == pArray->GetGameObject(pItem->m_possessorObjectId,
                                                                   &pOwner))
            {
                if (pOwner->AsSWCCreature() != 0)
                    SwitchWeaponEvent(0, 0, pOwner->AsSWCCreature());
            }

            pItem->SetArea(0);
            pItem->AttachToScene(pArea->m_pScene);
        }
    }
    else
    {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

        char nTextureVariation = 'M';
        if (pPlayer != 0 && pPlayer->m_pCreatureAppearance != 0)
            nTextureVariation = pPlayer->m_pCreatureAppearance->GetGenderChar();

        CSWCItem* pNewItem = CreateItem(nBaseItem, static_cast<unsigned char>(nModelVariation), nTextureVariation);
        if (pNewItem == 0)
            return 0;

        pNewItem->SetId(objId);

        if (nMode == 1)
        {
            pNewItem->AttachToScene(0);
            pNewItem->SetArea(0);
            *ppItem = pNewItem;
        }
        else
        {
            pNewItem->AttachToScene(pArea->m_pScene);
            pNewItem->SetArea(pArea);
            g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(pNewItem);
        }

        if (CGameObjectArray::SUCCESS != pArray->AddExternalObject(objId, static_cast<CGameObject*>(pNewItem), 1))
            return 0;
    }

    HandleServerToPlayerActiveItemProperties(objId);
    return 1;
}

// Re-homes an item that already exists client-side into the current area, attaching its model to the
// area's scene and stamping the area pointer. It consumes no payload: the id arrives as a parameter
// and nSize is unused.
int CSWCMessage::HandleServerToPlayerItemUpdate_Update(unsigned long long objectId,
                                                       unsigned int nSize)
{
    K2_UNUSED(nSize);
    CResRef resRef;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objectId);

    if (!pItem)
        return 0;

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;

    pItem->AttachToScene(pArea->m_pScene);
    pItem->SetArea(pArea);

    return 1;
}

// Spawns, or re-homes, a script-created item in the current area. The wire carries an object id, a
// baseitems.2da row and a model variation, plus two appearance bytes only for the multi-part model
// scheme. The overflow check after the base-item id gates only the remaining reads, so an overflowed
// message still runs the whole spawn path. An id that already resolves reuses the existing object;
// only the not-found path allocates, and an AddExternalObject failure leaks it.
int CSWCMessage::HandleServerToPlayerItemScript_Add()
{
    unsigned long long objId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    unsigned int nBaseItem = ReadDWORD(32);
    unsigned char nModelVariation;
#ifdef K2_FIX_ENGINE_BUGS
    nModelVariation = 0;
#endif

    if (!MessageReadOverflow(1))
    {
        nModelVariation = ReadBYTE(8);

        if (g_pRules->GetBaseItem(nBaseItem)->m_nModelType == 1)
        {
            ReadBYTE(8);
            ReadBYTE(8);
        }
    }

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    CGameObject* pObject;
    CSWCItem* pItem;

    if (pArray->GetGameObject(objId, &pObject) == CGameObjectArray::SUCCESS)
    {
        pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
    }
    else
    {
        pItem = CreateItem(nBaseItem, nModelVariation, 'M');

        if (!pItem)
            return 0;

        pItem->SetId(objId);
        pItem->SetArea(pArea);

        // The cast goes through CSWCObject* to pick up the sub-object adjustment; CSWCObject does
        // not derive from CGameObject, so the second hop is a plain reinterpret.
        if (pArray->AddExternalObject(objId, static_cast<CGameObject*>(static_cast<CSWCObject*>(pItem)), 1)
            != CGameObjectArray::SUCCESS)
            return 0;
    }

    pItem->m_bInContainer = 0;
    pItem->SetObjectVisibilityOnSetArea(pArea);
    pItem->AttachToScene(pArea->m_pScene);
    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(pItem);

    HandleServerToPlayerActiveItemProperties(objId);

    if (!pItem->LoadModel(pItem->m_modelResRef, CSWCAnimBase::ANIM_MODEL_BODY, 0))
        return 0;

    if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_bHasEnvMap)
        pItem->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 1, CResRef("default"));

    return 1;
}

// Removes an item from play. The wire bool picks between destroy (pull it out of the global object
// array and delete the client object) and drop (leave it in the array, detach it from the world).
// The two arms guard asymmetrically; destroy null-checks the out-parameter, drop does not.
int CSWCMessage::HandleServerToPlayerItemUpdate_Delete()
{
    unsigned long long objId = ReadOBJECTIDClient();
    int bDestroy = ReadBOOL();

    if (MessageReadOverflow(1))
        return 0;

    CGameObject* pObject;

    if (bDestroy == 1)
    {
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(objId, &pObject)
            != CGameObjectArray::SUCCESS)
            return 0;

        if (!pObject)
            return 0;

        if (pObject->AsSWCItem())
            delete static_cast<CSWCItem*>(pObject->AsSWCItem());

        return 1;
    }

    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(objId, &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());

    if (!pItem)
        return 0;

    pItem->SetArea(0);

    if (!pItem->GetEquippedBy())
        pItem->AttachToScene(0);

    return 1;
}

// Destroys an item outright: drops it from the global object array, then deletes the client object.
int CSWCMessage::HandleServerToPlayerItemUpdate_DestroyItem()
{
    unsigned long long objId = ReadOBJECTIDClient();

    if (MessageReadOverflow(1))
        return 0;

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);

    if (!pItem)
        return 0;

    CGameObject* pObject;

    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(pItem->m_objectId,
                                                                    &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    delete pItem;
    return 1;
}

// Character download. The blob is handed out as a pointer into the message buffer, so it has to be
// copied before the next read moves the cursor.
int CSWCMessage::HandleServerToPlayerCharacterDownload(unsigned char nType)
{
    CExoString sFileName;
    CExoString sFirstName;
    CExoString sLastName;
    unsigned int nSize;
    void* pData;
    int bOverwrite;

    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_CHARACTER_DOWNLOAD_REPLY)
    {
        sFirstName = ReadCExoLocStringClient();
        sLastName = ReadCExoLocStringClient();

        nSize = ReadDWORD(32);
        pData = ReadVOIDPtr(nSize);

        g_pAppManager->m_pClientExoApp->SetCharacterFile(nSize, static_cast<unsigned char*>(pData), 0);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
    }
    else if (nType == MSG_CHARACTER_DOWNLOAD_FAIL)
    {
        sFileName = ReadCExoString(32);

        bOverwrite = ReadBOOL();

        g_pAppManager->m_pClientExoApp->SetCharacterFile(
            sFileName.GetLength(),
            reinterpret_cast<unsigned char*>(sFileName.CStr()), bOverwrite);
    }
    else
    {
        return 0;
    }

    return 1;
}

// Quickbar button payload. nMode == 0 updates a single button, anything else walks the whole bar.
// Every field is discarded, so only the payload grammar survives: a leading kind byte selects how
// many bytes follow, and most kinds fall to a default that skips even the overflow check.
int CSWCMessage::HandleServerToPlayerGuiQuickbar_SetButton(unsigned char nButton, int nMode)
{
    unsigned char nLast;
    unsigned char nFirst;

    if (nMode == 0)
    {
        nLast = nButton + 1;
        nFirst = nButton;
    }
    else
    {
        nLast = 36;
        nFirst = 0;
    }

    for (unsigned char i = nFirst; i < nLast; i++)
    {
        unsigned char nKind = ReadBYTE(8);

        switch (nKind)
        {
            case MSG_QUICKBAR_BUTTON_SPELL:
                ReadBYTE(8);
                ReadDWORD(32);
                ReadBYTE(8);
                ReadBYTE(8);
                if (MessageReadOverflow(1))
                    return 0;
                break;

            case 3:
            case 4:
            case 8:
            case 10:
            case 37:
            case 42:
                ReadINT(32);
                if (MessageReadOverflow(1))
                    return 0;
                break;

            case 18:
            {
                CExoString sFirst;
                CExoString sSecond;

                sFirst = ReadCExoString(32);
                sSecond = ReadCExoString(32);

                if (MessageReadOverflow(1))
                    return 0;
                break;
            }

            case MSG_QUICKBAR_BUTTON_SPELL_LIKE_ABILITY:
                ReadDWORD(32);
                ReadBYTE(8);
                if (MessageReadOverflow(1))
                    return 0;
                break;
        }
    }

    return 1;
}

// Swaps the player's controlled object, re-points the owning player record and either follows the
// creature with the camera or defers it to ReAttachCamera. Taking control of the creature already
// controlled is a separate path that only does the camera work.
int CSWCMessage::HandleServerToPlayerCreatureUpdate_ControlObject(unsigned long long objId)
{
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

    pApp->SetCharacterChangeInProgress(0);

    CSWCCreature* pOld = pApp->GetPlayerCreature();

    pApp->SetPlayerCreature(objId);

    if (pOld)
    {
        CSWCPlayer* pPlayer = pApp->GetPlayerByGameObjectID(pOld->m_objectId);
        if (pPlayer)
            pPlayer->m_gameObjectId = objId;
    }

    CSWCCreature* pNew = pApp->GetCreatureByGameObjectID(objId);
    if (!pNew)
        return 0;

    CSWCModule* pModule = pApp->GetModule();
    if (!pModule)
        return 0;
    if (!pModule->m_pCamera)
        return 0;

    if (pOld != pNew)
    {
        pApp->GetPlayerByGameObjectID(objId);
        pNew->m_pStats->GetMaxHitPoints();
        pApp->AttachPlayerGameObjectToArea();

        if (enablemodelrequest)
        {
            pNew->m_bCameraReattachPending = 1;
            pNew->m_bCameraFindClearView = 1;
        }
        else
        {
            pModule->SetCameraFollow(pNew->m_objectId, 1);
        }

        if (pApp->GetModule()->m_pArea && (pApp->GetModule()->m_pArea->m_nAreaFlags & 8))
        {
            Vector vOrigin = { 0.0f, 0.0f, 0.0f };

            pNew->AddVisualEffect(VFX_ARD_HEAT_SHIMMER, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vOrigin);

            if (pOld)
                pOld->DeleteVisualEffect(VFX_ARD_HEAT_SHIMMER);
        }

        SendPlayerToServerMessage(MSG_MAJOR_PARTY, MSG_PARTY_LIST_REQUEST, 0, 0);
    }
    else
    {
        if (enablemodelrequest)
        {
            pOld->m_bCameraReattachPending = 1;
            pOld->m_bCameraFindClearView = 1;
        }
        else
        {
            pModule->SetCameraFollow(pOld->m_objectId, 1);
        }
    }

    return 1;
}

// Load-bar updates. When a save-game load bar exists it takes priority and the generic app-manager
// bar stays untouched until sub-opcode 3 clears the latch, and a hosted server suppresses the
// channel entirely.
int CSWCMessage::HandleServerToPlayerLoadBar(unsigned char nType)
{
    if (MessageReadOverflow(1))
        return 0;

    if (nType == MSG_LOAD_BAR_START_STALL_EVENT)
    {
        unsigned int nStep = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

        if (nStep == CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME && pApp->m_pInternal->m_pLoadGameLoadBar)
        {
            pApp->m_pInternal->m_bLoadGameLoadBarActive = 1;
            return 1;
        }

        if (g_pAppManager->m_pServerExoApp)
            return 1;

        if (nStep == CLoadGameErrorInfo::STALL_EVENT_SAVE_GAME)
            pApp->AddFeedbackMessage(FEEDBACK_STALL_EVENT_STARTED, 0, CExoString(""));

        g_pAppManager->m_pLoadGameErrorInfo->m_flag = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = nStep;
        g_pAppManager->m_pLoadGameErrorInfo->m_bStallFirstFrame = 1;
        g_pAppManager->m_pLoadGameErrorInfo->m_errorCode = 0;
        g_pAppManager->m_pLoadGameErrorInfo->m_nLoadBarPair = 0;
        return 1;
    }

    if (nType == MSG_LOAD_BAR_UPDATE_STALL_EVENT)
    {
        unsigned int nTotal = ReadDWORD(32);
        unsigned int nProgress = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->m_pInternal->m_bLoadGameLoadBarActive == 1)
            return 1;
        if (g_pAppManager->m_pServerExoApp)
            return 1;

        CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;

        pInfo->m_loadBar.m_total = nTotal;
        if (nProgress == 0xffffffff)
            return 1;

        pInfo->m_loadBar.m_progress = nProgress;
        return 1;
    }

    if (nType == MSG_LOAD_BAR_END_STALL_EVENT)
    {
        unsigned int nStep = ReadDWORD(32);
        unsigned int nLoadGameError = ReadDWORD(3);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

        if (pApp->m_pInternal->m_bLoadGameLoadBarActive == 1)
        {
            pApp->m_pInternal->m_bLoadGameLoadBarActive = 0;
            return 1;
        }

        pApp->m_pInternal->m_nLoadGameError = nLoadGameError;

        if (g_pAppManager->m_pServerExoApp)
            return 1;

        if (nStep == 2)
            pApp->AddFeedbackMessage(FEEDBACK_STALL_EVENT_FINISHED, 0, CExoString(""));

        g_pAppManager->m_pLoadGameErrorInfo->m_bStallFinished = 1;
        return 1;
    }

    return 0;
}

// Handles the player roster. Sub-opcode 1 wipes the list then reads a byte count and that many
// entry blocks, 2 reads exactly one block with no count byte, 3 reads a player id and nothing else.
// An entry block is a player id, the object that player drives, an account name, a bool saying
// whether the server resolved a character and, when it did, the character's creature id, the two
// localized name halves joined with a space, a portrait id and, for a custom portrait, its resref.
int CSWCMessage::HandleServerToPlayerPlayerList(unsigned char nType)
{
    CExoString sPlayerName;
    CExoString sUnused;
    CExoString sFirstName;
    CExoString sLastName;
    CResRef    resPortrait;
    CExoLinkedListNode* pPos;

    if (MessageReadOverflow(1))
        return 0;

    CExoLinkedList<CSWCPlayer>* pList = g_pAppManager->m_pClientExoApp->GetPlayerList();
    g_pAppManager->m_pClientExoApp->GetInGameGui();

    unsigned char nCount;
    if (nType == MSG_PLAYER_LIST_ALL)
    {
        // Sub-opcode 1 replaces the roster wholesale, so drain it first. The list is type-erased.
        while (pList->m_pInternal->m_pHead != 0)
        {
            CSWCPlayer* pOld = static_cast<CSWCPlayer*>(pList->RemoveHead());
            if (pOld != 0) {
                pOld->CSWCPlayer::~CSWCPlayer();   // a non-virtual destructor; nothing derives from it
                operator delete(pOld);
            }
        }

        nCount = ReadBYTE(8);
    }
    else if (nType == MSG_PLAYER_LIST_ADD)
    {
        nCount = 1;
    }
    else if (nType == MSG_PLAYER_LIST_DELETE)
    {
        unsigned int nPlayerId = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;

        pPos = pList->m_pInternal->m_pHead;
        if (pList->m_pInternal->m_pHead == 0)
            return 0;

        CSWCPlayer* pPlayer = static_cast<CSWCPlayer*>(pList->m_pInternal->m_pHead->m_pData);
        while (pPlayer != 0)
        {
            if (pPlayer->m_nPlayerId == nPlayerId)
            {
                pList->Remove(pPos);
                pPlayer->CSWCPlayer::~CSWCPlayer();
                operator delete(pPlayer);
                return 1;
            }
            pPlayer = static_cast<CSWCPlayer*>(pList->GetNext(pPos));
        }

        return 0;
    }
    else
    {
        return 0;
    }

    if (MessageReadOverflow(1))
        return 0;

    for (unsigned short i = 0; i < nCount; i++)
    {
        unsigned int nPlayerId = ReadDWORD(32);
        unsigned long long objectId = ReadOBJECTIDClient();
        sPlayerName = ReadCExoString(32);

        int bHasCharacter = ReadBOOL();

        unsigned long long characterId K2_UNSET;
        unsigned short nPortraitId K2_UNSET;
        if (bHasCharacter == 1)
        {
            characterId = ReadOBJECTIDClient();
            sFirstName = ReadCExoLocStringClient();
            sLastName = ReadCExoLocStringClient();

#if KOTOR1  // K1: CExoString::IsEmpty is out of line
            if (!sLastName.IsEmpty())
#else
            if (sLastName.m_sString != 0 && *sLastName.m_sString != 0)
#endif
                sFirstName = sFirstName + CExoString(" ") + sLastName;

            nPortraitId = ReadWORD(16);
            if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
                resPortrait = ReadCResRef(16);
        }

        if (MessageReadOverflow(1))
            return 0;

        CSWCPlayer* pPlayer;
        int bIsNew;
        if (g_pAppManager->m_pClientExoApp->GetPlayerByPlayerID(nPlayerId) == 0)
        {
            pPlayer = new CSWCPlayer(nPlayerId);
            bIsNew = 1;
        }
        else
        {
            pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerByPlayerID(nPlayerId);
            bIsNew = 0;
            if (pPlayer == 0)
                return 0;
        }

        pPlayer->m_gameObjectId = objectId;
        pPlayer->m_sPlayerName = sPlayerName;

        if (bHasCharacter == 1)
        {
            pPlayer->m_sCharacterName = sFirstName;
            pPlayer->m_characterObjectId = characterId;
            pPlayer->SetPrimaryPCPortraitId(nPortraitId);

            if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            {
                CSWCCreatureStats::CheckCustomPortrait(&resPortrait,
                                                       nPortraitId == MSG_PORTRAIT_ID_CUSTOM);
                pPlayer->SetPrimaryPCPortraitResRef(resPortrait);
            }
        }

        if (bIsNew)
            pList->AddHead(pPlayer);
    }

    return MessageReadUnderflow(1) == 0;
}

// Per-damage-type label strrefs, indexed by the wire slot that carries the amount; slots 0..2 share
// one strref because the physical trio is summed into a single term.
static const uint64_t s_anDamageTypeLabels[14] = {
    STRREF_DAMAGE_AMOUNT_PHYSICAL, STRREF_DAMAGE_AMOUNT_PHYSICAL, STRREF_DAMAGE_AMOUNT_PHYSICAL, STRREF_DAMAGE_AMOUNT_UNIVERSAL,
    STRREF_DAMAGE_AMOUNT_ACID, STRREF_DAMAGE_AMOUNT_COLD, STRREF_DAMAGE_AMOUNT_LIGHT_SIDE, STRREF_DAMAGE_AMOUNT_ELECTRICAL,
    STRREF_DAMAGE_AMOUNT_FIRE, STRREF_DAMAGE_AMOUNT_DARK_SIDE, STRREF_DAMAGE_AMOUNT_SONIC, STRREF_DAMAGE_AMOUNT_ION,
    STRREF_DAMAGE_AMOUNT_ENERGY, STRREF_DAMAGE_AMOUNT_POISON
};

// K1 has no combat buffer, so every combat line goes to the message buffer and its "unknown
// object" name fallback is GUI strref 0. Its records also differ in their trailing fields: type 8
// has three ints, 20 has 21, 23 has 28, 26 is type 25's object plus 9 ints, and 11 has no trailing
// feedback ids.
#if KOTOR1
#define AppendToCombatBuffer AppendToMsgBuffer
#endif

int CSWCMessage::HandleServerToPlayerCCMessage(unsigned char nType)
{
    CExoString sOut;
    CExoString sSpeaker;
    CResRef resTemplate;
    CExoString sName;

    int bRet = 0;

    switch (nType)
    {
    case 1:
    {
        unsigned long long objId = ReadOBJECTIDClient();
        unsigned int nStrRef = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;

        CExoString sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        g_pTlkTable->SetCustomToken(1, sTokenValue);

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_LABEL_VALUE, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_DAMAGE:
    {
        // A combat damage breakdown carries the object id, then 15 entries read as (bool present,
        // optional 14-bit value), -1 for absent. Entries 0..2 are the physical trio, summed by
        // their positive parts into token 0; entries 3..13 each append a label to token 0 when
        // non-negative, and entry 14 publishes alone as token 1.
        //
        // The eleven label cases repeat their guard, token publish and label fetch; hoisting the
        // shared part would publish one label instead of eleven. nFirst is a first-entry flag,
        // initially set only when the whole physical trio is absent and cleared once the first label
        // lands.
        CExoString sLabel;
        int nValues[15];

        unsigned long long objId = ReadOBJECTIDClient();

        for (int i = 0; i < 15; i++)
        {
            int bPresent = ReadBOOL();

            if (MessageReadOverflow(1))
                return 0;

            nValues[i] = -1;

            if (bPresent)
            {
                nValues[i] = ReadWORD(14);

                if (MessageReadOverflow(1))
                    return 0;
            }
        }

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        int nFirst = 0;

        g_pTlkTable->SetCustomToken(1, CExoString(nValues[14]));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DAMAGED, 0, 0, 0) + CExoString(" (");

        if (nValues[0] < 0 && nValues[1] < 0 && nValues[2] < 0)
        {
            nFirst = 1;
        }
        else
        {
            int nPhysical = 0;

            if (nValues[0] > 0)
                nPhysical += nValues[0];

            if (nValues[1] > 0)
                nPhysical += nValues[1];

            if (nValues[2] > 0)
                nPhysical += nValues[2];

            g_pTlkTable->SetCustomToken(0, CExoString(nPhysical));

            sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_PHYSICAL, 0, 0, 0);
            sOut = sOut + sLabel;
            nFirst = 0;
        }

        for (int i = 0; i < 15; i++)
        {
            switch (i)
            {
            case 3:
                if (nValues[3] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[3]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_UNIVERSAL, 0, 0, 0);
                break;
            case 4:
                if (nValues[4] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[4]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ACID, 0, 0, 0);
                break;
            case 5:
                if (nValues[5] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[5]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_COLD, 0, 0, 0);
                break;
            case 6:
                if (nValues[6] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[6]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_LIGHT_SIDE, 0, 0, 0);
                break;
            case 7:
                if (nValues[7] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[7]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ELECTRICAL, 0, 0, 0);
                break;
            case 8:
                if (nValues[8] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[8]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_FIRE, 0, 0, 0);
                break;
            case 9:
                if (nValues[9] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[9]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_DARK_SIDE, 0, 0, 0);
                break;
            case 10:
                if (nValues[10] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[10]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_SONIC, 0, 0, 0);
                break;
            case 11:
                if (nValues[11] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[11]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ION, 0, 0, 0);
                break;
            case 12:
                if (nValues[12] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[12]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ENERGY, 0, 0, 0);
                break;
            case 13:
                if (nValues[13] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[13]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_POISON, 0, 0, 0);
                break;
            default:
                continue;
            }

            if (nFirst != 0)
                sOut = sOut + sLabel;
            else
                sOut = sOut + CExoString(" ") + sLabel;

            nFirst = 0;
        }

        sOut = sOut + CExoString(")");

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }


    case CC_MESSAGE_DAMAGE_FROM_ATTACKER:
    {
        // In the combat damage breakdown with two names, object 1 publishes its display name as
        // token 0 (or the unknown-name fallback), object 2 becomes sName / token 1, and the same
        // 15-entry array as case 2 goes out with entry 14 alone as token 2. The separator and the
        // eleven labels are case 2's.
        //
        // This arm also reads a further bool and, when set, a further string it renders through a
        // second AppendToCombatBuffer, which makes it the only arm of the family that renders twice.
        // The failed name lookup publishes sSpeaker, the outer local nothing assigns, rather than the
        // fallback it just stored into sName.
        CExoString sName0;      // token 0 -- GetObjectName(first reader)
        CExoString sFallback;   // the STRREF_OTHER name, used only on a failed lookup
        CExoString sLabel;      // the per-slot label chosen by the loop below
        int nValues[15];

        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        unsigned long long objId2 = ReadOBJECTIDClient();   // -> sName / token 1

        for (int i = 0; i < 15; i++)
        {
            int bPresent = ReadBOOL();

            if (MessageReadOverflow(1))
                return 0;

            nValues[i] = -1;

            if (bPresent)
                nValues[i] = ReadWORD(14);
        }

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId2, sName))
        {
            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(1, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif

            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(1, sSpeaker);
        }

        int nFirst = 0;

        g_pTlkTable->SetCustomToken(2, CExoString(nValues[14]));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DAMAGES_FOR, 0, 0, 0) + CExoString(" (");

        if (nValues[0] < 0 && nValues[1] < 0 && nValues[2] < 0)
        {
            nFirst = 1;
        }
        else
        {
            int nPhysical = 0;

            if (nValues[0] > 0)
                nPhysical += nValues[0];

            if (nValues[1] > 0)
                nPhysical += nValues[1];

            if (nValues[2] > 0)
                nPhysical += nValues[2];

            g_pTlkTable->SetCustomToken(0, CExoString(nPhysical));

            sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_PHYSICAL, 0, 0, 0);
            sOut = sOut + sLabel;
            nFirst = 0;
        }

        for (int i = 0; i < 15; i++)
        {
            switch (i)
            {
            case 3:
                if (nValues[3] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[3]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_UNIVERSAL, 0, 0, 0);
                break;
            case 4:
                if (nValues[4] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[4]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ACID, 0, 0, 0);
                break;
            case 5:
                if (nValues[5] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[5]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_COLD, 0, 0, 0);
                break;
            case 6:
                if (nValues[6] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[6]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_LIGHT_SIDE, 0, 0, 0);
                break;
            case 7:
                if (nValues[7] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[7]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ELECTRICAL, 0, 0, 0);
                break;
            case 8:
                if (nValues[8] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[8]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_FIRE, 0, 0, 0);
                break;
            case 9:
                if (nValues[9] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[9]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_DARK_SIDE, 0, 0, 0);
                break;
            case 10:
                if (nValues[10] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[10]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_SONIC, 0, 0, 0);
                break;
            case 11:
                if (nValues[11] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[11]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ION, 0, 0, 0);
                break;
            case 12:
                if (nValues[12] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[12]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ENERGY, 0, 0, 0);
                break;
            case 13:
                if (nValues[13] < 0)
                    continue;
                g_pTlkTable->SetCustomToken(0, CExoString(nValues[13]));
                sLabel = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_POISON, 0, 0, 0);
                break;
            default:
                continue;
            }

            if (nFirst != 0)
                sOut = sOut + sLabel;
            else
                sOut = sOut + CExoString(" ") + sLabel;

            nFirst = 0;
        }

        sOut = sOut + CExoString(")");

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        int bMore = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        if (bMore)
        {
            CExoString sExtra;
            sExtra = ReadCExoString(32);

            if (MessageReadOverflow(1))
                return 0;

            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
                sExtra, kMessageTypeFeedback, 0);
        }

        bRet = 1;
        break;
    }

    case 6:
    {
        // Feat-and-roll feedback record. No sender writes this subtype, so every field is named from
        // usage. The wire opens like case 13 (two objects, a signed attack bonus, a d20 roll, a result
        // code) and then carries a feat id published under its name, a 3-bit kind, three bools, an
        // optional extra magnitude, a concealment value for result codes 16 and 17, and an optional
        // string that lands in a local nothing reads.
        //
        // The arm renders nothing. It sets ten custom tokens and assembles sOut out of up to three
        // prefix fragments, but never calls a buffer, so the string is dropped. Token 8 holds a
        // roll breakdown built only when the third bool is set.
        CExoString sSign;       // token 5 -- the literal at the front of the bonus
        CExoString sName0;      // token 0 -- GetObjectName(reader 0)
        CExoString sFeatName;   // token 1 -- the feat's name strref
        CExoString sResult;     // token 3 -- the per-result string, chosen by the switch below
        CExoString sFallback;   // the STRREF_OTHER name, used only on a failed lookup
        CExoString sConceal;    // token 8 -- the roll breakdown, built only when the third BOOL is 1

        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        unsigned long long objId2 = ReadOBJECTIDClient();   // -> sName / token 2
        char nAttackBonus = ReadCHAR(8);
        unsigned char nRoll = ReadBYTE(5);
        unsigned char nResultCode = ReadBYTE(4);
        unsigned short nFeatId = ReadWORD(16);
        unsigned char nKind = ReadBYTE(3);
        int bFirst = ReadBOOL();
        int bSecond = ReadBOOL();
        int bThird = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        unsigned int nExtra = 0;
        if (bThird == 1)
            nExtra = ReadBYTE(5);

        unsigned char nConceal = 0;
        if ((nResultCode & 0xfe) == kAttackResultTargetConcealed)
            nConceal = ReadBYTE(7);

        if (MessageReadOverflow(1))
            return 0;

        int nSum = nRoll + nAttackBonus;

        // The sign goes out as its own token, and the amount it signs appears twice, as the
        // magnitude and inside the roll breakdown.
        int nAbs;
        if (nAttackBonus < 0)
        {
            sSign = "-";
            nAbs = -nAttackBonus;
        }
        else
        {
            sSign = "+";
            nAbs = nAttackBonus;
        }

        if (bThird == 1)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nExtra));
            sConceal = g_pSWTlkTable->Fetch(STRREF_FMT_THREAT_ROLL, 0, 0, 0);
            sConceal.Format(const_cast<char*>("%s %s %d = %d"), sConceal.CStr(), sSign.CStr(), nAbs, nAbs + nExtra);
        }

        if (nFeatId != 0)
        {
            CSWFeat* pFeat = g_pRules->GetFeat(nFeatId);
            if (pFeat != 0)
                sFeatName = g_pAppManager->m_pClientExoApp->GetGUIString(pFeat->m_nNameStrRef);
        }

        // Result codes 1/2/4/5/6/8 get their own string; 16 and 17 also publish the concealment value
        // as token 0 and reset the speaker before fetching theirs.
        switch (nResultCode)
        {
        case ATTACK_RESULT_HIT_SUCCESSFUL:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HIT);
            break;
        case ATTACK_RESULT_CRITICAL_HIT:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CRITICAL_HIT);
            break;
        case ATTACK_RESULT_MISS:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MISS);
            break;
        case ATTACK_RESULT_ATTACK_RESISTED:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RESISTED);
            break;
        case ATTACK_RESULT_ATTACK_FAILED:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_FAILED);
            break;
        case ATTACK_RESULT_PARRIED:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(0);
            break;
        case kAttackResultTargetConcealed:
            g_pTlkTable->SetCustomToken(0, CExoString(nConceal));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_TARGET_CONCEALED, 0, 0, 0);
            break;
        case kAttackResultAttackerMissChance:
            g_pTlkTable->SetCustomToken(0, CExoString(nConceal));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_ATTACKER_MISS_CHANCE, 0, 0, 0);
            break;
        }

        // Both arms of the name chain publish token 2 themselves, because the failing arm has to
        // overwrite sName with the unknown-name fallback first.
        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId2, sName))
        {
            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(2, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif

            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(2, sName);
        }

        g_pTlkTable->SetCustomToken(1, sFeatName);
        g_pTlkTable->SetCustomToken(3, sResult);
        g_pTlkTable->SetCustomToken(4, CExoString(nRoll));
        g_pTlkTable->SetCustomToken(5, sSign);
        g_pTlkTable->SetCustomToken(6, CExoString(nAbs));
        g_pTlkTable->SetCustomToken(7, CExoString(nSum));

        if (bThird == 1)
            g_pTlkTable->SetCustomToken(8, sConceal);
        else
            g_pTlkTable->SetCustomToken(8, CExoString(""));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_ATTEMPTS_ON, 0, 0, 0);

        if (bSecond != 0)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SNEAK_ATTACK) + CExoString(" : ") + sOut;

        if (nKind >= 2)
        {
            CExoString sBuf;
            switch (nKind)
            {
            case 2: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_POWER_ATTACK); break;
            case 3: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_IMPROVED_POWER_ATTACK); break;
            case 5: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FLURRY); break;
            case 6: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RAPID_SHOT); break;
            }

            sOut = sBuf + CExoString(" : ") + sOut;
        }

        if (bFirst == 1)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(0) + CExoString(" : ") + sOut;

        int bTrailing = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        if (bTrailing != 0)
        {
            // Read into a local nothing consumes.
            CExoString sUnused;
            sUnused = ReadCExoString(32);

            if (MessageReadOverflow(1))
                return 0;
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_SAVING_THROW:
    {
        // The subtype picks one of three token-1 strings; the two signed chars plus the amount,
        // summed, are compared with the limit to choose the short line or the normal one. Three of
        // the six declared CExoStrings are never read.
        unsigned char nSub = ReadBYTE(8);
        ReadBYTE(8);
        unsigned char nAmount = ReadBYTE(5);
        char nChA = ReadCHAR(8);
        char nChB = ReadCHAR(8);
        unsigned char nLimit = ReadBYTE(7);
        unsigned long long objId = ReadOBJECTIDClient();

        if (MessageReadOverflow(1))
            return 0;

        CExoString sMsg;

        if (nChB != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nChB));

            sMsg = sMsg + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_BASE_SAVE, 0, 0, 0);
        }

        if (nChA != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nChA));

            sMsg = sMsg + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_MOD_SAVE, 0, 0, 0);
        }

        CExoString sUnused1;
        CExoString sSubStr;
        CExoString sUnused2;
        CExoString sCountStr;
        CExoString sObjName;
        CExoString sUnused3;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sObjName))
        {
            g_pTlkTable->SetCustomToken(0, sObjName);
        }
        else
        {
#if KOTOR1
            sObjName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sObjName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjName);
        }

        switch (nSub)
        {
        case 1: sSubStr = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FORTITUDE_SAVE); break;
        case 2: sSubStr = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_REFLEX_SAVE); break;
        case 3: sSubStr = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_WILL_SAVE); break;
        }
        g_pTlkTable->SetCustomToken(1, sSubStr);

        int nSum = nChA + nAmount + nChB;

        if (nSum < nLimit)
            sCountStr = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FAILURE);
        else
            sCountStr = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SUCCESS);

        g_pTlkTable->SetCustomToken(2, sCountStr);
        g_pTlkTable->SetCustomToken(3, CExoString(nSum));
        g_pTlkTable->SetCustomToken(4, CExoString(nAmount));
        g_pTlkTable->SetCustomToken(5, sMsg);
        g_pTlkTable->SetCustomToken(6, CExoString(nLimit));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_SAVING_THROW, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_SPELL:
    {
        // Spell display. The subtype indexes the rules' spell array: the arm publishes the spell's own
        // name strref when it is set, otherwise the client locstring (or the spell's name strref) plus
        // a fixed label. The two counters then go out as a damage-style " %d (%d + %d " summary, and
        // the last one, when non-zero, as its own line. A read overflow here reports success. sUnused
        // is never read.
        CExoString sUnused;
        CExoString sSpellName;
        unsigned long long objId = ReadOBJECTIDClient();
        unsigned long long objIdAlt = ReadOBJECTIDClient();
        int nSpell = ReadINT(32);
        int bHasLocString = ReadBOOL();
        CExoString sLocString;
        if (bHasLocString)
            sLocString = ReadCExoLocStringClient();
        int nA = ReadINT(32);
        int nB = ReadINT(32);
        ReadINT(32);
#if !KOTOR1  // K1: three trailing INTs, no count line
        int nC = ReadINT(32);
        ReadINT(32);
#endif

        bRet = 1;

        if (MessageReadOverflow(1))
            break;

        CSWSpell* pSpell = g_pRules->GetSpell(nSpell);

        if (pSpell != 0)
        {
            if (pSpell->m_nAltMessage != 0)
            {
                if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
                {
                    g_pTlkTable->SetCustomToken(0, sName);
                }
                else
                {
#if KOTOR1
                    sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                    sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                    g_pTlkTable->SetCustomToken(0, sName);
                }

                // Held in a local across the speaker reset below.
                unsigned int nStrRef = pSpell->m_nAltMessage;

                sOut = g_pSWTlkTable->Fetch(nStrRef, 0, 0, 0);
            }
            else
            {
                if (bHasLocString)
                    sSpellName = sLocString;
                else
                    sSpellName = g_pAppManager->m_pClientExoApp->GetGUIString(pSpell->m_nNameStrRef);

                if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
                {
                    g_pTlkTable->SetCustomToken(0, sName);
                }
                else
                {
#if KOTOR1
                    sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                    sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                    g_pTlkTable->SetCustomToken(0, sName);
                }

                g_pTlkTable->SetCustomToken(1, sSpellName);

                sOut = g_pSWTlkTable->Fetch(STRREF_FMT_USES, 0, 0, 0);
            }

            if (nB != 0)
            {
                CExoString sBuf;

                if (nB - nA < 0)
                    sBuf.Format(const_cast<char*>(" %d (%d - %d "), nB, nA, nA - nB);
                else
                    sBuf.Format(const_cast<char*>(" %d (%d + %d "), nB, nA, nB - nA);

#if KOTOR1
                sOut = sOut + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_FORCE_POINTS_SPENT) + sBuf
                     + g_pTlkTable->GetSimpleString(STRREF_FORCE_ADJUSTMENT) + CExoString(")");
#else
                if (nC != 0)
                {
                    sOut = sOut + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_FORCE_POINTS_SPENT) + sBuf
                         + g_pTlkTable->GetSimpleString(STRREF_FORCE_ADJUSTMENT_ALT) + CExoString(")");
                }
                else
                {
                    sOut = sOut + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_FORCE_POINTS_SPENT) + sBuf
                         + g_pTlkTable->GetSimpleString(STRREF_FORCE_ADJUSTMENT) + CExoString(")");
                }
#endif
            }

#if !KOTOR1
            if (nC != 0)
            {
                CExoString sCount;

                sCount.Format(const_cast<char*>(" %d "), nC);

                sOut = sOut + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_VITALITY_POINTS_SPENT) + sCount;
            }
#endif
        }

        g_pAppManager->m_pClientExoApp->GetObjectName(objIdAlt, sName);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 1);
        break;
    }

    case CC_MESSAGE_SKILL:
    {
        // The seven-slot skill check, as broadcast by the server's skill data message. Object 0 is
        // the skill user, object 1 the object checked against (the invalid sentinel on the self-only
        // paths, which is why the floating feedback drops out), then the skill's name strref, the d20
        // roll, the skill rank, the DC, the take-20 flag (sent but never read), the outcome code and
        // the name strref again.
        //
        // The outcome drives both the verb and the feedback. Outcome 1 is success, 0 and 3 failure
        // (3 being a failed take-20), 5 the spike/lock refused band, and 2 / 4 group with 0/3 and 1
        // respectively. sDead is never written or read.
        CExoString sSign;       // token 5 -- the literal in front of the skill modifier
        CExoString sDead;       // ctor + dtor only
        CExoString sSkill;      // token 2 -- GetGUIString(int 0); also the feedback's guard value
        CExoString sSkill2;     // token 6 -- GetGUIString(int 6)
        CExoString sVerb;       // token 1 -- the success/failure verb (UNASSIGNED on outcome 5)

        unsigned long long objId = ReadOBJECTIDClient();        // -> token 0
        unsigned long long objIdTarget = ReadOBJECTIDClient();  // -> the feedback's object
        unsigned int nSkillStrRef = ReadDWORD(32);
        unsigned char nRoll = ReadBYTE(5);
        char nSkillRank = ReadCHAR(8);
        unsigned char nDC = ReadBYTE(7);
        ReadBOOL();                                             // the take-20 flag, unread
        unsigned char nOutcome = ReadBYTE(8);
        unsigned int nSkillStrRef2 = ReadDWORD(32);

        if (MessageReadOverflow(1))
            return 0;

        sSkill = g_pAppManager->m_pClientExoApp->GetGUIString(nSkillStrRef);
        sSkill2 = g_pAppManager->m_pClientExoApp->GetGUIString(nSkillStrRef2);

        // The refused band skips the token block, because no verb was assigned and the assembled line
        // would render a default-constructed string; its feedback is the fixed refused line instead.
        // Outcomes above 5 share the 0-4 token block.
        switch (nOutcome)
        {
        case 0:
        case 2:
        case 3:
            sVerb = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FAILURE);
            break;
        case 1:
        case 4:
            sVerb = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SUCCESS);
            break;
        case 5:
            break;
        }

        if (nOutcome != 5)
        {
            int nSum = nSkillRank + nRoll;

            // The skill modifier goes out three ways, as its sign, as its magnitude, and folded
            // into the check total (token 3).
            int nAbs;
            if (nSkillRank < 0)
            {
                sSign = "-";
                nAbs = -nSkillRank;
            }
            else
            {
                sSign = "+";
                nAbs = nSkillRank;
            }

            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
            {
                g_pTlkTable->SetCustomToken(0, sName);
            }
            else
            {
#if KOTOR1
                sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sName);
            }

            g_pTlkTable->SetCustomToken(1, sVerb);
            g_pTlkTable->SetCustomToken(2, sSkill);
            g_pTlkTable->SetCustomToken(3, CExoString(nSum));
            g_pTlkTable->SetCustomToken(4, CExoString(nRoll));
            g_pTlkTable->SetCustomToken(5, sSign);
            g_pTlkTable->SetCustomToken(6, sSkill2);
            g_pTlkTable->SetCustomToken(7, CExoString(nAbs));
            g_pTlkTable->SetCustomToken(8, CExoString(nDC));

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_SKILL_CHECK, 0, 0, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sOut, kMessageTypeFeedback, 0);
        }

        // The floating feedback on the checked object reads "<skill> *<verb>*", with the emphasis
        // marks re-formatted over the verb alone when the line runs past 20 characters.
        {
            CGameObject* pTargetObject =
                g_pAppManager->m_pClientExoApp->GetGameObject(objIdTarget);

            if (pTargetObject != 0)
            {
                CSWCObject* pTarget = pTargetObject->AsSWCObject();

                if (pTarget != 0)
                {
                    bRet = 1;

                    switch (nOutcome)
                    {
                    case 0:
#if KOTOR1  // K1: Disable Mine, Recover Mine and Security; Set Mine gets no floating feedback
                        if (nSkillStrRef == STRREF_SECURITY || nSkillStrRef == STRREF_RECOVER_MINE
                                || nSkillStrRef == STRREF_DISABLE_MINE)
#else
                        if (nSkillStrRef - STRREF_DISABLE_MINE < 3 || nSkillStrRef == STRREF_SECURITY)
#endif
                        {
                            sOut.Format(const_cast<char*>("%s *%s*"), sSkill.CStr(), sVerb.CStr());

                            if (sOut.GetLength() > kFeedbackInfoMaxLength)
                                sOut.Format(const_cast<char*>("*%s*"), sVerb.CStr());

                            pTarget->SetFeedbackInfo(sOut);
                        }
                        break;

                    case 1:
                        if ((nSkillStrRef | 2) == STRREF_RECOVER_MINE)
                        {
                            sOut.Format(const_cast<char*>("%s *%s*"), sSkill.CStr(), sVerb.CStr());

                            if (sOut.GetLength() > kFeedbackInfoMaxLength)
                                sOut.Format(const_cast<char*>("*%s*"), sVerb.CStr());

                            pTarget->SetFeedbackInfo(sOut);
                        }
                        break;

                    case 3:
#if KOTOR1  // K1: Set Mine is not in the set here either
                        if (nSkillStrRef == STRREF_SECURITY || nSkillStrRef == STRREF_RECOVER_MINE
                                || nSkillStrRef == STRREF_DISABLE_MINE)
#else
                        if (nSkillStrRef - STRREF_DISABLE_MINE < 3 || nSkillStrRef == STRREF_SECURITY)
#endif
                        {
                            CExoString sAlt =
                                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SKILL_TOO_LOW);

                            sOut.Format(const_cast<char*>("%s *%s*"), sSkill.CStr(), sAlt.CStr());

                            if (sOut.GetLength() > kFeedbackInfoMaxLength)
                                sOut.Format(const_cast<char*>("*%s*"), sAlt.CStr());

                            pTarget->SetFeedbackInfo(sOut);
                        }
                        break;

                    case 5:
                        if (nSkillStrRef == STRREF_SECURITY)
                        {
                            CExoString sAlt =
                                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_IMPOSSIBLE);

                            sOut.Format(const_cast<char*>("%s *%s*"), sSkill.CStr(), sAlt.CStr());

                            if (sOut.GetLength() > kFeedbackInfoMaxLength)
                                sOut.Format(const_cast<char*>("*%s*"), sAlt.CStr());

                            pTarget->SetFeedbackInfo(sOut);
                        }
                        break;
                    }
                }
            }
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_DEATH:
    {
        // Four CExoStrings and two CResRefs are declared here; the arm reads two of them.
        CExoString sName0;      // token 0 -- GetObjectName(second reader)
        CExoString sName1;      // token 1 -- GetObjectName(first reader)
        STR_RES strTmp;
        STR_RES strTmp2;

        unsigned long long objId0 = ReadOBJECTIDClient();   // -> token 1
        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        int nValue = ReadINT(32);

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
        {
            g_pTlkTable->SetCustomToken(0, sName0);
        }
        else
        {
#if KOTOR1
            sName0 = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName0 = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName0);
        }

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId0, sName1))
        {
            g_pTlkTable->SetCustomToken(1, sName1);
        }
        else
        {
#if KOTOR1
            sName1 = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName1 = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(1, sName1);
        }

        g_pTlkTable->SetCustomToken(2, CExoString(nValue));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_KILLED_XP, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case 5:
    {
        // The CExoString-less sibling of case 6, with the same wire head (two objects, a signed
        // attack bonus, a d20 roll, a result code), then a kind byte and five bools. No sender
        // writes it either, and it renders nothing; sOut is assembled from the same prefix
        // fragments and dropped. Tokens 0..7 carry the two names, the per-result string, the bonus
        // and its sign, the magnitude, the raw sum and the value breakdown.
        STR_RES strTmp;
        CExoString sSign;       // token 4 -- the literal at the front of the value
        CExoString sName0;      // token 0 -- GetObjectName(reader 1)
        CExoString sResult;     // token 2 -- the per-result string, chosen by the switch below
        CExoString sFallback;   // the STRREF_OTHER name, used only on a failed lookup
        CExoString sConceal;    // token 7 -- the breakdown, built only when the fifth BOOL is 1

        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        unsigned long long objId2 = ReadOBJECTIDClient();   // -> sName / token 1

        if (MessageReadOverflow(1))
            return 0;

        int nHp = ReadCHAR(8);
        unsigned char nA = ReadBYTE(5);
        unsigned char nResultCode = ReadBYTE(4);
        unsigned char nKind = ReadBYTE(3);
        int bFirst = ReadBOOL();
        int bSecond = ReadBOOL();
        int bThird = ReadBOOL();
        int bFourth = ReadBOOL();
        int bFifth = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        // Not zero-initialized, unlike the identical gate in case 6.
        unsigned int nExtra K2_UNSET;
        if (bFifth == 1)
            nExtra = ReadBYTE(5);

        unsigned char nConceal = 0;
        if ((nResultCode & 0xfe) == kAttackResultTargetConcealed)
            nConceal = ReadBYTE(7);

        if (MessageReadOverflow(1))
            return 0;

        int nAbs;
        if (nHp < 0)
        {
            sSign = "-";
            nAbs = -nHp;
        }
        else
        {
            sSign = "+";
            nAbs = nHp;
        }

        if (bFifth == 1)
        {
            // nValue is re-derived from the sign string rather than from nHp.
            int nValue = (sSign == "-") ? -nAbs : nAbs;

            g_pTlkTable->SetCustomToken(0, CExoString(nExtra));
            sConceal = g_pSWTlkTable->Fetch(STRREF_FMT_THREAT_ROLL, 0, 0, 0);
            sConceal.Format(const_cast<char*>("%s %s %d = %d"), sConceal.CStr(), sSign.CStr(), nAbs,
                            nValue + nExtra);
        }

        // Result codes 1/2/3/4/8 get their own string; 16 and 17 also publish the concealment value as
        // token 0 and reset the speaker before fetching theirs.
        switch (nResultCode)
        {
        case ATTACK_RESULT_HIT_SUCCESSFUL:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HIT);
            break;
        case ATTACK_RESULT_CRITICAL_HIT:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CRITICAL_HIT);
            break;
        case ATTACK_RESULT_AUTOMATIC_HIT:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_AUTOMATIC_HIT_LOWER);
            break;
        case ATTACK_RESULT_MISS:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MISS);
            break;
        case ATTACK_RESULT_PARRIED:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(0);
            break;
        case kAttackResultTargetConcealed:
            g_pTlkTable->SetCustomToken(0, CExoString(nConceal));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_TARGET_CONCEALED, 0, 0, 0);
            break;
        case kAttackResultAttackerMissChance:
            g_pTlkTable->SetCustomToken(0, CExoString(nConceal));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_ATTACKER_MISS_CHANCE, 0, 0, 0);
            break;
        }

        // Both arms of the name chain publish token 1 themselves, because the failing arm has to
        // overwrite sName with the unknown-name fallback first.
        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId2, sName))
        {
            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(1, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif

            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(1, sName);
        }

        g_pTlkTable->SetCustomToken(2, sResult);
        g_pTlkTable->SetCustomToken(3, CExoString(nA));
        g_pTlkTable->SetCustomToken(4, sSign);
        g_pTlkTable->SetCustomToken(5, CExoString(nAbs));
        g_pTlkTable->SetCustomToken(6, CExoString(nA + nHp));

        if (bFifth == 1)
            g_pTlkTable->SetCustomToken(7, sConceal);
        else
            g_pTlkTable->SetCustomToken(7, CExoString(""));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_ATTACKS, 0, 0, 0);

        if (bFourth == 1)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(0) + CExoString(" : ") + sOut;

        if (bSecond == 1)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SNEAK_ATTACK) + CExoString(" : ") + sOut;

        if (nKind != 0)
        {
            CExoString sBuf;
            switch (nKind)
            {
            case 2: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_POWER_ATTACK); break;
            case 3: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_IMPROVED_POWER_ATTACK); break;
            case 5: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FLURRY); break;
            case 6: sBuf = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RAPID_SHOT); break;
            }

            sOut = sBuf + CExoString(" : ") + sOut;
        }

        if (bThird == 1)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(0) + CExoString(" : ") + sOut;

        if (bFirst == 1)
            sOut = g_pAppManager->m_pClientExoApp->GetGUIString(0) + CExoString(" : ") + sOut;

        int bTrailing = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        if (bTrailing != 0)
        {
            // Read into a local nothing consumes.
            CExoString sUnused;
            sUnused = ReadCExoString(32);

            if (MessageReadOverflow(1))
                return 0;
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_FORCE_RESIST:
    {
        CExoString sType;
        unsigned long long objId = ReadOBJECTIDClient();
        unsigned char nSubType = ReadBYTE(8);

        if (MessageReadOverflow(1))
            return 0;

        // The token-0 publish sits in both branches, so the two calls stay two.
        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        if (nSubType == 1)
        {
            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_FORCE_RESISTED, 0, 0, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
                sOut, kMessageTypeFeedback, 0);
        }
        else
        {
            if (nSubType == 0)
                sType = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FAILURE);
            else
                sType = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FORCE_IMMUNITY);

            g_pTlkTable->SetCustomToken(1, sType);

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_ATTEMPTS_TO_RESIST, 0, 0, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
                sOut, kMessageTypeFeedback, 0);
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_FEEDBACK:
    {
        // On the feedback message channel, a 16-bit sub-type selects a payload shape, which is
        // packed into a CSWCCMessageData and handed to the feedback UI with an empty template.
        // Sub-cases 161/164 and 162/165 have identical bodies and keep their own blocks.
        unsigned short nMsgType = ReadWORD(16);

        if (MessageReadOverflow(1))
            return 0;

        switch (nMsgType)
        {
        case FEEDBACK_SKILL_NOT_READY:
        {
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case 4:
        case 47:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int bLocString = ReadBOOL();

            if (MessageReadOverflow(1))
                return 0;

            unsigned char nItem = 0;
            if (bLocString == 1)
                sName = ReadCExoLocStringClient();
            else
                nItem = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            if (!bLocString)
                sName = g_pRules->GetBaseItem(nItem)->GetNameText();

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(0, sName);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case 5:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_OBJECT_LOCKED:
        {
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        // The object-id-only group, 37 sub-types that carry nothing but the target object.
        case FEEDBACK_PARTY_NEW_LEADER:
        case FEEDBACK_PARTY_MEMBER_KICKED:
        case FEEDBACK_PARTY_INVITATION_PENDING:
        case FEEDBACK_PARTY_INVITEE_INVITING:
        case FEEDBACK_PARTY_SENT_INVITATION:
        case FEEDBACK_PARTY_RECEIVED_INVITATION:
        case FEEDBACK_PARTY_JOINED:
        case FEEDBACK_PARTY_INVITATION_IGNORED:
        case FEEDBACK_PARTY_YOU_IGNORED_INVITATION:
        case FEEDBACK_PARTY_INVITATION_REJECTED:
        case FEEDBACK_PARTY_YOU_REJECTED_INVITATION:
        case FEEDBACK_PARTY_INVITATION_EXPIRED:
        case FEEDBACK_PARTY_MEMBER_LEFT:
        case 46:
        case 48:
        case FEEDBACK_IMMUNE_DISEASE:
        case FEEDBACK_IMMUNE_CRITICAL_HITS:
        case FEEDBACK_IMMUNE_DEATH:
        case FEEDBACK_IMMUNE_FEAR:
        case FEEDBACK_IMMUNE_KNOCKDOWN:
        case FEEDBACK_IMMUNE_PARALYSIS:
        case FEEDBACK_IMMUNE_NEGATIVE_LEVELS:
        case FEEDBACK_IMMUNE_MIND_EFFECTS:
        case FEEDBACK_IMMUNE_POISON:
        case FEEDBACK_IMMUNE_SNEAK_ATTACKS:
        case FEEDBACK_IMMUNE_SLEEP:
        case FEEDBACK_IMMUNE_DAZE:
        case FEEDBACK_IMMUNE_CONFUSION:
        case FEEDBACK_IMMUNE_STUN:
        case FEEDBACK_IMMUNE_BLINDNESS:
        case FEEDBACK_IMMUNE_DEAFNESS:
        case FEEDBACK_IMMUNE_CURSE:
        case FEEDBACK_IMMUNE_ENTANGLE:
        case FEEDBACK_IMMUNE_SILENCE:
        case FEEDBACK_IMMUNE_SLOW:
        case FEEDBACK_PARTY_INVITEE_IN_OTHER_PARTY:
        case FEEDBACK_IMMUNE_FEAR_FEAT:
        {
            unsigned long long objId = ReadOBJECTIDClient();

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_ITEM_ACQUIRED:
        case FEEDBACK_ITEM_LOST:
        {
            int bLocString = ReadBOOL();

            if (MessageReadOverflow(1))
                return 0;

            unsigned char nItem = 0;
            if (bLocString == 1)
                sName = ReadCExoLocStringClient();
            else
                nItem = ReadBYTE(8);

            if (MessageReadOverflow(1))
                return 0;

            if (!bLocString)
                sName = g_pRules->GetBaseItem(nItem)->GetNameText();

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetString(0, sName);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_DAMAGE_IMMUNITY:
        {
            STR_RES strUnused;

            unsigned long long objId = ReadOBJECTIDClient();
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_DAMAGE_RESISTED:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_DAMAGE_REDUCED:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_DAMAGE_RESISTED_REMAINING:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_DAMAGE_REDUCED_REMAINING:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_SPELL_IMMUNITY:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case 69:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case 70:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case 71:
        {
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_FLOATING_TEXT_STRREF:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_FLOATING_TEXT_STRING:
        {
            CExoString sText;
            unsigned long long objId = ReadOBJECTIDClient();
            sText = ReadCExoString(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(0, sText);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_ALIGNMENT_SHIFT:
        {
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_ATTRIBUTE_REDUCED:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nA = ReadINT(32);
            int nB = ReadINT(32);
            float fValue = ReadFLOAT(1.0f, 32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);
            pData->SetFloat(0, fValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_EXPERIENCE_RECEIVED:
        {
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_CREDITS_ACQUIRED:
        case FEEDBACK_CREDITS_LOST:
#if !KOTOR1  // K1: no components / chemicals feedback
        case FEEDBACK_COMPONENTS_ACQUIRED:
        case FEEDBACK_CHEMICALS_ACQUIRED:
#endif
        {
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_HEALED:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_SCRIPT_DEBUG_MESSAGE:
        {
            CExoString sNameA;
            CExoString sNameB;

            unsigned long long objId = ReadOBJECTIDClient();
            sNameA = ReadCExoString(32);
            sNameB = ReadCExoString(32);
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(0, sNameA);
            pData->SetString(1, sNameB);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_ACTION_LIST_OVERFLOW:
        case FEEDBACK_ACTION_LIST_ERASED:
        {
            CExoString sText;

            unsigned long long objId = ReadOBJECTIDClient();
            sText = ReadCExoString(32);
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(1, sText);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_EFFECT_LIST_OVERFLOW:
        case FEEDBACK_EFFECT_LIST_ERASED:
        {
            CExoString sText;

            unsigned long long objId = ReadOBJECTIDClient();
            sText = ReadCExoString(32);
            int nA = ReadINT(32);
            int nB = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(1, sText);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_AI_UPDATE_TIME_OVERFLOW:
        {
            CExoString sTextA;
            CExoString sTextB;

            unsigned long long objId = ReadOBJECTIDClient();
            sTextA = ReadCExoString(32);
            int nA = ReadINT(32);
            int nB = ReadINT(32);
            sTextB = ReadCExoString(32);
            int nC = ReadINT(32);
            int nD = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetString(1, sTextA);
            pData->SetString(2, sTextB);
            pData->SetInteger(0, nA);
            pData->SetInteger(1, nB);
            pData->SetInteger(2, nC);
            pData->SetInteger(3, nD);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_XP_GAINED:
        case FEEDBACK_XP_LOST:
        {
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_SCRIPT_MESSAGE:
        {
            CExoString sText;
            sText = ReadCExoString(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetString(0, sText);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        // The only sub-case without an overflow re-test; the value goes straight into the record.
        case 222:
        {
            int nValue = ReadINT(32);

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        case FEEDBACK_FORCE_POINTS_REGAINED:
        {
            unsigned long long objId = ReadOBJECTIDClient();
            int nValue = ReadINT(32);

            if (MessageReadOverflow(1))
                return 0;

            CSWCCMessageData* pData = new CSWCCMessageData;
            pData->SetObjectID(0, objId);
            pData->SetInteger(0, nValue);

            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, pData, CExoString(""));
            break;
        }

        default:
        {
            g_pAppManager->m_pClientExoApp->AddFeedbackMessage(nMsgType, 0, CExoString(""));
            break;
        }
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_FORCE_POINT_USAGE:
    {
        CExoString sText = ReadCExoString(32);
        int nA = ReadINT(32);
        int nB = ReadINT(32);
        ReadINT(32);

        if (nB != 0)
        {
            CExoString sBuf;

            if (nB - nA < 0)
                sBuf.Format(const_cast<char*>(" %d (%d - %d "), nB, nA, nA - nB);
            else
                sBuf.Format(const_cast<char*>(" %d (%d + %d "), nB, nA, nB - nA);

            sOut = g_pTlkTable->GetSimpleString(STRREF_FORCE_POINTS_SPENT) + sBuf
                 + g_pTlkTable->GetSimpleString(STRREF_FORCE_ADJUSTMENT) + CExoString(")");

            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
                sOut, kMessageTypeFeedback, 0);
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_ATTACK_ROLL:
    {
        // Attack roll, as written by the touch attack command: object 0 is the attacker, object 1 the
        // target, then the signed attack bonus, the d20 roll, the result code, a flag marking the
        // ranged variant and an optional concealment byte.
        //
        // Both arms of the name chain look up object 0's name for token 0 (falling back to the
        // unknown string) and publish sName as token 2, because the failing arm has to overwrite
        // sName with that fallback first.
        CExoString sSign;       // token 5 -- the literal at the front of the modifier
        CExoString sName0;      // token 0 -- GetObjectName(reader 0)
        CExoString sResult;     // token 3 -- the per-result string, chosen by the switch below
        CExoString sVerb;       // token 1 -- the melee/ranged verb
        CExoString sFallback;   // the STRREF_OTHER name, used only on a failed lookup

        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        unsigned long long objId2 = ReadOBJECTIDClient();   // -> sName / token 2
        char nAttackBonus = ReadCHAR(8);
        unsigned char nRoll = ReadBYTE(5);
        unsigned char nResult = ReadBYTE(4);
        int bRangedAttack = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        // Declared between the two overflow checks.
        unsigned int nConcealment = 0;
        int bHasConcealment = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        if (bHasConcealment == 1)
            nConcealment = ReadBYTE(7);

        if (bRangedAttack == 1)
            sVerb = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RANGED_TOUCH_ATTACK);
        else
            sVerb = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MELEE_TOUCH_ATTACK);

        // Result codes 1 / 2 / 4 get their own string. 16 and 17 also publish the concealment value
        // as token 0 and reset the speaker before fetching theirs.
        switch (nResult)
        {
        case 1:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HIT);
            break;
        case 2:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CRITICAL_HIT);
            break;
        case 4:
            sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MISS);
            break;
        case 16:
            g_pTlkTable->SetCustomToken(0, CExoString(nConcealment));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_TARGET_CONCEALED, 0, 0, 0);
            break;
        case 17:
            g_pTlkTable->SetCustomToken(0, CExoString(nConcealment));
            sResult = g_pSWTlkTable->Fetch(STRREF_FMT_ATTACKER_MISS_CHANCE, 0, 0, 0);
            break;
        }

        int nSum = nRoll + nAttackBonus;

        // The sign goes out as its own token, and the amount it signs goes out twice, as the signed
        // magnitude (reusing nSum) and as the running total.
        int nAbs;
        if (nAttackBonus < 0)
        {
            sSign = "-";
            nAbs = -nAttackBonus;
        }
        else
        {
            sSign = "+";
            nAbs = nAttackBonus;
        }

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId2, sName))
        {
            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(2, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif

            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName0))
            {
                g_pTlkTable->SetCustomToken(0, sName0);
            }
            else
            {
#if KOTOR1
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sFallback = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sFallback);
            }

            g_pTlkTable->SetCustomToken(2, sName);
        }

        g_pTlkTable->SetCustomToken(1, sVerb);
        g_pTlkTable->SetCustomToken(3, sResult);
        g_pTlkTable->SetCustomToken(4, CExoString(nRoll));
        g_pTlkTable->SetCustomToken(5, sSign);
        g_pTlkTable->SetCustomToken(6, CExoString(nAbs));
        g_pTlkTable->SetCustomToken(7, CExoString(nSum));
        g_pTlkTable->SetCustomToken(8, CExoString(""));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_ATTEMPTS_ON, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    // 3, 5, 11, 18, 20 and 23 follow in later arms.
    case 14:
    {
        CExoString sSign;

        unsigned long long objId = ReadOBJECTIDClient();
        unsigned char nA = ReadBYTE(5);
        char nB = ReadCHAR(8);

        if (MessageReadOverflow(1))
            return 0;

        int nSum = nB + nA;

        // The sign string is the literal at the front of token 3; the amount it signs goes out twice,
        // as the signed sum (tokens 1 and 5) and as the magnitude (token 4).
        int nAbs;
        if (nB < 0)
        {
            sSign = "-";
            nAbs = -nB;
        }
        else
        {
            sSign = "+";
            nAbs = nB;
        }

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        g_pTlkTable->SetCustomToken(1, CExoString(nSum));
        g_pTlkTable->SetCustomToken(2, CExoString(nA));
        g_pTlkTable->SetCustomToken(3, sSign);
        g_pTlkTable->SetCustomToken(4, CExoString(nAbs));
        g_pTlkTable->SetCustomToken(5, CExoString(nSum));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_INITIATIVE_ROLL, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_DISPEL:
    {
        // A named spell list is "<strref 0> : <object name> : " followed by every spell's name,
        // comma separated. The count is a byte and each spell id a 9-bit value indexing the rules'
        // spell array; the name comes off the spell's own name strref.
        unsigned long long objId = ReadOBJECTIDClient();
        unsigned char nSpellCount = ReadBYTE(8);

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        sOut = g_pSWTlkTable->Fetch(0, 0, 0, 0) + CExoString(" : ") + sName
             + CExoString(" : ");

        for (unsigned char i = 0; i < nSpellCount; i++)
        {
            unsigned short nSpellId = ReadWORD(9);

            if (MessageReadOverflow(1))
                return 0;

            if (i != 0)
                sOut = sOut + CExoString(", ");

            CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(pSpell->m_nNameStrRef);
        }

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_STEALTH_DETECTION:
    {
        // Spot detection, as broadcast by the server's stealth detection data. Object 0 is the
        // spotter, object 1 the creature being spotted, then a bool saying there is data and ten
        // 16-bit terms: spot roll, hide roll, spot rank, hide rank, self stationary, target
        // stationary, distance, arc, move and combat.
        //
        // Three lines go out through AppendToMsgBuffer: the summary, after the two names and the two
        // totals, then one detail line per side, the spotter's then the hider's, whose trailing
        // "+ <label>" terms are the non-zero modifiers. The spotter's total covers terms
        // 1/3/5/7/8/9/10 and the hider's terms 2/4/6.
        //
        // The hide line's single term is guarded by the self-stationary slot but renders the
        // target-stationary value, and it reuses the spot line's stationary label.
        CExoString sSpotter;   // object 0's name -- token 0 of both spot lines
        CExoString sTarget;    // object 1's name -- token 1 of the summary, token 0 of the hide line

        unsigned long long objSpotter = ReadOBJECTIDClient();
        unsigned long long objTarget = ReadOBJECTIDClient();
        int bHasData = ReadBOOL();
        short nSpotRoll = ReadSHORT(16);
        short nHideRoll = ReadSHORT(16);
        short nSpotRank = ReadSHORT(16);
        short nHideRank = ReadSHORT(16);
        short nSelfStationary = ReadSHORT(16);
        short nTargetStationary = ReadSHORT(16);
        short nDistance = ReadSHORT(16);
        short nArc = ReadSHORT(16);
        short nMove = ReadSHORT(16);
        short nCombat = ReadSHORT(16);

        if (MessageReadOverflow(1))
            return 0;

        CExoString sUnused;   // ctor + dtor only

        g_pAppManager->m_pClientExoApp->GetObjectName(objSpotter, sSpotter);
        g_pAppManager->m_pClientExoApp->GetObjectName(objTarget, sTarget);

        if (bHasData != 0)
        {
            g_pTlkTable->SetCustomToken(0, sSpotter);
            g_pTlkTable->SetCustomToken(1, sTarget);

            short nSpotTotal =
                nSpotRoll + nSpotRank + nSelfStationary + nDistance + nArc + nMove + nCombat;
            g_pTlkTable->SetCustomToken(2, CExoString(nSpotTotal));

            short nHideTotal = nHideRoll + nHideRank + nTargetStationary;
            g_pTlkTable->SetCustomToken(3, CExoString(nHideTotal));

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DETECTS_STEALTH, 0, 0, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sOut, kMessageTypeFeedback, 1);

            // The spotter's detail line carries the name, total, roll, rank, then every non-zero
            // modifier.
            g_pTlkTable->SetCustomToken(0, sSpotter);
            g_pTlkTable->SetCustomToken(1, CExoString(nSpotTotal));
            g_pTlkTable->SetCustomToken(2, CExoString(nSpotRoll));
            g_pTlkTable->SetCustomToken(3, CExoString(nSpotRank));

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_AWARENESS_BREAKDOWN, 0, 0, 0);

            if (nSelfStationary != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nSelfStationary));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_STANDING_STILL_MODIFIER, 0, 0, 0);
            }

            if (nDistance != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nDistance));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_DISTANCE_MODIFIER, 0, 0, 0);
            }

            if (nArc != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nArc));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_REAR_ARC_MODIFIER, 0, 0, 0);
            }

            if (nMove != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nMove));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_RUN_MODIFIER, 0, 0, 0);
            }

            if (nCombat != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nCombat));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_COMBAT_MODIFIER, 0, 0, 0);
            }

            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sOut, kMessageTypeFeedback, 0);

            // The hider's detail line.
            g_pTlkTable->SetCustomToken(0, sTarget);
            g_pTlkTable->SetCustomToken(1, CExoString(nHideTotal));
            g_pTlkTable->SetCustomToken(2, CExoString(nHideRoll));
            g_pTlkTable->SetCustomToken(3, CExoString(nHideRank));

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_STEALTH_BREAKDOWN, 0, 0, 0);

            // The guard reads nSelfStationary while the value published is nTargetStationary.
            if (nSelfStationary != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nTargetStationary));

                sOut = sOut + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_STANDING_STILL_MODIFIER, 0, 0, 0);
            }

            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
                sOut, kMessageTypeFeedback, 0);
        }

        bRet = 1;
        break;
    }

    case CC_MESSAGE_FORCE_RESISTANCE_ROLL:
    {
        // Two object readers and three bytes; the names go out at tokens 0 and 3, token 0 being the
        // second reader.
        unsigned long long objId0 = ReadOBJECTIDClient();   // -> token 3
        unsigned long long objId1 = ReadOBJECTIDClient();   // -> token 0
        unsigned char nA = ReadBYTE(8);
        unsigned char nB = ReadBYTE(8);
        unsigned char nC = ReadBYTE(8);

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        g_pTlkTable->SetCustomToken(1, CExoString(nA));
        g_pTlkTable->SetCustomToken(2, CExoString(nB));

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId0, sName))
        {
            g_pTlkTable->SetCustomToken(3, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(3, sName);
        }

        g_pTlkTable->SetCustomToken(4, CExoString(nC));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_FORCE_RESIST_ROLL, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_ATTACK_SUMMARY:
    {
        // Attack summary, the reader for the server's attack-breakdown writer (two object ids, eight
        // 32-bit numbers, a 16-bit feat id and three flag bytes). The message renders as
        // "<attacker> <verb> <target> ...": a chain of sOut fragments, each guarded by its own flag
        // byte, with the lead-in verb picked from the attack type. Both selector pairs below are
        // if/else because each arm does its own assignment.
        CExoString sTokenValue;

        unsigned long long objId0 = ReadOBJECTIDClient();  // attacker -> token 0
        unsigned long long objId1 = ReadOBJECTIDClient();  // target -> token 2, then token 0 again
        int nAttackType = ReadINT(32);                   // verb selector; 8 and 9 get their own
        unsigned short nFeatId = ReadWORD(16);           // a feat's name strref
        int nDmg1 = ReadINT(32);                         // -> token 1
        int nDmg2 = ReadINT(32);                         // -> token 2
        int nDmg3 = ReadINT(32);                         // -> token 3, clamped at 0
        int bFlagA = ReadINT(32);
        int bFlagB = ReadINT(32);
        int bFlagC = ReadINT(32);
        int nKind = ReadINT(32);                         // picks the trailing template string
        int bFlag1 = ReadBOOL();
        int bFlag2 = ReadBOOL();
        int bFlag3 = ReadBOOL();

        if (MessageReadOverflow(1))
            return 0;

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId0, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        if (nAttackType >= 1 && nAttackType <= 3)
            sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SUCCEEDS);
        else
            sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FAILS);

        g_pTlkTable->SetCustomToken(1, sTokenValue);

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName))
        {
            g_pTlkTable->SetCustomToken(2, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(2, sName);
        }

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_WITH_ATTACK_ON, 0, 0, 0) + CExoString(". ");

        if (nFeatId != 0)
        {
            g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                GetGUIString(g_pRules->GetFeat(nFeatId)->m_nNameStrRef));

            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_USED, 0, 0, 0) + CExoString(". ");
        }

        if (nAttackType >= 1 && nAttackType <= 3)
            sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HIT_CAPITALIZED);
        else
            sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MISS_CAPITALIZED);

        g_pTlkTable->SetCustomToken(0, sTokenValue);

        g_pTlkTable->SetCustomToken(1, CExoString(nDmg1));
        g_pTlkTable->SetCustomToken(2, CExoString(nDmg2));
        g_pTlkTable->SetCustomToken(3, CExoString(nDmg3 > 0 ? nDmg3 : 0));

        sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_ATTACK_VS_DEFENSE, 0, 0, 0);

        if (bFlagA != 0)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_CRITICAL_HIT_BANG, 0, 0, 0) + CExoString(" ");
        }

        if (bFlagB != 0)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_SNEAK_ATTACK_BANG, 0, 0, 0) + CExoString(" ");
        }

        if (bFlagC != 0)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_DEATH_BLOW, 0, 0, 0) + CExoString(" ");
        }

        if (bFlag2 != 0)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_AUTOMATIC_HIT, 0, 0, 0) + CExoString(" ");
        }
        else if (bFlag3 != 0)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_AUTOMATIC_MISS, 0, 0, 0) + CExoString(" ");
        }

        if (nAttackType == 9)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_RETURNED, 0, 0, 0) + CExoString(" ");
        }
        else if (nAttackType == 8)
        {
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_DEFLECTED, 0, 0, 0) + CExoString(" ");
        }

        if (nKind != 0)
        {
            if (g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName))
            {
                g_pTlkTable->SetCustomToken(0, sName);
            }
            else
            {
#if KOTOR1
                sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
                sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
                g_pTlkTable->SetCustomToken(0, sName);
            }

            // The 11/12/13/17 slots fall straight through to the switch's exit, leaving the template
            // string at 0 so GetGUIString(0) is published anyway. Cases 14/15/18/19 and the bFlag1
            // pairs differ only in which constant the zero flag picks.
            uint64_t nStrRef = 0;

            switch (nKind)
            {
            case 1:
#if !KOTOR1  // K1: kinds 1..10 only
            case 16:
#endif
                     nStrRef = bFlag1 == 0 ? STRREF_IS_CONFUSED : STRREF_IS_NOT_CONFUSED; break;
            case 2:  nStrRef = bFlag1 == 0 ? STRREF_IS_FRIGHTENED : STRREF_IS_NOT_FRIGHTENED; break;
            case 3:  nStrRef = bFlag1 == 0 ? STRREF_IS_DROID_STUNNED : STRREF_IS_NOT_DROID_STUNNED; break;
            case 4:  nStrRef = bFlag1 == 0 ? STRREF_IS_STUNNED : STRREF_IS_NOT_STUNNED; break;
            case 5:  nStrRef = bFlag1 == 0 ? STRREF_IS_HELD : STRREF_IS_NOT_HELD; break;
            case 6:  nStrRef = bFlag1 == 0 ? STRREF_IS_SLEEPING : STRREF_IS_NOT_SLEEPING; break;
            case 7:  nStrRef = bFlag1 == 0 ? STRREF_IS_CHOKED : STRREF_IS_NOT_CHOKED; break;
            case 8:  nStrRef = bFlag1 == 0 ? STRREF_IS_HORRIFIED : STRREF_IS_NOT_HORRIFIED; break;
            case 9:  nStrRef = bFlag1 == 0 ? STRREF_IS_FORCE_PUSHED : STRREF_IS_NOT_FORCE_PUSHED; break;
            case 10: nStrRef = bFlag1 == 0 ? STRREF_IS_IN_WHIRLWIND : STRREF_IS_NOT_IN_WHIRLWIND; break;
#if !KOTOR1
            case 14: nStrRef = bFlag1 == 0 ? STRREF_IS_KNOCKED_DOWN : STRREF_IS_NOT_KNOCKED_DOWN; break;
            case 15: nStrRef = bFlag1 == 0 ? STRREF_IS_BEING_CRUSHED : STRREF_IS_NOT_BEING_CRUSHED; break;
            case 18: nStrRef = bFlag1 == 0 ? STRREF_IS_MIND_TRICKED : STRREF_IS_NOT_MIND_TRICKED; break;
            case 19: nStrRef = bFlag1 == 0 ? STRREF_IS_SCRAMBLED : STRREF_IS_NOT_SCRAMBLED; break;
#endif
            }

            sTokenValue = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
            g_pTlkTable->SetCustomToken(1, sTokenValue);

            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_NAME_STATUS, 0, 0, 0);
        }

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 1);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_MINE_DETECTION:
    {
        // Two object names and three shorts, with the same payload published twice: once to the msg
        // buffer with the flag set and once clear. Both GetObjectName results are discarded, and
        // there is no unknown-name fallback here.
        CExoString sName0;
        CExoString sName1;

        unsigned long long objId0 = ReadOBJECTIDClient();
        unsigned long long objId1 = ReadOBJECTIDClient();
        short nA = ReadSHORT(16);
        short nB = ReadSHORT(16);
        short nC = ReadSHORT(16);

        if (MessageReadOverflow(1))
            return 0;

        CExoString sMsg;

        g_pAppManager->m_pClientExoApp->GetObjectName(objId0, sName0);
        g_pAppManager->m_pClientExoApp->GetObjectName(objId1, sName1);

        short nSum = nB + nA;

        g_pTlkTable->SetCustomToken(0, sName0);
        g_pTlkTable->SetCustomToken(1, sName1);
        g_pTlkTable->SetCustomToken(2, CExoString(nSum));
        g_pTlkTable->SetCustomToken(3, CExoString(nC));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DETECTS_VS_DC, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
            sOut, kMessageTypeFeedback, 1);

        g_pTlkTable->SetCustomToken(0, sName0);
        g_pTlkTable->SetCustomToken(1, CExoString(nSum));
        g_pTlkTable->SetCustomToken(2, CExoString(nA));
        g_pTlkTable->SetCustomToken(3, CExoString(nB));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_AWARENESS_BREAKDOWN, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(
            sOut, kMessageTypeFeedback, 0);

        bRet = 1;
        break;
    }

    case CC_MESSAGE_ATTACK_BREAKDOWN:
    {
        // Attack roll breakdown, the first of the two 31-int wire records, shipped by the server's
        // attack breakdown message. The field names come from the combat sub-attack the server fills it
        // from:
        //   0      the d20 roll + the to-hit bonus -- the headline number
        //   1      m_nAttackRoll (the raw d20)
        //   2      m_nAttackModifier                          (read, dropped)
        //   3/4    m_nStrAttackMod / m_nDexAttackMod -- the ability pair, one line
        //   5      m_nFeatId         (feat name, with 6 below as its value)
        //   6      m_nSpecialAttackBonus
        //   7      m_bOffhandAttack -- selects the headline verb
        //   8      m_nLightOffhandBonus
        //   9      m_nTwoWeaponFeatBonus -- added to 18's main-hand penalty
        //   10     m_nTwoWeaponFeat                         (read, dropped)
        //   11/12  m_nDuelingBonus / m_nDuelingFeat (feat name + bonus)
        //   13     m_nGoodAimBonus
        //   14     m_nOffensiveTrainingBonus
        //   15     m_nWeaponFocusBonus
        //   16     m_nEffectAttackBonus
        //   17     m_nBaseAttackBonus
        //   18     m_nTwoWeaponPenalty
        //   19/20  m_bAutomaticHit / m_bAutomaticMiss -- the two-way short-form selector, see below
        //   21     m_nBattleMeditationBonus
        //   22/23  m_nTargetingFeat / m_nTargetingBonus
        //   24/25  m_nCrushOppositionSpell / m_nCrushOppositionPenalty
        //   26/27  m_nInspireFollowersSpell / m_nInspireFollowersBonus
        //   28     m_nPCCharismaBonus
        //   29/30  m_nSaberFormId / m_nSaberFormBonus
        //
        // 19/20 gate the whole arm: if either is non-zero the arm appends one " " + label term and
        // stops, leaving the else branch to render the detailed breakdown. Two of the 31 reads are
        // dropped, indices 2 and 10, and both are load-bearing because each read advances the wire
        // cursor.
        CExoString sAttackLabel;    // the dual-wield attack label when the dual-wield flag is set

        int nAttackRollTotal = ReadINT(32);
        int nAttackRoll = ReadINT(32);
        ReadINT(32);
        int nStrAttackMod = ReadINT(32);
        int nDexAttackMod = ReadINT(32);
        int nFeatId = ReadINT(32);
        int nSpecialAttackBonus = ReadINT(32);
        int bDualWield = ReadINT(32);
        int nDoubleWeapon = ReadINT(32);
        int nOffhandByte = ReadINT(32);
        ReadINT(32);
        int nDuelingBonus = ReadINT(32);
        int nDuelingFeat = ReadINT(32);
        int nGoodAim = ReadINT(32);
        int nOffenseTraining = ReadINT(32);
        int nWeaponFocus = ReadINT(32);
        int nEffectAttackBonus = ReadINT(32);
        int nBaseAttackBonus = ReadINT(32);
        int nMainHandPenalty = ReadINT(32);
        int bAutomaticHit = ReadINT(32);
        int bAutomaticMiss = ReadINT(32);
#if !KOTOR1  // K1: the record ends at index 20 (21 INTs)
        int nBattleMeditation = ReadINT(32);
        int nTargetingFeat = ReadINT(32);
        int nTargetingBonus = ReadINT(32);
        int nCrushSpell = ReadINT(32);
        int nCrushOpposition = ReadINT(32);
        int nInspireSpell = ReadINT(32);
        int nInspireFollowers = ReadINT(32);
        int nPCCharismaBonus = ReadINT(32);
        int nJediDefenseFeat = ReadINT(32);
        int nJediDefense = ReadINT(32);
#endif

        sAttackLabel = g_pSWTlkTable->Fetch(bDualWield != 0 ? STRREF_OFFHAND : STRREF_MAINHAND, 0,
                                            0, 0);
        g_pTlkTable->SetCustomToken(0, sAttackLabel);
        g_pTlkTable->SetCustomToken(1, CExoString(nAttackRollTotal));

        sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_ATTACK_BREAKDOWN, 0, 0, 0);

        g_pTlkTable->SetCustomToken(0, CExoString(nAttackRoll));

        sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_ROLL, 0, 0, 0);

        if (bAutomaticHit != 0)
        {
            sOut = sOut + CExoString(" ") + g_pSWTlkTable->Fetch(STRREF_AUTOMATIC_HIT, 0, 0, 0);
        }
        else if (bAutomaticMiss != 0)
        {
            sOut = sOut + CExoString(" ") + g_pSWTlkTable->Fetch(STRREF_AUTOMATIC_MISS, 0, 0, 0);
        }
        else
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nBaseAttackBonus));

            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_BASE_ATTACK, 0, 0, 0);

            if (nMainHandPenalty != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nMainHandPenalty + nOffhandByte));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_DUAL_WIELD_PENALTY, 0, 0, 0);
            }

            if (nDoubleWeapon != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nDoubleWeapon));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_SMALL_OFFHAND_BONUS, 0, 0, 0);
            }

            if (nFeatId != 0 && nSpecialAttackBonus != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetFeat(static_cast<unsigned short>(nFeatId))->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nSpecialAttackBonus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }

            if (nDuelingBonus != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetFeat(static_cast<unsigned short>(nDuelingFeat))->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nDuelingBonus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }

            if (nGoodAim != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nGoodAim));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_CLOSE_RANGED_BONUS, 0, 0, 0);
            }

            if (nOffenseTraining != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nOffenseTraining));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_MELEE_ON_RANGED, 0, 0, 0);
            }

#if !KOTOR1
            if (nBattleMeditation != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nBattleMeditation));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_DUAL_STRIKE, 0, 0, 0);
            }

            if (nTargetingBonus != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetFeat(static_cast<unsigned short>(nTargetingFeat))->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nTargetingBonus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }

            if (nCrushOpposition != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetSpell(nCrushSpell)->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nCrushOpposition));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }

            if (nInspireFollowers != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetSpell(nInspireSpell)->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nInspireFollowers));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }

            if (nPCCharismaBonus != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nPCCharismaBonus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLAYER_CHARISMA_MODIFIER, 0, 0, 0);
            }

            if (nJediDefense != 0)
            {
                g_pTlkTable->SetCustomToken(0, g_pAppManager->m_pClientExoApp->
                    GetGUIString(g_pRules->GetSpell(nJediDefenseFeat)->m_nNameStrRef));
                g_pTlkTable->SetCustomToken(1, CExoString(nJediDefense));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
            }
#endif

            CExoString sTmpAttackMod;   // ctor + dtor only (same dead family as case 4's strTmp)
            int nAttackMod = nDexAttackMod != 0 ? nDexAttackMod : nStrAttackMod;

            if (nAttackMod != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nAttackMod));


                sOut = sOut + g_pSWTlkTable->Fetch(nDexAttackMod != 0 ? STRREF_FMT_DEXTERITY_MOD : STRREF_FMT_STRENGTH_MOD, 0, 0, 0);
            }

            if (nWeaponFocus != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nWeaponFocus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_WEAPON_FOCUS_BONUS, 0, 0, 0);
            }

            if (nEffectAttackBonus != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nEffectAttackBonus));

                sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_EFFECT_BONUS, 0, 0, 0);
            }
        }

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);
        break;
    }

    case CC_MESSAGE_THREAT_BREAKDOWN:
    {
        int nTok0 = ReadINT(32);
        int nTok1 = ReadINT(32);
        int nHaveData = ReadINT(32);
        int nTok3 = ReadINT(32);
        int nTok4 = ReadINT(32);
        ReadINT(32);
        int nSel = ReadINT(32);

        if (nHaveData == 0)
            return 0;

        g_pTlkTable->SetCustomToken(0, CExoString(nTok0));
        g_pTlkTable->SetCustomToken(1, CExoString(nTok1));

        CExoString sTok2;
        sTok2 = g_pAppManager->m_pClientExoApp->GetGUIString(nSel == 0 ? STRREF_FAILURE : STRREF_SUCCESS);
        g_pTlkTable->SetCustomToken(2, sTok2);

        g_pTlkTable->SetCustomToken(3, CExoString(nTok3));
        g_pTlkTable->SetCustomToken(4, CExoString(nTok4));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_THREAT_BREAKDOWN, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);
        break;
    }

    case CC_MESSAGE_DEFENSE_BREAKDOWN:
    {
        // Nine ints, the fourth read and discarded. The first is required and gets custom token 0 plus
        // the leading strref; each of the remaining seven, when non-zero, gets its own token-0 publish
        // and label. The arm leaves bRet at 0.
        int nValue0 = ReadINT(32);
        int nValue1 = ReadINT(32);
        int nValue2 = ReadINT(32);
        ReadINT(32);
        int nValue4 = ReadINT(32);
        int nValue5 = ReadINT(32);
        int nValue6 = ReadINT(32);
        int nValue7 = ReadINT(32);
        int nValue8 = ReadINT(32);

        if (nValue0 == 0)
            return 0;

        g_pTlkTable->SetCustomToken(0, CExoString(nValue0));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DEFENSE_BREAKDOWN, 0, 0, 0);

        if (nValue1 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue1));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_ARMOR, 0, 0, 0);
        }

        if (nValue2 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue2));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_DEX_MOD, 0, 0, 0);
        }

        if (nValue4 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue4));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_CLASS, 0, 0, 0);
        }

        if (nValue5 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue5));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_NATURAL, 0, 0, 0);
        }

        if (nValue6 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue6));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_FEATS_AND_EFFECTS_MOD, 0, 0, 0);
        }

        if (nValue7 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue7));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_FEAT, 0, 0, 0);
        }

        if (nValue8 != 0)
        {
            g_pTlkTable->SetCustomToken(0, CExoString(nValue8));
            sOut = sOut + g_pSWTlkTable->Fetch(STRREF_FMT_DEBILITATED_PENALTY, 0, 0, 0);
        }

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);
        break;
    }

    case CC_MESSAGE_DAMAGE_BREAKDOWN:
    {
        // Damage breakdown, the last of the three 31-int wire records. The field names come from the
        // damage signal the server fills it from:
        //   0..2   the physical trio, summed positive into nValues[2] and rendered as one term
        //   3..13  the remaining damage types (per-slot label from s_anDamageTypeLabels)
        //   14     m_anDamage[14] == the base damage: the arm's gate, and the head of token 0
        //   15     m_nDamageRoll     (read, dropped)
        //   16     m_nStrDamageBonus     -> label, sep " "
        //   17     m_nFeatId                  (read, dropped)
        //   18     m_nDamageBonus        -> label, sep " + "
        //   19     damage.m_nReserved1           (read, dropped)
        //   20     m_nSneakAttackDamage        -> label, sep " + "
        //   21     m_nWeaponSpecializationBonus    -> label, sep " "
        //   22     m_threat.m_nUpgradeType         -> label, into the final token-0 string
        //   23     m_nFeatSoakAmount     -> label, sep " - "
        //   24     m_nFeatFlatDRAmount   -> label, sep " - "
        //   25..27 m_nDamageTypeBits / m_nDamageAfterSoak / m_nDamageAfterFlatDR
        //          (all three read and dropped)
        //   28     damage.m_nSpellDamageReduction           -> token 1 (the spell-bonus amount)
        //   29     damage.m_bOffHandWeaponEquipped           -> the spell id: token 0 is that spell's name
        //   30     m_nEffectDamageBonus  -> label, sep " - "
        //
        // Six of the 31 are read and dropped, and they are load-bearing because each read advances the
        // wire cursor. Three separators are in use: " + " for the render loop and the plus terms, a
        // bare space for the strength and special-bonus terms, and " - " for soak, flat DR and the
        // spell bonus. The label table stops at 14 entries while nValues holds 15.
        int nValues[15] = {0};
        CExoString sDamageList;

        for (int i = 0; i < 14; i++)
        {
            if (i < 3)
            {
                int nDamage = ReadINT(32);
                if (nDamage > 0)
                    nValues[2] += nDamage;
            }
            else
            {
                nValues[i] = ReadINT(32);
            }
        }

        int nBaseDamage = ReadINT(32);
        ReadINT(32);
        int nStrDamageBonus = ReadINT(32);
        ReadINT(32);
        int nDamageBonus = ReadINT(32);
        ReadINT(32);
        int nSneakDamage = ReadINT(32);
        int nSpecDamageBonus = ReadINT(32);
        int nThreatBonus = ReadINT(32);
        int nFeatSoak = ReadINT(32);
        int nFeatFlatDR = ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        ReadINT(32);
#if !KOTOR1  // K1: 28 INTs -- no spell bonus, spell id or effect damage
        int nSpellBonus = ReadINT(32);
        int nSpellId = ReadINT(32);
        int nEffectDamage = ReadINT(32);
#endif

        int bFirst = 1;

        for (int i = 0; i < 14; i++)
        {
            int nDamage = nValues[i];
            uint64_t nStrRef = s_anDamageTypeLabels[i];

            if (nDamage < 1)
                continue;

            g_pTlkTable->SetCustomToken(0, CExoString(nDamage));

            CExoString sLabel = g_pSWTlkTable->Fetch(nStrRef, 0, 0, 0);

            if (i < 3 || i == 12)
            {
                if (bFirst != 0)
                    sDamageList = sDamageList + sLabel;
                else
                    sDamageList = sLabel + CExoString(" + ") + sDamageList;
            }
            else
            {
                if (bFirst != 0)
                    sDamageList = sDamageList + sLabel;
                else
                    sDamageList = sDamageList + CExoString(" + ") + sLabel;
            }

            bFirst = 0;
        }

        if (nBaseDamage < 1)
            return 0;

        if (nStrDamageBonus != 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only (same dead pair as case 4's sTmpName)
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nStrDamageBonus));

            sDamageList = sDamageList + CExoString(" ") + g_pSWTlkTable->Fetch(STRREF_FMT_STRENGTH_MOD, 0, 0, 0);
        }

        if (nSpecDamageBonus != 0)
        {
            // No dead CExoString/CResRef pair here, unlike its six siblings: this is the one term that
            // declares none.
            g_pTlkTable->SetCustomToken(0, CExoString(nSpecDamageBonus));

            sDamageList = sDamageList + CExoString(" ") + g_pSWTlkTable->Fetch(STRREF_FMT_WEAPON_SPECIALIZATION, 0, 0, 0);
        }

        if (nDamageBonus > 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nDamageBonus));

            sDamageList = sDamageList + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_BONUS_DAMAGE, 0, 0, 0);
        }

#if !KOTOR1
        if (nEffectDamage != 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nEffectDamage));

            sDamageList = sDamageList + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_MISC_EFFECT_DAMAGE, 0, 0, 0);
        }
#endif

        if (nSneakDamage > 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nSneakDamage));

            sDamageList = sDamageList + CExoString(" + ") + g_pSWTlkTable->Fetch(STRREF_FMT_SNEAK_ATTACK_DAMAGE, 0, 0, 0);
        }

        if (nFeatSoak != 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nFeatSoak));

            sDamageList = sDamageList + CExoString(" - ") + g_pSWTlkTable->Fetch(STRREF_FMT_TOUGHNESS, 0, 0, 0);
        }

        if (nFeatFlatDR != 0)
        {
            CExoString sTmpDamage;  // ctor + dtor only
            CResRef resTmp;         // ctor only

            g_pTlkTable->SetCustomToken(0, CExoString(nFeatFlatDR));

            sDamageList = sDamageList + CExoString(" - ") + g_pSWTlkTable->Fetch(STRREF_FMT_WOOKIEE_TOUGHNESS, 0, 0, 0);
        }

#if !KOTOR1
        if (nSpellBonus != 0)
        {
            g_pTlkTable->SetCustomToken(
                0, g_pRules->GetSpell(nSpellId)->GetSpellNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nSpellBonus));

            sDamageList = sDamageList + CExoString(" - ") + g_pSWTlkTable->Fetch(STRREF_FMT_SPELL_BONUS, 0, 0, 0);
        }
#endif

        {
            CExoString sTmp;

            if (nThreatBonus > 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(nThreatBonus));

                sTmp = g_pSWTlkTable->Fetch(STRREF_FMT_CRITICAL_MULTIPLIER, 0, 0, 0);
            }

            sTmp = sTmp + CExoString(nBaseDamage);

            g_pTlkTable->SetCustomToken(0, sTmp);
            g_pTlkTable->SetCustomToken(1, sDamageList);

            sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DAMAGE_BREAKDOWN, 0, 0, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
                sOut, kMessageTypeFeedback, 0);
        }

        // The arm leaves bRet at 0.
        break;
    }

    // 24 is an empty arm, sharing the switch's exit with case 29.
    case CC_MESSAGE_CRITICAL_HIT_BREAKDOWN:
    {
        break;
    }

    case CC_MESSAGE_EFFECT_BREAKDOWN:
    {
        // Nine ints go in and only the second and the ninth are kept: the second is the display
        // selector, the ninth is a variant discriminator that the arm rejects outright at -1 and 1.
        // The variant selects one of a small run of adjacent strrefs and the selector picks a token-1
        // string out of a switch whose 11/12/13/17 arms are the default (strref 0). Also note the arm
        // does not set bRet: like case 21 it just `break`s, leaving bRet at 0.
        unsigned long long objId = ReadOBJECTIDClient();
        ReadINT(32);
        int nKind = ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        ReadINT(32);
        int nSel = ReadINT(32);

        if (nSel == -1 || nSel == 1)
            return 0;

        CExoString sTmp;

        sTmp = g_pSWTlkTable->Fetch(STRREF_EFFECT_APPLICATION_BREAKDOWN, 0, 0, 0);

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        // The selector is compared and scaled as unsigned here, so the cast is part of the spelling.
        uint64_t nStrRef = STRREF_FMT_EFFECT_APPLIED;
        if (static_cast<unsigned int>(nSel) < 3u)
            nStrRef += static_cast<unsigned int>(nSel);

        if (nKind != 0)
        {
            uint64_t nKindStrRef = 0;

            switch (nKind)
            {
            case 1:  nKindStrRef = STRREF_IS_CONFUSED; break;
            case 2:  nKindStrRef = STRREF_IS_FRIGHTENED; break;
            case 3:  nKindStrRef = STRREF_IS_DROID_STUNNED; break;
            case 4:  nKindStrRef = STRREF_IS_STUNNED; break;
            case 5:  nKindStrRef = STRREF_IS_HELD; break;
            case 6:  nKindStrRef = STRREF_IS_SLEEPING; break;
            case 7:  nKindStrRef = STRREF_IS_CHOKED; break;
            case 8:  nKindStrRef = STRREF_IS_HORRIFIED; break;
            case 9:  nKindStrRef = STRREF_IS_FORCE_PUSHED; break;
            case 10: nKindStrRef = STRREF_IS_IN_WHIRLWIND; break;
#if !KOTOR1  // K1: kinds 1..10 only
            case 14: nKindStrRef = STRREF_IS_KNOCKED_DOWN; break;
            case 15: nKindStrRef = STRREF_IS_BEING_CRUSHED; break;
            case 16: nKindStrRef = STRREF_IS_CONFUSED; break;
            case 18: nKindStrRef = STRREF_IS_MIND_TRICKED; break;
            case 19: nKindStrRef = STRREF_IS_SCRAMBLED; break;
#endif
            }


            g_pTlkTable->SetCustomToken(1, g_pSWTlkTable->Fetch(nKindStrRef, 0, 0, 0));
        }

        sOut = sTmp + g_pSWTlkTable->Fetch(nStrRef, 0, 0, 0);

        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        break;
    }

    case CC_MESSAGE_DEFLECT_BREAKDOWN:
    {
        // Deflect breakdown, sent by the server's shared breakdown forwarder. The attack-resolution
        // code fills entries 0..16 from the attack's deflect data when the result lands in the
        // deflect/reflect band; entry 0, the deflect result, is read and dropped.
        //
        // One sentence is assembled in a single CExoString: the roll first, then five optional term
        // pairs, then three mandatory terms. A pair publishes token 0 as the feat or spell name (a
        // fresh TLK fetch, hence the speaker reset inside every block) and token 1 as the bonus, so the
        // five blocks differ only in whether they use a feat or a spell.
        //
        // Token 1 is the deflect total, token 2 the assembled string and token 3 the attack total; the
        // line goes to the combat buffer. A zero deflect total ends the arm early, and the arm leaves
        // bRet at 0.
        unsigned long long objId = ReadOBJECTIDClient();

        ReadINT(32);                                 // m_nDeflectResult -- read, never used
        int nAttackTotal = ReadINT(32);
        int nDeflectTotal = ReadINT(32);
        int nDeflectRoll = ReadINT(32);
        int nDeflectFeatBonus = ReadINT(32);
        int nDeflectFeat = ReadINT(32);
        int nBaseAttackBonus = ReadINT(32);
        int nArmourPenalty = ReadINT(32);
        int nEffectBonus = ReadINT(32);
#if !KOTOR1  // K1: type 25's record (object + 9 INTs) -- only the deflect-feat pair
        int nPowerBonus = ReadINT(32);
        int nPowerId = ReadINT(32);
        int nStanceBonus = ReadINT(32);
        int nStanceId = ReadINT(32);
        int nClassFeatBonus = ReadINT(32);
        int nClassFeat = ReadINT(32);
        int nAttackerFeatPenalty = ReadINT(32);
        int nAttackerFeat = ReadINT(32);
#endif

        if (nDeflectTotal == 0)
            return 0;

        CExoString sTokenValue;
        GetSingleCustomTokenStringPart(nDeflectRoll, STRREF_FMT_ROLL, sTokenValue);

        if (nDeflectFeatBonus != 0)
        {
            g_pTlkTable->SetCustomToken(0, g_pRules->GetFeat(static_cast<unsigned short>(nDeflectFeat))->GetNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nDeflectFeatBonus));

            sTokenValue = sTokenValue + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
        }

#if !KOTOR1
        if (nClassFeatBonus != 0)
        {
            g_pTlkTable->SetCustomToken(0, g_pRules->GetFeat(static_cast<unsigned short>(nClassFeat))->GetNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nClassFeatBonus));

            sTokenValue = sTokenValue + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
        }

        if (nAttackerFeatPenalty != 0)
        {
            g_pTlkTable->SetCustomToken(0, g_pRules->GetFeat(static_cast<unsigned short>(nAttackerFeat))->GetNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nAttackerFeatPenalty));

            sTokenValue = sTokenValue + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
        }

        if (nPowerBonus != 0)
        {
            g_pTlkTable->SetCustomToken(0,
                g_pRules->GetSpell(nPowerId)->GetSpellNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nPowerBonus));

            sTokenValue = sTokenValue + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
        }

        if (nStanceBonus != 0)
        {
            g_pTlkTable->SetCustomToken(0,
                g_pRules->GetSpell(nStanceId)->GetSpellNameText());
            g_pTlkTable->SetCustomToken(1, CExoString(nStanceBonus));

            sTokenValue = sTokenValue + g_pSWTlkTable->Fetch(STRREF_FMT_PLUS_LABEL_VALUE, 0, 0, 0);
        }
#endif

        GetSingleCustomTokenStringPart(nBaseAttackBonus, STRREF_FMT_BASE_ATTACK_BONUS, sTokenValue);
        GetSingleCustomTokenStringPart(nArmourPenalty, STRREF_FMT_ARMOR_CHECK_PENALTY, sTokenValue);
        GetSingleCustomTokenStringPart(nEffectBonus, STRREF_FMT_ITEM_BONUS, sTokenValue);

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        g_pTlkTable->SetCustomToken(1, CExoString(nDeflectTotal));
        g_pTlkTable->SetCustomToken(2, sTokenValue);
        g_pTlkTable->SetCustomToken(3, CExoString(nAttackTotal));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_DEFLECTION_BREAKDOWN, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);

        break;
    }

    case CC_MESSAGE_POISONED:
    {
        unsigned long long objId = ReadOBJECTIDClient();

        if (g_pAppManager->m_pClientExoApp->GetObjectName(objId, sName))
        {
            g_pTlkTable->SetCustomToken(0, sName);
        }
        else
        {
#if KOTOR1
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(0);
#else
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sName);
        }

        g_pTlkTable->SetCustomToken(1, g_pTlkTable->GetSimpleString(ReadINT(32)));

        sOut = g_pSWTlkTable->Fetch(STRREF_FMT_AFFECTED_BY, 0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(
            sOut, kMessageTypeFeedback, 0);
        break;
    }

    default:
        break;
    }

    return bRet;
}

#if KOTOR1
#undef AppendToCombatBuffer
#endif


int CSWCMessage::HandleServerToPlayerMessage(unsigned char* pMessage, unsigned int nSize)
{
    CExoString sMessageName;
    int bOverflow;
    int bRet = 0;

    unsigned char nMajor = pMessage[0];
    unsigned char nCategory = pMessage[1];
    unsigned char nType = pMessage[2];

    SetReadMessage(pMessage + 3, nSize - 3, MSG_PLAYERID_ALL_CLIENTS, 1);

    bOverflow = MessageReadOverflow(1);

    if (nMajor != MSG_SERVER_TO_PLAYER || bOverflow)
        return 0;

    sMessageName.Format(const_cast<char*>("unknown Major (0x%.2X)"), nCategory);

    switch (nCategory)
    {
    case MSG_MAJOR_SERVER_STATUS:
        sMessageName = "ServerStatus";
        bRet = HandleServerToPlayerServerStatus(nType);
        break;
    case MSG_MAJOR_LOGIN:
        sMessageName = "Login";
        bRet = HandleServerToPlayerLogin(nType);
        break;
    case MSG_MAJOR_MODULE:
        sMessageName = "Module";
        bRet = HandleServerToPlayerModule(nType);
        break;
    case MSG_MAJOR_AREA:
        sMessageName = "Area";
        bRet = HandleServerToPlayerArea(nType);
        break;
    case MSG_MAJOR_GAME_OBJECT_UPDATE:
        sMessageName = "GameObjUpdate";
        bRet = HandleServerToPlayerGameObjectUpdate(nType);
        break;
    case MSG_MAJOR_CHAT:
        sMessageName = "Chat";
        bRet = HandleServerToPlayerChat(nType);
        break;
    case MSG_MAJOR_PLAYER_LIST:
        sMessageName = "PlayerList";
        bRet = HandleServerToPlayerPlayerList(nType);
        break;
    case MSG_MAJOR_INVENTORY:
        sMessageName = "Inventory";
        bRet = HandleServerToPlayerInventory(nType);
        break;
    case MSG_MAJOR_GUI_INVENTORY:
        sMessageName = "GuiInventory";
        bRet = HandleServerToPlayerGuiInventory(nType);
        break;
    case MSG_MAJOR_PARTY:
        sMessageName = "Party";
        bRet = HandleServerToPlayerParty(nType);
        break;
    case MSG_MAJOR_CHEAT:
        sMessageName = "Cheat";
        bRet = HandleServerToPlayerCheatMessage(nType);
        break;
    case MSG_MAJOR_CAMERA:
        sMessageName = "Camera";
        bRet = HandleServerToPlayerCamera(nType);
        break;
    case MSG_MAJOR_CHAR_LIST:
        sMessageName = "CharList";
        bRet = HandleServerToPlayerCharacterList(nType);
        break;
    case MSG_MAJOR_CLIENT_SIDE_MESSAGE:
        sMessageName = "ClientSideMessage";
        bRet = HandleServerToPlayerCCMessage(nType);
        break;
    case MSG_MAJOR_COMBAT_ROUND:
        sMessageName = "Combat_Round";
        bRet = HandleServerToPlayerCombatRound(nType);
        break;
    case MSG_MAJOR_DIALOG:
        sMessageName = "Dialog";
        bRet = HandleServerToPlayerDialog(nType);
        break;
    case MSG_MAJOR_GUI_CHARACTER_SHEET:
        sMessageName = "GUICharacterSheet";
        bRet = HandleServerToPlayerGUICharacterSheet(nType);
        break;
    case MSG_MAJOR_QUICK_CHAT:
        sMessageName = "QuickChat";
        bRet = HandleServerToPlayerQuickChat(nType);
        break;
    case MSG_MAJOR_SOUND:
        sMessageName = "Sound";
        bRet = HandleServerToPlayerSound(nType);
        break;
    case MSG_MAJOR_ITEM_PROPERTY:
        sMessageName = "Item_Property";
        bRet = HandleServerToPlayerActiveItemPropertyUpdate(nType);
        break;
    case MSG_MAJOR_GUI_CONTAINER:
        sMessageName = "GuiContainer";
        bRet = HandleServerToPlayerGuiContainerObjectMessage(nType);
        break;
    case MSG_MAJOR_VOICE_CHAT:
        sMessageName = "VoiceChat";
        bRet = HandleServerToPlayerVoiceChatMessage(nType);
        break;
    case MSG_MAJOR_GUI_INFO_POPUP:
        sMessageName = "GuiInfoPopup";
        bRet = HandleServerToPlayerGuiInfoPopupMessage(nType);
        break;
    case MSG_MAJOR_JOURNAL:
        sMessageName = "Journal";
        bRet = HandleServerToPlayerJournalMessage(nType);
        break;
    case MSG_MAJOR_LEVEL_UP:
        sMessageName = "LevelUp";
        bRet = HandleServerToPlayerLevelUpMessage(nType);
        break;
    case MSG_MAJOR_GUI_QUICKBAR:
        sMessageName = "GuiQuickbar";
        bRet = HandleServerToPlayerGuiQuickbar(nType);
        break;
    case MSG_MAJOR_MAP_PIN:
        sMessageName = "MapPin";
        bRet = HandleServerToPlayerMapPin(nType);
        break;
    case MSG_MAJOR_DEBUG_INFO:
        sMessageName = "DebugInfo";
        bRet = HandleServerToPlayerDebugInfo(nType);
        break;
    case MSG_MAJOR_SAFE_PROJECTILE:
        sMessageName = "SafeProjectile";
        bRet = HandleServerToPlayerSafeProjectile(nType);
        break;
    case MSG_MAJOR_POPUP_GUI_PANEL:
        sMessageName = "PopUpGUIPanel";
        bRet = HandleServerToPlayerPopUpGUIPanel(nType);
        break;
    case MSG_MAJOR_AMBIENT:
        sMessageName = "Ambient";
        bRet = HandleServerToPlayerAmbient(nType);
        break;
    case MSG_MAJOR_CHARACTER_DOWNLOAD:
        sMessageName = "Character_Download";
        bRet = HandleServerToPlayerCharacterDownload(nType);
        break;
    case MSG_MAJOR_LOAD_BAR:
        sMessageName = "LoadBar";
        bRet = HandleServerToPlayerLoadBar(nType);
        break;
    case MSG_MAJOR_SAVE_LOAD:
        sMessageName = "SaveLoad";
        bRet = HandleServerToPlayerSaveLoad(nType);
        break;
    case MSG_MAJOR_SHUTDOWN_SERVER:
        sMessageName = "ShutDownServer";
        bRet = HandlePlayerToServerShutDownServer(nType);
        break;
    case MSG_MAJOR_GUI_TIMING_EVENT:
        sMessageName = "LevelUp";
        bRet = HandleServerToPlayerGuiTimingEvent(nType);
        break;
    case MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST:
        sMessageName = "PlayModuleCharacterList";
        bRet = HandleServerToPlayerPlayModuleCharacterList(nType);
        break;
    case MSG_MAJOR_CUSTOM_TOKEN:
        sMessageName = "CustomToken";
        bRet = HandleServerToPlayerSetCustomToken(nType);
        break;
    }

    CExoString sError;

    if (MessageReadOverflow(0))
        sError.Format(const_cast<char*>("Client message OVERFLOW in %s (0x%.2X)\nFIX this or FLOG the person responsible!"), sMessageName.CStr(), nType);

    if (MessageReadUnderflow(0))
        sError.Format(const_cast<char*>("Client message UNDERFLOW in %s (0x%.2X)\nFIX this or FLOG the person responsible!"), sMessageName.CStr(), nType);

    return bRet;
}


int CSWCMessage::HandleServerToPlayerDebugInfo_Creature(unsigned long long objId)
{
    char szBuf[64];

#if KOTOR1  // K1 has 18 equipment slots
    CExoString aItemTags[18];
#else
    CExoString aItemTags[20];
#endif
    CExoString sLine;
    CExoString sAppearance;
    CExoString sTag;
    CExoString sFaction;
    CResRef resConversation;
    CExoString sAreaTag;

    sTag = ReadCExoString(32);
    sFaction = ReadCExoString(32);
    resConversation = ReadCResRef(16);
    short nCurrentHitPoints = ReadSHORT(16);
    short nMaxHitPoints = ReadSHORT(16);
    short nArmorClass = ReadSHORT(16);
    int nAILevel = ReadINT(32);
    float fChallengeRating = ReadFLOAT(1.0f, 32);
    sAreaTag = ReadCExoString(32);

    unsigned int nEquippedMask = ReadDWORD(32);
    int nNumEquipped = 0;
#if KOTOR1
    for (unsigned int nSlot = 0; nSlot < 18; nSlot++)
#else
    for (unsigned int nSlot = 0; nSlot < 20; nSlot++)
#endif
    {
        if (nEquippedMask & (1 << nSlot))
        {
            aItemTags[nSlot] = ReadCExoString(32);
            nNumEquipped++;
        }
    }

    unsigned int nNumRepositoryItems = ReadDWORD(32);

    // The read loop is not inside the allocation's guard, so it keeps its own zero-trip test.
    CExoString* paRepositoryTags = 0;
    if (nNumRepositoryItems != 0)
        paRepositoryTags = new CExoString[nNumRepositoryItems];

    for (unsigned int i = 0; i < nNumRepositoryItems; i++)
        paRepositoryTags[i] = ReadCExoString(32);

    if (MessageReadOverflow(1))
        return 0;

    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    if (pCreature == 0)
        return 0;

    CSWCCreatureAppearance* pAppearance = pCreature->m_pCreatureAppearance;
    if (pAppearance == 0)
        return 0;

    CSWCCreatureStats* pStats = pCreature->m_pStats;
    if (pStats == 0)
        return 0;

    CSWCArea* pArea = pCreature->m_pArea;
    if (pArea == 0)
        return 0;

    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui == 0)
        return 0;

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

#if KOTOR1  // K1 uses the two stored name fields, not the split full name
    sLine = "FirstName: ";
    sLine = sLine + pStats->m_name;
    sLine = sLine + CExoString("    LastName: ");
    sLine = sLine + pStats->m_sLastName;
    sLine = sLine + CExoString("    Tag: ");
    sLine = sLine + sTag;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    pStats->m_name.CStr();
    pStats->m_sLastName.CStr();
    sTag.CStr();
#else
    sLine = "FirstName: ";
    sLine = sLine + pStats->GetFirstName();
    sLine = sLine + CExoString("    LastName: ");
    sLine = sLine + pStats->GetLastName();
    sLine = sLine + CExoString("    Tag: ");
    sLine = sLine + sTag;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    pStats->GetFirstName().CStr();
    pStats->GetLastName().CStr();
    sTag.CStr();
#endif

    sLine = "Conversation File: ";
    sLine = sLine + CExoString(resConversation.GetResRef());
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    resConversation.GetResRef();

    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pAppearance->m_nAppearanceId, CExoString("LABEL"), &sAppearance);

    sLine = "Appearance: ";
    sLine = sLine + sAppearance;
    sLine = sLine + CExoString("    Race: ");
    sLine = sLine + pStats->GetRaceString();
    sLine = sLine + CExoString("    Class: ");
    sLine = sLine + pStats->GetClassesString(' ');
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    sAppearance.CStr();
    pStats->GetRaceString().CStr();
    pStats->GetClassesString(' ').CStr();

    sLine = "Faction: ";
    sLine = sLine + sFaction;
    sLine = sLine + CExoString("    AI Level: ");
    sLine = sLine + CExoString(nAILevel);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    sFaction.CStr();
    CExoString(nAILevel).CStr();

    sLine = "CurrentHP: ";
    sLine = sLine + CExoString(nCurrentHitPoints);
    sLine = sLine + CExoString("    MaxHP: ");
    sLine = sLine + CExoString(nMaxHitPoints);
    sLine = sLine + CExoString("    AC: ");
    sLine = sLine + CExoString(nArmorClass);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    // Discarded stat reads; the values reported above came off the wire.
    CExoString(pStats->m_nCurrentHitPoints).CStr();
    CExoString(static_cast<short>(pStats->GetMaxHitPoints())).CStr();
    CExoString(static_cast<short>(pStats->GetArmorClass(0))).CStr();

    sprintf(szBuf, "%.2f, %.2f, %.2f", pCreature->m_position.x, pCreature->m_position.y,
            pCreature->m_position.z);

    sLine = "Area Name: ";
    sLine = sLine + pArea->m_areaName;
    sLine = sLine + CExoString("    Area Tag: ");
    sLine = sLine + sAreaTag;
    sLine = sLine + CExoString("    Pos: ");
    sLine = sLine + CExoString(szBuf);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    pArea->m_areaName.CStr();

    sLine = "EquippedItems: ";
    sLine = sLine + CExoString(nNumEquipped);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(nNumEquipped).CStr();

    if (nEquippedMask & MSG_SLOT_HEAD)
    {
        sLine = "    Head: ";
        sLine = sLine + aItemTags[0];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[0].CStr();
    }

    if (nEquippedMask & MSG_SLOT_BODY)
    {
        sLine = "    Chest: ";
        sLine = sLine + aItemTags[1];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[1].CStr();
    }

    if (nEquippedMask & MSG_SLOT_BOOTS)
    {
        sLine = "    Boots: ";
        sLine = sLine + aItemTags[2];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[2].CStr();
    }

    if (nEquippedMask & MSG_SLOT_HANDS)
    {
        sLine = "    Arms: ";
        sLine = sLine + aItemTags[3];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[3].CStr();
    }

    if (nEquippedMask & MSG_SLOT_RIGHT_WEAPON)
    {
        sLine = "    RightHand: ";
        sLine = sLine + aItemTags[4];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[4].CStr();
    }

    if (nEquippedMask & MSG_SLOT_LEFT_WEAPON)
    {
        sLine = "    LeftHand: ";
        sLine = sLine + aItemTags[5];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[5].CStr();
    }

    if (nEquippedMask & MSG_SLOT_CLOAK)
    {
        sLine = "    Cloak: ";
        sLine = sLine + aItemTags[6];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[6].CStr();
    }

    // Source order, not numeric order, puts bit 8 before bit 7.
    if (nEquippedMask & MSG_SLOT_RIGHT_ARM)
    {
        sLine = "    RightForeArm: ";
        sLine = sLine + aItemTags[8];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[8].CStr();
    }

    if (nEquippedMask & MSG_SLOT_LEFT_ARM)
    {
        sLine = "    LeftForeArm: ";
        sLine = sLine + aItemTags[7];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[7].CStr();
    }

    if (nEquippedMask & MSG_SLOT_IMPLANT)
    {
        sLine = "    Neck: ";
        sLine = sLine + aItemTags[9];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[9].CStr();
    }

    if (nEquippedMask & MSG_SLOT_BELT)
    {
        sLine = "    Belt: ";
        sLine = sLine + aItemTags[10];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[10].CStr();
    }

    if (nEquippedMask & MSG_SLOT_CREATURE_WEAPON_L)
    {
        sLine = "    CWeapon1: ";
        sLine = sLine + aItemTags[14];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[14].CStr();
    }

    if (nEquippedMask & MSG_SLOT_CREATURE_WEAPON_R)
    {
        sLine = "    CWeapon2: ";
        sLine = sLine + aItemTags[15];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[15].CStr();
    }

    if (nEquippedMask & MSG_SLOT_CREATURE_WEAPON_B)
    {
        sLine = "    CWeapon3: ";
        sLine = sLine + aItemTags[16];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[16].CStr();
    }

    if (nEquippedMask & MSG_SLOT_CREATURE_ARMOR)
    {
        sLine = "    CArmor: ";
        sLine = sLine + aItemTags[17];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        aItemTags[17].CStr();
    }

    sLine = "RepositoryItems: ";
    sLine = sLine + CExoString(nNumRepositoryItems);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(nNumRepositoryItems).CStr();

    for (unsigned int i = 0; i < nNumRepositoryItems; i++)
    {
        sLine = "    ";
        sLine = sLine + paRepositoryTags[i];
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);
        paRepositoryTags[i].CStr();
    }

    sLine = "KnownSpells: ";
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    for (unsigned char nClassPos = 0; nClassPos < pStats->m_nNumClasses; nClassPos++)
    {
        CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClass(nClassPos)];
        if (pClass == 0)
            continue;

        sLine = "    Class: ";
        sLine = sLine + pClass->GetNameText();
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

        pClass->GetNameText().CStr();

        // Narrower than the declared int return, so the compare and the later widening are both
        // 16-bit.
        unsigned short nNumKnownSpells = pStats->GetNumberKnownSpells(nClassPos, 0);
        if (nNumKnownSpells == 0)
            continue;

        // The spell level is hard-coded to 0 throughout this block; even the "Level: " field formats a
        // literal zero.
        sLine = "        Level: ";
        sLine = sLine + CExoString(0);
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

        CExoString(0).CStr();

        if (CSWClass::IsJedi(pStats->GetClass(nClassPos)))
        {
            sLine = "        SpellsPerDayLeft: ";
            sLine = sLine + CExoString(pStats->GetSpellsPerDayLeft(nClassPos, 0));
            pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

            CExoString(pStats->GetSpellsPerDayLeft(nClassPos, 0)).CStr();
        }

        for (unsigned int i = 0; i < nNumKnownSpells; i++)
        {
            int nSpellId = pStats->GetKnownSpell(nClassPos, 0, static_cast<unsigned char>(i));
            if (g_pRules->GetSpell(nSpellId) != 0)
            {
                sLine = "            ";
                sLine = sLine + g_pRules->GetSpell(nSpellId)->GetSpellNameText();
                pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

                g_pRules->GetSpell(nSpellId)->GetSpellNameText().CStr();
            }
        }
    }

    sprintf(szBuf, "%.2f", fChallengeRating);

    sLine = "ChallengeRating: ";
    sLine = sLine + CExoString(szBuf);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

    delete[] paRepositoryTags;

    return 1;
}


int CSWCMessage::HandleServerToPlayerDebugInfo_Placeable(unsigned long long objId)
{
    CResRef resTemplate;
    CExoString sLine;
    CExoString sScratch;
    CExoString sName;
    CExoString sTag;
    CExoString sUnused;
    CExoString sFaction;
    unsigned char nHardness;
    short nHitPoints;
    unsigned char nFortSave;
    unsigned char nRefSave;
    unsigned char nWillSave;
    CSWCPlaceable* pPlaceable;
    CGuiInGame* pGui;

    sName = ReadCExoLocStringClient();
    sTag = ReadCExoString(32);
    nHardness = ReadBYTE(8);
    nHitPoints = ReadSHORT(16);
    nFortSave = ReadBYTE(8);
    nRefSave = ReadBYTE(8);
    nWillSave = ReadBYTE(8);
    resTemplate = ReadCResRef(16);
    sFaction = ReadCExoString(32);

    if (MessageReadOverflow(1))
        return 0;

    pPlaceable = g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(objId);
    if (pPlaceable == 0)
        return 0;

    pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui == 0)
        return 0;

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

    g_pRules->m_p2DArrays->m_pPlaceables->GetCExoStringEntry(
        pPlaceable->m_nAppearanceType, CExoString("LABEL"), &sScratch);

    sLine = "Name: ";
    sLine = sLine + sName;
    sLine = sLine + CExoString("    Tag: ");
    sLine = sLine + sTag;
    sLine = sLine + CExoString("    Appearance: ");
    sLine = sLine + sScratch;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    sName.CStr();
    sTag.CStr();
    sScratch.CStr();

    sLine = "Hardness: ";
    sLine = sLine + CExoString(nHardness);
    sLine = sLine + CExoString("    HP: ");
    sLine = sLine + CExoString(nHitPoints);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(nHardness).CStr();
    CExoString(nHitPoints).CStr();

    sLine = "FortitudeSave: ";
    sLine = sLine + CExoString(nFortSave);
    sLine = sLine + CExoString("    ReflexSave: ");
    sLine = sLine + CExoString(nRefSave);
    sLine = sLine + CExoString("    WillSave: ");
    sLine = sLine + CExoString(nWillSave);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(nFortSave).CStr();
    CExoString(nRefSave).CStr();
    CExoString(nWillSave).CStr();

    resTemplate.CopyToString(sScratch);

    sLine = "TemplateResRef: ";
    sLine = sLine + sScratch;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    sScratch.CStr();

    sLine = "Faction: ";
    sLine = sLine + sFaction;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    sFaction.CStr();

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

    return 1;
}


int CSWCMessage::HandleServerToPlayerDebugInfo_Item(unsigned long long objId)
{
    STR_RES strRes;
    CExoString sBaseName;
    CExoString sLine;
    CExoString sTag;
    int nPassiveProperties;
    unsigned short* pnPassiveTypes;
    C2DA* p2DA;
    CSWCItem* pItem;
    CGuiInGame* pGui;
    int nStringRef;

    sTag = ReadCExoString(32);
    nPassiveProperties = ReadINT(32);

    if (nPassiveProperties > 0)
    {
        pnPassiveTypes = new unsigned short[nPassiveProperties];
        for (int i = 0; i < nPassiveProperties; i++)
            pnPassiveTypes[i] = ReadWORD(16);
    }
    else
    {
        pnPassiveTypes = 0;
    }

    if (MessageReadOverflow(1))
        return 0;

    p2DA = new C2DA(CResRef("ITEMPROPS"), 0);
    if (p2DA->Load2DArray() == 0)
        return 0;

    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
    if (pItem == 0)
        return 0;

    pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui == 0)
        return 0;

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

    sBaseName = g_pRules->GetBaseItem(pItem->m_baseItem)->GetNameText();

    sLine = "Name: ";
    sLine = sLine + pItem->GetName(0);
    sLine = sLine + CExoString("    Tag: ");
    sLine = sLine + sTag;
    sLine = sLine + CExoString("    BaseType: ");
    sLine = sLine + sBaseName;
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    pItem->GetName(0).CStr();
    sTag.CStr();
    sBaseName.CStr();

    sLine = "Weight: ";
    sLine = sLine + CExoString(0);
    sLine = sLine + CExoString("    AC: ");
    sLine = sLine + CExoString(pItem->m_nBaseArmorClass);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(0).CStr();
    CExoString(pItem->m_nBaseArmorClass).CStr();

    sLine = "ActiveProperties: ";
    sLine = sLine + CExoString(pItem->m_nNumActiveProperties);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(pItem->m_nNumActiveProperties).CStr();

    for (int i = 0; i < pItem->m_nNumActiveProperties; i++)
    {
        CSWItemProperty* pProperty = pItem->GetActiveProperty(i);
        if (pProperty != 0)
        {
            p2DA->GetINTEntry(pProperty->m_type, CExoString("STRINGREF"), &nStringRef);
            g_pTlkTable->Fetch(nStringRef, strRes, CTlkTable::GENDER_MALE);

            sLine = "    ";
            sLine = sLine + strRes.text;
            pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

            strRes.text.CStr();
        }
    }

    sLine = "PassiveProperties: ";
    sLine = sLine + CExoString(nPassiveProperties);
    pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

    CExoString(nPassiveProperties).CStr();

    for (int i = 0; i < nPassiveProperties; i++)
    {
        p2DA->GetINTEntry(pnPassiveTypes[i], CExoString("STRINGREF"), &nStringRef);
        g_pTlkTable->Fetch(nStringRef, strRes, CTlkTable::GENDER_MALE);

        sLine = "    ";
        sLine = sLine + strRes.text;
        pGui->AppendToMsgBuffer(sLine, kMessageTypeDebug, 0);

        strRes.text.CStr();
    }

    pGui->AppendToMsgBuffer(CExoString("******************"), kMessageTypeDebug, 0);

    p2DA->Unload2DArray();
    delete p2DA;
    delete[] pnPassiveTypes;

    return 1;
}


int CSWCMessage::HandleServerToPlayerInventory(unsigned char nType)
{
    CGameObject* pObject = 0;

    unsigned long long objId = ReadOBJECTIDClient();
    if (MessageReadOverflow(1))
        return 0;

    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    if (pArray == 0)
        return 0;

    switch (nType)
    {
    case MSG_INVENTORY_EQUIP:
    case MSG_INVENTORY_EQUIP_CANCEL:
    {
        unsigned int nSlot = ReadDWORD(32);
        int bInstant K2_UNSET;
        if (nType == MSG_INVENTORY_EQUIP)
            bInstant = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
        if (nType != MSG_INVENTORY_EQUIP)
            return 1;

        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == 0)
            return 1;
        g_pAppManager->m_pClientExoApp->GetPlayerCreature()->SetLastInstant(nSlot, bInstant);

        if (pArray->GetGameObject(objId, &pObject) != CGameObjectArray::SUCCESS)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;

        unsigned long long idEquipped =
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetEquippedItemID(pItem->m_nInventorySlot);
        if (nSlot != 1 && idEquipped != pItem->m_objectId)
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->EquipToInventorySlot(nSlot, pItem, 0);
        return 1;
    }

    case MSG_INVENTORY_DROP:
    case MSG_INVENTORY_DROP_CANCEL:
    {
        if (MessageReadUnderflow(1))
            return 0;
        if (pArray->GetGameObject(objId, &pObject) != CGameObjectArray::SUCCESS)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;
        if (nType == MSG_INVENTORY_DROP_CANCEL)
        {
            pItem->m_bInventoryRequestPending = 0;
            return 1;
        }

        pItem->PlayDropSound();
        CSWCCreature* pEquippedBy = pItem->GetEquippedBy();
        if (pEquippedBy != 0)
            SwitchWeaponEvent(0, 0, pEquippedBy);
        pItem->m_bInventoryRequestPending = 0;

        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetEquippedItemID(pItem->m_nInventorySlot)
            != pItem->m_objectId)
            return 1;
        if (pItem->m_nInventorySlot == 1)
            return 1;
        g_pAppManager->m_pClientExoApp->GetPlayerCreature()->EquipToInventorySlot(pItem->m_nInventorySlot, 0, 0);
        return 1;
    }

    case MSG_INVENTORY_PICKUP:
    case MSG_INVENTORY_PICKUP_CANCEL:
    {
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
        if (pArray->GetGameObject(objId, &pObject) != CGameObjectArray::SUCCESS)
            return 1;
        if (pObject == 0)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;
        if (nType == MSG_INVENTORY_PICKUP_CANCEL)
        {
            pItem->m_bInventoryRequestPending = 0;
            return 1;
        }

        pItem->PlayDropSound();
        pItem->AttachToScene(0);
        if (pItem->m_nPendingEquipSlot != -1)
        {
            CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
            if (pMessage != 0)
                pMessage->SendPlayerToServerInventory_Equip(pItem->m_objectId, pItem->m_nPendingEquipSlot);
        }
        else
        {
            pItem->m_bInventoryRequestPending = 0;
        }
        pItem->m_nPendingEquipSlot = -1;
        return 1;
    }

    case MSG_INVENTORY_UNEQUIP:
    case MSG_INVENTORY_UNEQUIP_CANCEL:
    {
        // Two separate UNEQUIP tests, so UNEQUIP's jump-table edge stays with UNEQUIP_CANCEL's group.
        int bInstant K2_UNSET;
#if KOTOR1  // K1: one flag, always the player creature, and no store-panel exception
        if (nType == MSG_INVENTORY_UNEQUIP)
            bInstant = ReadBOOL();
#else
        int bParam;
        if (nType == MSG_INVENTORY_UNEQUIP)
            bInstant = ReadBOOL();
        if (nType == MSG_INVENTORY_UNEQUIP)
            bParam = ReadBOOL();
        else
            bParam = 0;
#endif
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
        if (pArray->GetGameObject(objId, &pObject) != CGameObjectArray::SUCCESS)
            return 1;
        if (pObject == 0)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;

        if (nType != MSG_INVENTORY_UNEQUIP_CANCEL && !bInstant)
            pItem->PlayDropSound();
        pItem->m_bInventoryRequestPending = 0;
        if (nType != MSG_INVENTORY_UNEQUIP)
            return 1;

#if KOTOR1
        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetEquippedItemID(pItem->m_nInventorySlot)
            != pItem->m_objectId)
            return 1;
        if (pItem->m_nInventorySlot != 1)
        {
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->SetLastInstant(pItem->m_nInventorySlot, bInstant);
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->EquipToInventorySlot(pItem->m_nInventorySlot, 0);
        }
#else
        // The unequip applies to the party member the top menu overrides to, otherwise to the player.
        CSWCCreature* pCreature;
        if (g_pAppManager->m_pClientExoApp->GetInGameGui() == 0)
        {
            pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        }
        else
        {
            int nCharacter =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
            if (nCharacter == -1)
            {
                pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            }
            else
            {
                CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
                pCreature = static_cast<CSWSCreature*>(pServerApp->GetGameObject(
                    pServerApp->GetPartyTable()->GetNPCObject(nCharacter, 0, 1)))->GetClientCreature();
            }
        }

        if (pCreature->GetEquippedItemID(pItem->m_nInventorySlot) != pItem->m_objectId)
            return 1;
        if (pItem->m_nInventorySlot != 1
            || (g_pGuiMan != 0
                && g_pGuiMan->PanelExists(
                       g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pStorePanel)))
        {
            pCreature->SetLastInstant(pItem->m_nInventorySlot, bInstant);
            pCreature->EquipToInventorySlot(pItem->m_nInventorySlot, 0, bParam);
        }
#endif
        if (pItem->m_nInventorySlot != -1)
            pItem->m_nInventorySlot = -1;
        return 1;
    }

    case MSG_INVENTORY_LEARNED_SCROLL:
    {
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
        pObject = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pObject == 0)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;
        pItem->m_bInventoryRequestPending = 0;
        return 1;
    }

    case MSG_INVENTORY_CONFIRM_DROP:
    {
        if (MessageReadOverflow(1))
            return 0;
        if (MessageReadUnderflow(1))
            return 0;
        if (pArray->GetGameObject(objId, &pObject) != CGameObjectArray::SUCCESS)
            return 1;
        if (pObject == 0)
            return 1;
        CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
        if (pItem == 0)
            return 1;
        pItem->PlayDropSound();
        return 1;
    }

    default:
        return 0;
    }
}

int CSWCMessage::HandleServerToPlayerSafeProjectileSpell()
{
    unsigned long long dummyId;
    Vector vTarget;
    Vector vDirection;

    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;
    vDirection.x = 0.0f;
    vDirection.y = 0.0f;
    vDirection.z = 0.0f;

    if (MessageReadOverflow(1))
        return 0;

    unsigned long long sourceId = ReadOBJECTIDClient();
    unsigned long long targetId = ReadOBJECTIDClient();

    Vector vSource;
    vSource.x = ReadFLOAT(1.0f, 32);
    vSource.y = ReadFLOAT(1.0f, 32);
    vSource.z = ReadFLOAT(1.0f, 32);
    vTarget.x = ReadFLOAT(1.0f, 32);
    vTarget.y = ReadFLOAT(1.0f, 32);
    vTarget.z = ReadFLOAT(1.0f, 32);

    unsigned int  nFlightTime = ReadDWORD(32);
    unsigned char nType       = ReadBYTE(8);

    ReadDWORD(32);
    ReadINT(32);

    if (MessageReadOverflow(1))
        return 0;

    unsigned int nSpellId K2_UNSET;
    unsigned int nProjectileType K2_UNSET;

    if (nType == 6)
    {
        nSpellId = ReadDWORD(32);
    }
    else if (nType == 7)
    {
        nSpellId = ReadDWORD(32);
        nProjectileType = ReadBYTE(8);
    }
    else
    {
        nProjectileType = ReadBYTE(8);
        ReadBYTE(8);
        ReadBYTE(8);
    }

    if (MessageReadOverflow(1))
        return 0;

    CGameObject* pSourceObject = g_pAppManager->m_pClientExoApp->GetGameObject(sourceId);
    CSWCObject*  pSource = pSourceObject ? static_cast<CSWCObject*>(pSourceObject->AsSWCObject()) : 0;

    CGameObject* pTargetObject = g_pAppManager->m_pClientExoApp->GetGameObject(targetId);
    CSWCObject*  pTarget = pTargetObject ? static_cast<CSWCObject*>(pTargetObject->AsSWCObject()) : 0;

    // The direction is only normalized when the source and target differ; a degenerate pair falls
    // back to +X.
    float fDeltaX = vTarget.x - vSource.x;
    float fDeltaY = vTarget.y - vSource.y;
    float fDeltaZ = vTarget.z - vSource.z;

    if (fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ > 1e-5f)
    {
        Vector vDelta;
        vDelta.x = fDeltaX;
        vDelta.y = fDeltaY;
        vDelta.z = fDeltaZ;
        vDirection = normalize(vDelta);
    }
    else
    {
        vDirection.x = 1.0f;
        vDirection.y = 0.0f;
        vDirection.z = 0.0f;
    }

    // The stand-in target. The early exits below leak it.
    if (pTarget == 0 && targetId != CGameObjectArray::INVALIDOBJID)
    {
        CSWCCreature* pDummy = new CSWCCreature();
        pDummy->m_nAppearanceRequestTimer = 0;

        if (pDummy->LoadModel(CResRef("c_dummy"), CSWCAnimBase::ANIM_MODEL_BODY, 0))
        {
            if (g_pAppManager->m_pClientExoApp->GetPlayerCreature())
            {
                CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_pArea;

                if (pArea)
                {
                    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(
                            dummyId, static_cast<CGameObject*>(pDummy), 0) != CGameObjectArray::SUCCESS)
                        return 0;

                    pDummy->SetId(dummyId);
                    pDummy->SetArea(pArea);
                    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(
                        pDummy);
                    pDummy->AttachToScene(pArea->m_pScene);
                    pDummy->SetPosition(vTarget);
                    pDummy->m_desiredPosition = vTarget;
                    pDummy->m_pCreatureAppearance->m_nAppearanceId = kAppearanceDummy;
                    targetId = dummyId;
                }
            }
        }

        pDummy->SetDesiredFadeState(1, 1, nFlightTime + 500, 0);
        pDummy->m_bPendingRemoval = 1;
        pDummy->AsSWCObject();
    }

    if (nType != 6 && nType != 7)
        return 1;

    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);

    if (pSpell == 0)
        return 1;

    Vector     vCastPos = {};
    Quaternion qCastOrientation(1.0f, 0.0f, 0.0f, 0.0f);
    CResRef    resSound;

    vCastPos.x = vSource.x;
    vCastPos.y = vSource.y;
    vCastPos.z = vSource.z + 1.0f;

    if (nType == 6)
        nProjectileType = pSpell->m_nProjectileType;

    if (pSource)
    {
        if (pSource->AsSWCCreature())
        {
            pSource->AsSWCCreature()->m_pSpellCastVisual->LoadCastVisual(
                pSpell->m_castHeadVisual, pSpell->m_castHandVisual, pSpell->m_castGrndVisual,
                pSource->AsSWCCreature());
            pSource->AsSWCCreature()->m_pSpellCastVisual->m_bCastStarted = 1;
        }

        if (pSource->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
        {
            unsigned char nNode = pSpell->m_nCastAttachNode;

            if (nNode == 1)
                pSource->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("handconjure"), &vCastPos,
                                                            &qCastOrientation);
            else if (nNode == 2)
                pSource->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("headconjure"), &vCastPos,
                                                            &qCastOrientation);
            else if (nNode == 3)
                pSource->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("rhand"), &vCastPos,
                                                            &qCastOrientation);
            else
                pSource->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("root"), &vCastPos,
                                                            &qCastOrientation);
        }
    }

    resSound = pSpell->m_castSound;

    if (resSound.GetResRef()[0] != 0)
    {
        CExoSound* pSound = g_pExoSound;
        if (pSource)
            vSource = pSource->m_position;

        pSound->Play3DOneShotSound(resSound, vSource, 1.5f, PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
    }

    if (pSpell->m_bHasProjectile == 0)
        return 1;

    CSWCProjectile* pProjectile = new CSWCProjectile();
    pProjectile->SetProjectileSpellId(nSpellId);

    if (pProjectile->LoadModel(pSpell->m_projectileResRef, CSWCAnimBase::ANIM_MODEL_BODY, 0) == 0)
    {
        delete pProjectile;
        return 1;
    }

    {
        Vector vSpawn = vCastPos;
        pProjectile->SetPosition(vSpawn);
    }

    if (nProjectileType != 7)
    {
        Vector vFlat = {0.0f, 1.0f, 1.0f};
        pProjectile->SetOrientation(vFlat);
    }

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();

    if (pModule && pModule->m_pArea)
        pProjectile->AttachToScene(pModule->m_pArea->m_pScene);

    pProjectile->SetDesiredPositionTimer(nFlightTime);
    pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);
    pProjectile->m_desiredPosition = vTarget;

#if !KOTOR1  // K1's projectile never homes on the target object
    if (pSpell->m_nItemTargeting == 3)
        pProjectile->SetDesiredTargetObject(targetId, CExoString(""));
#endif

    pProjectile->SetProjectileType(nProjectileType);

    if (nProjectileType != 4)
        return 1;

    // The spread type fires four more projectiles from the same cast point, each aimed along the
    // flight direction rather than the flat orientation.
    for (unsigned char i = 0; i < 4; i++)
    {
        pProjectile = new CSWCProjectile();
        pProjectile->SetProjectileSpellId(nSpellId);

        if (pProjectile->LoadModel(pSpell->m_projectileResRef, CSWCAnimBase::ANIM_MODEL_BODY, 0) == 0)
        {
            delete pProjectile;
            continue;
        }

        {
            Vector vSpread = vCastPos;
            pProjectile->SetPosition(vSpread);
        }
        pProjectile->SetOrientation(vDirection);

        CSWCModule* pSpreadModule = g_pAppManager->m_pClientExoApp->GetModule();

        if (pSpreadModule && pSpreadModule->m_pArea)
            pProjectile->AttachToScene(pSpreadModule->m_pArea->m_pScene);

        pProjectile->SetDesiredPositionTimer(nFlightTime);
        pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);
        pProjectile->m_desiredPosition = vTarget;
        pProjectile->SetDesiredTargetObject(targetId, CExoString(""));
        pProjectile->SetProjectileType(4);
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerSafeProjectileProjectile()
{
    unsigned long long sourceId = ReadOBJECTIDClient();
    unsigned long long targetId = ReadOBJECTIDClient();

    Vector vSource = ReadVector();
    Vector vTarget = ReadVector();

    int           nFlightTime  = ReadINT(32);
    unsigned char nShotType    = ReadBYTE(8);
    int           nHand        = ReadINT(32);
    unsigned char nAmmoRow     = ReadBYTE(8);
    unsigned char nAmmoVariant = ReadBYTE(8);
    int           nBoltVisual  = ReadINT(32);

    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(sourceId);

    // A positive guard rather than an early return, so sMuzzlePart's scope stays inside a region the
    // trailing return sits outside of.
    if (pCreature)
    {
        CExoString sMuzzlePart;

        // Seeded with the message's source position and overwritten when a muzzle node is found, so a
        // creature with no bullet node still fires from the position the server sent.
        Vector vMuzzlePos = vSource;

        // Which weapon the shot came out of, and whether the node was found on the creature's own
        // model, in which case the equipped item is never consulted.
        int          bPartOnCreature = 0;
        unsigned int nWeaponSlot     = 0;

        switch (nHand)
        {
        case 0:
            if (pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPart(const_cast<char*>("rbullet")))
            {
                sMuzzlePart     = "rbullet";
                bPartOnCreature = 1;
            }
            else
            {
                sMuzzlePart = "bullethook";
                nWeaponSlot = MSG_SLOT_RIGHT_WEAPON;
            }
            break;

        case 1:
            if (pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPart(const_cast<char*>("lbullet")))
            {
                sMuzzlePart     = "lbullet";
                bPartOnCreature = 1;
            }
            else
            {
                sMuzzlePart = "bullethook";
                nWeaponSlot = MSG_SLOT_LEFT_WEAPON;
            }
            break;

        case 2:
            sMuzzlePart = "impact";
            break;
        }

        Quaternion qMuzzleOrient(1.0f, 0.0f, 0.0f, 0.0f);

        Gob* pMuzzleModel = 0;

        if (bPartOnCreature)
        {
            pMuzzleModel = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pMuzzleModel->GetPartPosition(static_cast<char*>(sMuzzlePart.CStr()), &vMuzzlePos, &qMuzzleOrient);
        }
        else
        {
            if (nHand == 2)
            {
                pMuzzleModel = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            }
            else
            {
                CGameObject* pWeapon = g_pAppManager->m_pClientExoApp->GetGameObject(
                    pCreature->GetEquippedItemID(nWeaponSlot));

                if (pWeapon)
                {
                    if (pWeapon->AsSWCItem())
                        pMuzzleModel = static_cast<CSWCItem*>(pWeapon->AsSWCItem())->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                    else
                        pMuzzleModel = 0;
                }
            }

            if (pMuzzleModel)
                pMuzzleModel->GetPartPosition(static_cast<char*>(sMuzzlePart.CStr()), &vMuzzlePos, &qMuzzleOrient);
        }

        // An if block with a shared trailing return rather than an early return, so the ammo-row
        // cleanup region also covers the fall-through path.
        if (nAmmoRow != 0xff)
        {

            CExoString sModelColumn;
            CExoString sModel;
            CExoString sMuzzleFlash;
            CExoString sShotSoundColumn;
            CExoString sShotSound;
            CExoString sImpactSoundColumn;
            CExoString sImpactSound;

            C2DA* pAmmo = g_pRules->m_p2DArrays->m_pAmmunitionTypes;

            CSWCProjectile* pProjectile = new CSWCProjectile();

            sModelColumn.Format(const_cast<char*>("Model%d"), nAmmoVariant);
            pAmmo->GetCExoStringEntry(nAmmoRow, sModelColumn, &sModel);

            // The failure path deletes the projectile and falls out of the ammo-row block.
            if (pProjectile->LoadModel(CResRef(sModel), CSWCAnimBase::ANIM_MODEL_BODY, 0))
            {

                pProjectile->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 1, 0, CResRef("DEFAULT"));
                pProjectile->SetPosition(vMuzzlePos);

                // A Vector operator- does not exist, so the flight direction is three explicit float
                // subtractions. The inner block ends vDirection's lifetime here.
                {
                    Vector vDirection;
                    vDirection.x = vTarget.x - vSource.x;
                    vDirection.y = vTarget.y - vSource.y;
                    vDirection.z = vTarget.z - vSource.z;
                    pProjectile->SetOrientation(normalize(vDirection));
                }

                pProjectile->m_nProjOrientation = 2;
                pProjectile->SetDesiredPositionTimer(nFlightTime);
                pProjectile->m_nImpactLingerMs = 0;
                pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);

                sImpactSoundColumn.Format(const_cast<char*>("ImpactSound%d"), nAmmoVariant);
                pAmmo->GetCExoStringEntry(nAmmoRow, sImpactSoundColumn, &sImpactSound);
                pProjectile->SetProjectileImpactSound(CResRef(static_cast<char*>(sImpactSound.CStr())));

                switch (nShotType)
                {
                // The blaster arms fire a plain bolt homing on the target's "impact" node. Sub-type
                // 2 also spawns a combat visual where it lands.
                case 1:
                case 2:
                case 3:
                case 5:
                    pProjectile->SetDesiredTargetObject(targetId, CExoString("impact"));
                    pProjectile->m_nImpactMode = 1;
                    pProjectile->SetSpawnCombatVisualOnImpact(1, nShotType == 2, 0);
                    pProjectile->SetProjectileType(1);
                    break;

                // The energy-bolt arms share the target node and the two wire scalars; they differ only
                // in where the impact point comes from.
                case 8:
                case 9:
                case 10:
                    pProjectile->m_nImpactMode = 1;
                    pProjectile->m_sourceObjectId = sourceId;
                    pProjectile->m_nDeflectFlightTime = nBoltVisual;
                    pProjectile->SetDesiredTargetObject(targetId, CExoString("impact_bolt"));

                    if (nShotType == 10)
                    {
                        // When deflected off a force shield, the shield's own 2DA row supplies a
                        // deflection radius, found by walking the Appearance_0N columns until one
                        // matches the target's appearance.
                        float fRadius = 0.0f;

                        CSWCCreature* pTarget =
                            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(targetId);

                        if (pTarget)
                        {
                            pProjectile->SetArea(pTarget->m_pArea);

                            unsigned short nAppearance = pTarget->m_pCreatureAppearance->m_nAppearanceId;
                            int            nShieldRow  = pTarget->GetForceShield();

                            int        nColumnValue = 0;
                            int        nColumn      = 0;
                            CExoString sShieldColumn;

                            // The scan exits with 5 when nothing matched, so the radius lookup below then
                            // asks for a "Radius_05" column that does not exist.
                            while (nColumnValue != nAppearance && ++nColumn <= 4)
                            {
                                sShieldColumn.Format(const_cast<char*>("Appearance_0%d"), nColumn);
                                g_pRules->m_p2DArrays->m_pForceShields->GetINTEntry(nShieldRow, sShieldColumn,
                                                                                    &nColumnValue);
                            }

                            sShieldColumn.Format(const_cast<char*>("Radius_0%d"), nColumn);
                            g_pRules->m_p2DArrays->m_pForceShields->GetFLOATEntry(nShieldRow, sShieldColumn,
                                                                                  &fRadius);
                        }

                        pProjectile->m_vImpactPosition = vTarget;
                        pProjectile->m_fShieldRadius   = fRadius;
                        pProjectile->SetSpawnCombatVisualOnImpact(1, 0, 0);
                        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_REFLECTED);
                    }
                    else if (nShotType == 8)
                    {
                        pProjectile->m_vImpactPosition = vTarget;
                        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_PARRIED);
                    }
                    else
                    {
                        // Sets the impact flag a second time, after the arm's shared prefix.
                        pProjectile->m_nImpactMode = 1;
                        pProjectile->m_vImpactPosition   = pCreature->m_position;
                        pProjectile->SetSpawnCombatVisualOnImpact(1, 0, 0);
                        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_DEFLECTED);
                    }
                    break;

                // Everything else flies at a fixed point with no target object at all.
                default:
                    pProjectile->SetDesiredTargetObject(OBJECT_INVALID, CExoString(""));
                    pProjectile->m_desiredPosition = vTarget;
                    pProjectile->SetProjectileType(1);
                    break;
                }

                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();

                if (pModule && pModule->m_pArea)
                {
                    CAurScene* pScene = pModule->m_pArea->m_pScene;
                    pProjectile->AttachToScene(pScene);
                    pAmmo->GetCExoStringEntry(nAmmoRow, CExoString("MuzzleFlash"), &sMuzzleFlash);
                    pProjectile->CreateMuzzleFlash(sMuzzleFlash, static_cast<CAurObject*>(pMuzzleModel), sMuzzlePart);
                }

                sShotSoundColumn.Format(const_cast<char*>("ShotSound%d"), nAmmoVariant);
                pAmmo->GetCExoStringEntry(nAmmoRow, sShotSoundColumn, &sShotSound);
                g_pExoSound->Play3DOneShotSound(CResRef(static_cast<char*>(sShotSound.CStr())), vMuzzlePos, 0.0f, PRIORITY_GROUP_COMBAT, 0, 0,
                                                0.0f, 0.0f);

                return 1;
            }

            delete pProjectile;
        }
    }

    return 0;
}

int CSWCMessage::HandleServerToPlayerUpdate_CharacterSheet()
{
    unsigned long long objId = ReadOBJECTIDClient();

    CGameObject* pObject;
    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(objId, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;

    CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObject->AsSWCCreature());
    CSWCCreatureStats* pStats = pCreature->m_pStats;

    unsigned int nMask = ReadDWORD(32);
    if (MessageReadOverflow(1))
        return 0;

    if (nMask & MSG_CHARACTER_SHEET_ABILITIES) {
        unsigned char nStr = ReadBYTE(8);
        unsigned char nDex = ReadBYTE(8);
        unsigned char nCon = ReadBYTE(8);
        unsigned char nInt = ReadBYTE(8);
        unsigned char nWis = ReadBYTE(8);
        unsigned char nCha = ReadBYTE(8);
        char nStrBonus = ReadCHAR(8);
        char nDexBonus = ReadCHAR(8);
        char nConBonus = ReadCHAR(8);
        char nIntBonus = ReadCHAR(8);
        char nWisBonus = ReadCHAR(8);
        char nChaBonus = ReadCHAR(8);
        char nStrBase = ReadCHAR(8);
        char nDexBase = ReadCHAR(8);
        char nConBase = ReadCHAR(8);
        char nIntBase = ReadCHAR(8);
        char nWisBase = ReadCHAR(8);
        char nChaBase = ReadCHAR(8);
        char nDexSkillMod = ReadCHAR(8);
        if (MessageReadOverflow(1))
            return 0;

        pStats->m_abScores[0] = nStr;
        pStats->m_abScores[1] = nDex;
        pStats->m_abScores[2] = nCon;
        pStats->m_abScores[3] = nInt;
        pStats->m_abScores[4] = nWis;
        pStats->m_abScores[5] = nCha;
        pStats->m_abScoresBase[0] = nStrBase;
        pStats->m_abScoresBase[1] = nDexBase;
        pStats->m_abScoresBase[2] = nConBase;
        pStats->m_abScoresBase[3] = nIntBase;
        pStats->m_abScoresBase[4] = nWisBase;
        pStats->m_abScoresBase[5] = nChaBase;
        pStats->m_nDEXArmorAdjust = nDexSkillMod;
        pStats->m_abScoreBonuses[0] = nStrBonus;
        pStats->m_abScoreBonuses[1] = nDexBonus;
        pStats->m_abScoreBonuses[2] = nConBonus;
        pStats->m_abScoreBonuses[3] = nIntBonus;
        pStats->m_abScoreBonuses[4] = nWisBonus;
        pStats->m_abScoreBonuses[5] = nChaBonus;
    }

    if (nMask & MSG_CHARACTER_SHEET_FORTITUDE_SAVE) {
        char nSave = ReadCHAR(8);
        if (MessageReadOverflow(1))
            return 0;
        pStats->m_nFortSavingThrow = nSave;
    }

    if (nMask & MSG_CHARACTER_SHEET_WILL_SAVE) {
        char nSave = ReadCHAR(8);
        if (MessageReadOverflow(1))
            return 0;
        pStats->m_nWillSavingThrow = nSave;
    }

    if (nMask & MSG_CHARACTER_SHEET_REFLEX_SAVE) {
        char nSave = ReadCHAR(8);
        if (MessageReadOverflow(1))
            return 0;
        pStats->m_nReflexSavingThrow = nSave;
    }

    if (nMask & MSG_CHARACTER_SHEET_EXPERIENCE) {
        int nExperience = ReadDWORD(32);
        if (MessageReadOverflow(1))
            return 0;
        pStats->m_nExperience = nExperience;
    }

    if (nMask & MSG_CHARACTER_SHEET_BASE_ATTACK_BONUS) {
        unsigned char nBaseAttackBonus = ReadBYTE(8);
        int bMonkAttackTables = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;
        pStats->m_nBaseAttackBonus = nBaseAttackBonus;
        pStats->m_bUseMonkAttackTables = bMonkAttackTables;
    }

    if (nMask & MSG_CHARACTER_SHEET_COMBAT_INFORMATION) {
        CCombatInformation* pCombatInfo = new CCombatInformation();

        unsigned char nNumOnHandAttacks = ReadBYTE(3);
        char nAttackModOnHand = ReadCHAR(8);
        char nStrDamageModOnHand = ReadCHAR(8);
        char nForceResistance = ReadCHAR(8);
        ReadBYTE(7);                       // arcane spell failure is shipped and dropped
        unsigned char nArmorCheckPenalty = ReadBYTE(5);
        unsigned char nDefaultDamageDie = ReadBYTE(5);
        unsigned char nDefaultDamageDiceCount = ReadBYTE(5);
        unsigned char nCritRangeOnHand = ReadBYTE(4);
        unsigned char nCritMultiplierOnHand = ReadBYTE(3);
        if (MessageReadOverflow(1))
            return 0;

        pCombatInfo->m_nNumOnHandAttacks = nNumOnHandAttacks;
        pCombatInfo->m_nAttackModOnHand = nAttackModOnHand;
        pCombatInfo->m_nStrDamageModOnHand = nStrDamageModOnHand;
        pCombatInfo->m_nForceResistance = nForceResistance;
        pCombatInfo->m_bArmorCheckPenalty = nArmorCheckPenalty;
        pCombatInfo->m_nDefaultDamageDie = nDefaultDamageDie;
        pCombatInfo->m_nDefaultDamageDiceCount = nDefaultDamageDiceCount;
        pCombatInfo->m_nCritRangeOnHand = nCritRangeOnHand;
        pCombatInfo->m_nCritMultiplierOnHand = nCritMultiplierOnHand;

        int bOffHand = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;
        pCombatInfo->m_bOffHandWeaponEquipped = bOffHand;

        if (bOffHand == 1) {
            char nAttackModOffHand = ReadCHAR(8);
            char nStrDamageModOffHand = ReadCHAR(8);
            unsigned char nCritRangeOffHand = ReadBYTE(4);
            unsigned char nCritMultiplierOffHand = ReadBYTE(3);
            if (MessageReadOverflow(1))
                return 0;
            pCombatInfo->m_nAttackModOffHand = nAttackModOffHand;
            pCombatInfo->m_nStrDamageModOffHand = nStrDamageModOffHand;
            pCombatInfo->m_nCritRangeOffHand = nCritRangeOffHand;
            pCombatInfo->m_nCritMultiplierOffHand = nCritMultiplierOffHand;
        }

        unsigned char nNumAttackMods = ReadBYTE(8);
        if (nNumAttackMods != 0) {
            int i = 0;
            do {
                char nValue = ReadCHAR(8);
                unsigned char nType = ReadBYTE(3);
                unsigned char nClassReq;
                if (ReadBOOL() == 1)
                    nClassReq = ReadBYTE(8);
                else
                    nClassReq = g_pRules->m_nNumRaces;
                unsigned char nFlag;
                if (ReadBOOL() == 1)
                    nFlag = ReadBYTE(3);
                else
                    nFlag = 0;
                if (MessageReadOverflow(1))
                    return 0;

                CCombatInformationNode* pNode = new CCombatInformationNode();
                pNode->m_cValue = nValue;
                pNode->m_nType = nType;
                pNode->m_nClassReq = nClassReq;
                pNode->m_nFlag = nFlag;

                pCombatInfo->m_attackBonusMods.Add(pNode);

                i++;
            } while (i != nNumAttackMods);
        }

        unsigned char nNumDamageMods = ReadBYTE(8);
        if (nNumDamageMods != 0) {
            int i = 0;
            do {
                char nValue = ReadCHAR(8);
                unsigned char nDamageSubType = ReadBYTE(4);
                unsigned char nType = ReadBYTE(3);
                unsigned char nClassReq;
                if (ReadBOOL() == 1)
                    nClassReq = ReadBYTE(8);
                else
                    nClassReq = g_pRules->m_nNumRaces;
                unsigned char nFlag;
                if (ReadBOOL() == 1)
                    nFlag = ReadBYTE(3);
                else
                    nFlag = 0;
                if (MessageReadOverflow(1))
                    return 0;

                CCombatInformationNode* pNode = new CCombatInformationNode();
                pNode->m_cValue = nValue;
                pNode->m_nModifierType = nDamageSubType;
                pNode->m_nType = nType;
                pNode->m_nClassReq = nClassReq;
                pNode->m_nFlag = nFlag;

                pCombatInfo->m_damageBonusMods.Add(pNode);

                i++;
            } while (i != nNumDamageMods);
        }

        pStats->SetCombatInformation(pCombatInfo);
    }

    if (nMask & MSG_CHARACTER_SHEET_ARMOR_CLASS) {
        short nArmorClass = ReadSHORT(16);
        pStats->m_nArmorClass = nArmorClass;
    }

    if (nMask & MSG_CHARACTER_SHEET_SKILLS) {
        unsigned int nSkillMask = ReadDWORD(g_pRules->m_nNumSkills);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned short i = 0; i < g_pRules->m_nNumSkills; i++) {
            if ((nSkillMask & (1 << i)) == 0)
                continue;

            char nRank = ReadCHAR(8);
            int bCanUse = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;

            // The rank bound is re-read between the two stores.
            if (i < g_pRules->m_nNumSkills)
                pStats->m_pSkillRanks[i] = nRank;

            // The base rank is read through a bounds-checked accessor and the store re-checks the bound
            // on its own, so the two are not one guarded block.
            signed char nBase = i < g_pRules->m_nNumSkills
                                    ? static_cast<signed char>(pCreature->m_pStats->m_pSkillRanksBase[i])
                                    : 0;
            if (bCanUse & 1) {
                if (nBase <= 0 && i < g_pRules->m_nNumSkills)
                    pCreature->m_pStats->m_pSkillRanksBase[i] = 1;
            } else {
                if (nBase >= 1 && i < g_pRules->m_nNumSkills)
                    pCreature->m_pStats->m_pSkillRanksBase[i] = 0;
            }
        }
    }

    if (nMask & MSG_CHARACTER_SHEET_FEATS) {
        pStats->ClearFeats();

        unsigned short nNumFeats = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned short i = 0; i < nNumFeats; i++) {
            unsigned short nFeat = ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;

            // inlined CExoArrayList<unsigned short>::AddUnique(unsigned short)
            if (pStats->m_lstFeats.Contains(nFeat) != 0)
                continue;
            pStats->m_lstFeats.Add(nFeat);
        }

        unsigned short nNumBonusFeats = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned short i = 0; i < nNumBonusFeats; i++) {
            unsigned short nFeat = ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;

            // inlined CExoArrayList<unsigned short>::AddUnique(unsigned short)
            if (pStats->m_lstBonusFeats.Contains(nFeat) != 0)
                continue;
            pStats->m_lstBonusFeats.Add(nFeat);
        }
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerCreatureUpdate_Appearance()
{
    CExoString sFirstName;
    CExoString sLastName;

    unsigned long long oidObject = ReadOBJECTIDClient();
    unsigned short nFieldMask = ReadWORD(16);

    if (MessageReadOverflow(1))
        return 0;

    CGameObjectArray* pObjectArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oidObject);
    if (pCreature == NULL)
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == NULL)
        return 0;
    CSWCArea* pArea = pModule->m_pArea;
    if (pArea == NULL)
        return 0;

    if (nFieldMask & MSG_APPEARANCE_UPDATE_NAME)
    {
        sFirstName = ReadCExoLocStringClient();
        sLastName = ReadCExoLocStringClient();
        if (MessageReadOverflow(1))
            return 0;
        pCreature->SetName(sFirstName, sLastName);
    }

    if (MessageReadOverflow(1))
        return 0;

    // The appearance is edited on a copy of the creature's live record and only committed by
    // SetAppearance/RequestAppearance at the bottom, which also lets the change-flag comparison below
    // diff the two.
    CCAppearanceInfo appearance;
    appearance.core = *reinterpret_cast<CCAppearanceCore*>(pCreature->m_pCreatureAppearance);  // idiom-exempt: the same layout-mirror aggregate copy CSWCCreatureAppearance::CreateSimple @0x3d2e38 does in the other direction; the orig emits it as one paired-NEON ldp/stp of 0x40 bytes, not field-by-field

    if (nFieldMask & MSG_APPEARANCE_UPDATE_APPEARANCE_ID)
    {
        unsigned short nAppearanceId = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        // A new appearance row means a new creature sound set: drop the looping sound and re-resolve
        // appearance.2da's sound-appearance-type column.
        if (nAppearanceId != appearance.core.m_nAppearanceId)
        {
            if (pCreature->m_pLoopingCreatureSound != NULL)
                pCreature->DeleteLoopingCreatureSound();

            int nSoundAppType;
            g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                nAppearanceId, CExoString("soundapptype"), &nSoundAppType);

            CExoString sLoopingSound;
            if (g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                    nSoundAppType, CExoString("Looping"), &sLoopingSound))
            {
                pCreature->SetNewLoopingCreatureSound(sLoopingSound);
            }
        }
        appearance.core.m_nAppearanceId = nAppearanceId;
    }
    if (nFieldMask & MSG_APPEARANCE_UPDATE_PHENOTYPE)
        appearance.core.m_nPhenotype = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_GENDER)
        appearance.core.m_nGender = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_HEAD)
        appearance.core.m_nHeadNumber = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_USE_BACKUP_HEAD)
        appearance.core.m_bUseBackupHead = ReadBOOL();
    if (nFieldMask & MSG_APPEARANCE_UPDATE_DUPLICATING_HEAD)
        appearance.core.m_race = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_TAIL)
        appearance.core.m_tailVariant = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_WINGS)
        appearance.core.m_wingVariant = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_SKIN_COLOR)
        appearance.core.m_nSkinColor = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_HAIR_COLOR)
        appearance.core.m_nHairColor = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_TATTOO_COLOR_1)
        appearance.core.m_nTattooColor1 = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_TATTOO_COLOR_2)
        appearance.core.m_nTattooColor2 = ReadBYTE(8);
    if (nFieldMask & MSG_APPEARANCE_UPDATE_SOUND_SET)
        pCreature->SetSoundSet(ReadWORD(16));

    // Which parts of the model actually have to be rebuilt. The full value means "everything" (the
    // row/gender/variant triple moved), and the three narrow bits below can only ever add to it.
    CSWCCreatureAppearance* pAppearance = pCreature->m_pCreatureAppearance;
    unsigned char nRebuildFlags;
    int bAppearanceChanged;
    if (appearance.core.m_nAppearanceId == pAppearance->m_nAppearanceId &&
        pAppearance->m_nPhenotype == appearance.core.m_nPhenotype &&
        pAppearance->m_nGender == appearance.core.m_nGender)
    {
        bAppearanceChanged = 0;
        nRebuildFlags = 0;
    }
    else
    {
        nRebuildFlags = kRebuildAll;
        bAppearanceChanged = 1;
    }
    if (appearance.core.m_nHeadNumber != pAppearance->m_nHeadNumber ||
        appearance.core.m_race != pAppearance->m_race ||
        appearance.core.m_bUseBackupHead != pAppearance->m_bUseBackupHead)
        nRebuildFlags |= kRebuildHead;
    if (appearance.core.m_tailVariant != pAppearance->m_cachedTailVariant)
        nRebuildFlags |= kRebuildTail;
    if (appearance.core.m_wingVariant != pAppearance->m_cachedWingVariant)
        nRebuildFlags |= kRebuildWings;

    // Null while the creature's body model has not been built yet, which is the window the
    // hand-attached weapon path below exists for.
    Gob* pBodyModel = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0);

    CSWCItem* pRightHandItem = NULL;
    CSWCItem* pLeftHandItem = NULL;
    int bRightHandTouched = 0;
    int bLeftHandTouched = 0;
    int nRightHandInstant = 0;
    int nLeftHandInstant = 0;

    if (nFieldMask & MSG_APPEARANCE_UPDATE_EQUIPMENT)
    {
        unsigned char nItems = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned char i = 0; i < nItems; i++)
        {
            char cAction = ReadCHAR(8);
            oidObject = ReadOBJECTIDClient();
            unsigned int nSlot = ReadDWORD(32);
            if (nSlot == MSG_SLOT_RIGHT_WEAPON)
                nRightHandInstant = ReadBOOL();
            else if (nSlot == MSG_SLOT_LEFT_WEAPON)
                nLeftHandInstant = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;

            if (cAction == 'D')
            {
                unsigned char nBit = 0;
                if (nSlot == MSG_SLOT_HEAD)
                {
                    appearance.core.m_oidHeadItem = CGameObjectArray::INVALIDOBJID;
                    nBit = kRebuildHead;
                }
                else if (nSlot == MSG_SLOT_BODY)
                {
                    appearance.core.m_aBodyParts[0] = 0;
                    appearance.core.m_aBodyParts[1] = 0;
                    appearance.core.m_aBodyParts[2] = 0;
                    appearance.core.m_aBodyParts[3] = 0;
                    appearance.core.m_aBodyParts[4] = 0;
                    appearance.core.m_aBodyParts[5] = 0;
                    pCreature->GetArmourClass();
                    appearance.core.m_bArmorEquipped = 0;
                    appearance.core.m_bTypeVariant = 0;
                    nBit = kRebuildBody;
                }
                nRebuildFlags |= nBit;

                // The two weapon hands are not unequipped here -- they are only remembered, so the
                // single pass at the bottom can decide between a real unequip and a bare detach.
                if (nSlot == MSG_SLOT_LEFT_WEAPON)
                {
                    pLeftHandItem = NULL;
                    bLeftHandTouched = 1;
                }
                else if (nSlot == MSG_SLOT_RIGHT_WEAPON)
                {
                    pRightHandItem = NULL;
                    bRightHandTouched = 1;
                }
                else
                {
                    pCreature->EquipToInventorySlot(nSlot, NULL, 0);
                }
            }
            else if (cAction == 'A')
            {
                unsigned int nBaseItem;
                unsigned char nAppearance;
                unsigned char nBodyVariation;
                unsigned char nTextureVariation;
                HandleServerToPlayerItemAppearance(&nBaseItem, &nAppearance,
                                                   &nBodyVariation, &nTextureVariation);

                CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidObject);
                if (pItem == NULL)
                {
                    // Not in the object array yet, so build it locally and add it under the
                    // server's id. The gender byte picks the male or female art variant.
                    if (appearance.core.m_nGender == 0)
                        pItem = CreateItem(nBaseItem, nAppearance, 'M');
                    else
                        pItem = CreateItem(nBaseItem, nAppearance, 'F');
                    if (pItem == NULL)
                        continue;
                    pItem->SetId(oidObject);
                    if (pObjectArray->AddExternalObject(
                            oidObject, static_cast<CGameObject*>(static_cast<CSWCObject*>(pItem)), 1) !=
                        CGameObjectArray::SUCCESS)
                        continue;
                }
                else if (g_pRules->GetBaseItem(pItem->m_baseItem) == NULL)
                {
                    continue;
                }

                HandleServerToPlayerActiveItemProperties(oidObject);

                CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
                if (pBaseItem->m_nModelType == 1)
                {
                    appearance.core.m_bTypeVariant = nBodyVariation;
                    appearance.core.m_bTypeNumber = nTextureVariation;
                }

                // Kreia and Visas keep their own head-slot art; the Handmaiden only does so while she
                // wears appearance row 454. For those three the head model is left alone.
#if KOTOR1  // K1: no per-companion head-slot exceptions
                int bGenericHead = 1;
#else
                CSWSCreature* pServerCreature = pCreature->GetServerCreature();
                int bGenericHead = 1;
                if (pServerCreature != NULL)
                {
                    if (pServerCreature->m_sTag.CompareNoCase("kreia") == 0 &&
                        pServerCreature->m_sTag.CompareNoCase("visasmarr") == 0)
                    {
                        bGenericHead = pServerCreature->m_sTag.CompareNoCase("handmaiden") == 0 ||
                                       appearance.core.m_nAppearanceId != APPEARANCE_HANDMAIDEN;
                    }
                    else
                    {
                        bGenericHead = 0;
                    }
                }
#endif

                // Only the head and weapon-hand slots carry items with their own model. The load-or-keep
                // predicate is evaluated once into bUseItemModel and tested on both sides of the model
                // check.
                if (nSlot == MSG_SLOT_HEAD || nSlot == MSG_SLOT_RIGHT_WEAPON || nSlot == MSG_SLOT_LEFT_WEAPON)
                {
                    int bUseItemModel = bGenericHead || nSlot != 1;
                    if (pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1) == NULL)
                    {
                        if (bUseItemModel)
                        {
                            pItem->LoadModel(pItem->m_modelResRef, CSWCAnimBase::ANIM_MODEL_BODY, 0);
                            if (pBaseItem->m_bHasEnvMap == 1)
                                pItem->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 1, CResRef("default"));
                        }
                    }
                    else if (!bUseItemModel)
                    {
                        pItem->SetEnvMap(static_cast<unsigned char>(CSWCAnimBase::ANIM_MODEL_BODY), 0, 1, CResRef("DEFAULT"));
                        pItem->UnloadModel(static_cast<unsigned char>(CSWCAnimBase::ANIM_MODEL_BODY));
                    }
                }

                pItem->AttachToScene(pArea->m_pScene);

                if (nSlot == MSG_SLOT_BODY)
                {
                    // For body armour the item's model is not drawn; its six part numbers are
                    // folded into the creature's appearance instead.
                    pItem->AttachToScene(NULL);
                    pCreature->GetArmourClass();
                    appearance.core.m_bArmorEquipped = 1;
                    appearance.core.m_aBodyParts[0] = pItem->m_aModelParts[0];
                    appearance.core.m_aBodyParts[1] = pItem->m_aModelParts[1];
                    appearance.core.m_aBodyParts[2] = pItem->m_aModelParts[2];
                    appearance.core.m_aBodyParts[3] = pItem->m_aModelParts[3];
                    appearance.core.m_aBodyParts[4] = pItem->m_aModelParts[4];
                    appearance.core.m_aBodyParts[5] = pItem->m_aModelParts[5];
                    pCreature->EquipToInventorySlot(MSG_SLOT_BODY, pItem, 0);
                    nRebuildFlags |= kRebuildBody;
                }
                else if (nSlot == MSG_SLOT_HEAD)
                {
                    pItem->AttachToScene(NULL);
                    appearance.core.m_oidHeadItem = oidObject;
                    pCreature->EquipToInventorySlot(MSG_SLOT_HEAD, pItem, 0);
                    nRebuildFlags |= kRebuildHead;
                }
                else if (nSlot == MSG_SLOT_RIGHT_WEAPON)
                {
                    pRightHandItem = pItem;
                    bRightHandTouched = 1;
                }
                else if (nSlot == MSG_SLOT_LEFT_WEAPON)
                {
                    pLeftHandItem = pItem;
                    bLeftHandTouched = 1;
                }
            }
        }
    }

    if (nRebuildFlags != 0)
    {
        if (enablemodelrequest)
        {
            if (g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay())
            {
                // Off-screen, rebuild synchronously and completely.
                if (pCreature->SetAppearance(&appearance, kRebuildAll,
                                             !(bLeftHandTouched || bRightHandTouched), 0) == 0)
                    return 0;
            }
            else
            {
                if (pCreature->RequestAppearance(&appearance, nRebuildFlags) == 0)
                    return 0;
                // The async request's frame budget is one tick while paused, half a second
                // otherwise.
                if (g_pAppManager->m_pClientExoApp->GetPauseState(1) == 0 &&
                    g_pAppManager->m_pClientExoApp->GetPauseState(2) == 0)
                    pCreature->m_nAppearanceRequestTimer = 500;
                else
                    pCreature->m_nAppearanceRequestTimer = 1;
            }
        }
        else if (pCreature->SetAppearance(&appearance, nRebuildFlags,
                                          !(bLeftHandTouched || bRightHandTouched), 0) == 0)
        {
            return 0;
        }
    }

    // While a model request is outstanding the live record keeps the old row and the creature
    // remembers the new one, so the pending request can be recognised when it lands.
    if (enablemodelrequest)
    {
        if (pCreature->m_nAppearanceRequestTimer == 0 ||
            appearance.core.m_nAppearanceId != pCreature->m_pCreatureAppearance->m_nAppearanceId)
        {
            pCreature->m_nAppearanceId = pCreature->m_pCreatureAppearance->m_nAppearanceId;
            pCreature->m_pCreatureAppearance->m_nAppearanceId = appearance.core.m_nAppearanceId;
        }
    }

    if (bLeftHandTouched)
    {
        if (pLeftHandItem != NULL)
        {
            // Only detach the incoming item when it is not already one of the creature's own two weapon
            // models, since re-attaching those would drop them out of the scene. The guard is a bitwise
            // or, so all four lookups run.
            if (static_cast<int>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY, 1) != pLeftHandItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)) &
                static_cast<int>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY, 1) != pLeftHandItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)))
                pLeftHandItem->AttachToScene(NULL);

            pCreature->SetLastInstant(MSG_SLOT_LEFT_WEAPON, nLeftHandInstant);
            if (pBodyModel == NULL)
            {
                // With no body model yet, EquipToInventorySlot has nothing to hang the weapon on,
                // so bind it by hand and let the renderer's switch-weapon hook place it on load.
                pLeftHandItem->SetEquippedBy(pCreature);
                pCreature->SetEquippedItemID(MSG_SLOT_LEFT_WEAPON, pLeftHandItem->m_objectId);
                pCreature->UpdateWeaponClass();
                SwitchWeaponEvent(0, const_cast<char*>("Loading"), pCreature);
            }
            else
            {
                pCreature->EquipToInventorySlot(MSG_SLOT_LEFT_WEAPON, pLeftHandItem, 0);
            }
        }
        else
        {
            pCreature->SetLastInstant(MSG_SLOT_LEFT_WEAPON, nLeftHandInstant);
            pCreature->EquipToInventorySlot(MSG_SLOT_LEFT_WEAPON, NULL, 0);
        }
    }

    if (bRightHandTouched)
    {
        if (pRightHandItem != NULL)
        {
            if (static_cast<int>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY, 1) != pRightHandItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)) &
                static_cast<int>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY, 1) != pRightHandItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)))
                pRightHandItem->AttachToScene(NULL);

            pCreature->SetLastInstant(MSG_SLOT_RIGHT_WEAPON, nRightHandInstant);
            if (pBodyModel == NULL)
            {
                pRightHandItem->SetEquippedBy(pCreature);
                pCreature->SetEquippedItemID(MSG_SLOT_RIGHT_WEAPON, pRightHandItem->m_objectId);
                pCreature->UpdateWeaponClass();
                SwitchWeaponEvent(0, const_cast<char*>("Loading"), pCreature);
            }
            else
            {
                pCreature->EquipToInventorySlot(MSG_SLOT_RIGHT_WEAPON, pRightHandItem, 0);
            }
            pCreature->SetLastInstant(MSG_SLOT_RIGHT_WEAPON, 0);
        }
        else
        {
            pCreature->SetLastInstant(MSG_SLOT_RIGHT_WEAPON, nRightHandInstant);
            pCreature->EquipToInventorySlot(MSG_SLOT_RIGHT_WEAPON, NULL, 0);
        }
    }

    if (bRightHandTouched || bLeftHandTouched)
    {
        pCreature->DetachVisualEffects();
        pCreature->ReAttachVisualEffects();
    }
    pCreature->UpdateShadowBlob();

    if (bAppearanceChanged)
        pCreature->AttachToScene(pArea->m_pScene);

    if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == pCreature)
    {
        if (enablemodelrequest)
        {
            if (pCreature->m_nAppearanceRequestTimer != 0)
                pCreature->m_bCameraReattachPending = 1;
        }
        else if (bAppearanceChanged)
        {
            if (pModule->m_pCamera == NULL)
                return 0;
            pModule->SetCameraFollow(pCreature->m_objectId, 0);
        }
    }
    else if (g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() == pCreature->m_objectId)
    {
        // The player's creature id points at us but the cached pointer does not -- the party leader
        // just changed body, so re-point the player record and re-seat the camera.
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        CSWCPlayer* pPlayer = pApp->GetPlayerByGameObjectID(pApp->GetPlayerCreatureId());
        if (pPlayer != NULL)
            pPlayer->m_gameObjectId = pCreature->m_objectId;
        g_pAppManager->m_pClientExoApp->SetPlayerCreature(pCreature->m_objectId);

        pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule == NULL || pModule->m_pCamera == NULL)
            return 0;

        g_pAppManager->m_pClientExoApp->GetPlayerByGameObjectID(oidObject);
        g_pAppManager->m_pClientExoApp->AttachPlayerGameObjectToArea();
        if (enablemodelrequest)
        {
            if (pCreature->m_nAppearanceRequestTimer != 0)
                pCreature->m_bCameraReattachPending = 1;
        }
        else
        {
            pModule->SetCameraFollow(pCreature->m_objectId, 0);
        }
    }

    // While still model-less, the queued looping animation has to be re-armed by hand, since the
    // normal path runs off the model's own animation callbacks.
    if (pBodyModel == NULL)
    {
        unsigned short nAnimation = pCreature->m_nInitialAnimation;
        if (pCreature->AnimationLooping(nAnimation))
            pCreature->SetLoopingAnimation(nAnimation);
        pCreature->SetAnimation(nAnimation, 0, 0);
    }

    return 1;
}

int CSWCMessage::HandleServerToPlayerUpdate_PlayerInfo()
{
    unsigned long long objId = ReadOBJECTIDClient();
    unsigned int nMask = ReadDWORD(32);
    if (MessageReadOverflow(1))
        return 0;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    CSWCCreatureStats* pStats = NULL;
    if (pCreature)
        pStats = pCreature->m_pStats;
#ifdef K2_FIX_ENGINE_BUGS
    if (!pCreature && (nMask & (MSG_PLAYER_UPDATE_RESET_STATS | MSG_PLAYER_UPDATE_SKILL_RANKS |
                                MSG_PLAYER_UPDATE_PORTRAIT | MSG_PLAYER_UPDATE_HIT_POINTS |
                                MSG_PLAYER_UPDATE_FORCE_POINTS)))
        return 0;
#endif

    // --- bit 12: wipe the derived stat blocks before the deltas below rebuild them.
    if (nMask & MSG_PLAYER_UPDATE_RESET_STATS) {
        pStats->ClearFeats();
        pStats->ClearKnownSpells();
        pCreature->DeleteSpellLikeAbilityList();
        if (pCreature)
            pCreature->ClearVisibilityList();
        g_pAppManager->m_pClientExoApp->GetInGameGui();
    }

    // --- bit 0: party resources + experience, then the level-up sub-block.
    if (nMask & MSG_PLAYER_UPDATE_CHARACTER) {
        short nArmorClass = ReadSHORT(16);
        unsigned int nGold = ReadDWORD(32);
#if !KOTOR1
        unsigned int nItemComponent = ReadDWORD(32);
        unsigned int nChemicals = ReadDWORD(32);
#endif
        int nEncumbranceState = ReadINT(32);
        int bLevelUp = ReadBOOL();
        unsigned int nExperience = ReadDWORD(32);
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature) {
            pCreature->m_pStats->m_nExperience = nExperience;
            pCreature->m_pStats->m_nArmorClass = nArmorClass;
            pCreature->m_nGold = nGold;
#if !KOTOR1
            pCreature->m_nItemComponent = nItemComponent;
            pCreature->m_nChemicals = nChemicals;
#endif
            pCreature->m_nPlayerInfoExtra = nEncumbranceState;
        }

        g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();

        CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        if (pGui)
            pGui->SetLevelUpMode(bLevelUp);

        if (bLevelUp == 1) {
            unsigned short nSkillPoints = ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature)
                pCreature->m_pStats->m_nSkillPointsRemaining = nSkillPoints;

            // The skill bound is re-tested inside the body against a re-read of the rules count.
            for (unsigned short i = 0; i < g_pRules->m_nNumSkills; i++) {
                char nRank = ReadCHAR(8);
                if (MessageReadOverflow(1))
                    return 0;
                if (pCreature && i < g_pRules->m_nNumSkills)
                    pCreature->m_pStats->m_pSkillRanksBase[i] = nRank;
            }

            unsigned char nStartingPackage = ReadBYTE(8);
            ReadBYTE(8);
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature)
                pCreature->m_pStats->m_nStartingPackage = nStartingPackage;
        }
    }

    // --- bit 1: three gui skill toggles off one selection word.
    if (nMask & MSG_PLAYER_UPDATE_GUI_SKILLS) {
        unsigned short nSkillMask = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;
        if (pCreature) {
            SetGuiSkillRank(pCreature, nSkillMask, MSG_GUI_SKILL_DEMOLITIONS, SKILL_DEMOLITIONS);
            SetGuiSkillRank(pCreature, nSkillMask, MSG_GUI_SKILL_SECURITY, SKILL_SECURITY);
            SetGuiSkillRank(pCreature, nSkillMask, MSG_GUI_SKILL_STEALTH, SKILL_STEALTH);
        }
    }

    // --- bit 3: the status word. Six combat-mode flags, each with its own on and off feedback message
    // id, then five raw bits copied into flag fields. The pairs are written out longhand rather than
    // folded into a helper. Bit 6 drives m_bRapidShotMode and bit 5 drives m_bFlurryMode, crossed relative to
    // every other pair.
    if (nMask & MSG_PLAYER_UPDATE_STATUS) {
        unsigned short nStatus = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature) {
            if (nStatus & MSG_PLAYER_STATUS_DETECT) {
                if (pCreature->m_bDetectMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_DETECT_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bDetectMode = 1;
            } else {
                if (pCreature->m_bDetectMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_DETECT_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bDetectMode = 0;
            }

            if (nStatus & MSG_PLAYER_STATUS_PARRY) {
                if (pCreature->m_bParryMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_PARRY_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bParryMode = 1;
            } else {
                if (pCreature->m_bParryMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_PARRY_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bParryMode = 0;
            }

            if (nStatus & MSG_PLAYER_STATUS_POWER_ATTACK) {
                if (pCreature->m_bPowerAttackMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_POWER_ATTACK_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bPowerAttackMode = 1;
            } else {
                if (pCreature->m_bPowerAttackMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_POWER_ATTACK_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bPowerAttackMode = 0;
            }

            if (nStatus & MSG_PLAYER_STATUS_IMPROVED_POWER_ATTACK) {
                if (pCreature->m_bImprovedPowerAttackMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_IMPROVED_POWER_ATTACK_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bImprovedPowerAttackMode = 1;
            } else {
                if (pCreature->m_bImprovedPowerAttackMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_IMPROVED_POWER_ATTACK_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bImprovedPowerAttackMode = 0;
            }

            if (nStatus & MSG_PLAYER_STATUS_RAPID_SHOT) {
                if (pCreature->m_bRapidShotMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_RAPID_SHOT_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bRapidShotMode = 1;
            } else {
                if (pCreature->m_bRapidShotMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_RAPID_SHOT_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bRapidShotMode = 0;
            }

            if (nStatus & MSG_PLAYER_STATUS_FLURRY) {
                if (pCreature->m_bFlurryMode == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_FLURRY_MODE_ON, pData, CExoString(""));
                }
                pCreature->m_bFlurryMode = 1;
            } else {
                if (pCreature->m_bFlurryMode == 1) {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_FLURRY_MODE_OFF, pData, CExoString(""));
                }
                pCreature->m_bFlurryMode = 0;
            }

            pCreature->m_bMovementSpeedLimited = (nStatus >> 7) & 1;
            pCreature->SetSpecialWalkAnimation((nStatus & MSG_PLAYER_STATUS_SPECIAL_WALK_ANIMATION) != 0);
            pCreature->m_bDisguised = (nStatus >> 10) & 1;
            pCreature->m_bCommandable = (nStatus >> 11) & 1;
            pCreature->m_bExcited = (nStatus >> 12) & 1;
            g_pAppManager->m_pClientExoApp->GetInGameGui();
            g_pAppManager->m_pClientExoApp->GetInGameGui();
        }
    }

    // --- bit 7: a counted list consumed only to advance the cursor.
    if (nMask & MSG_PLAYER_UPDATE_MEMORIZED_SPELL_COUNTS) {
        unsigned char nCount = ReadBYTE(8);
        for (unsigned char i = 0; i < nCount; i++) {
            ReadBYTE(8);
            ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;
        }
        if (g_pAppManager->m_pClientExoApp->GetInGameGui())
            g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    }

    // --- bit 4: known-power deltas, per class position.
    if (nMask & MSG_PLAYER_UPDATE_KNOWN_SPELLS) {
        for (unsigned char nClassPos = 0; nClassPos < 2; nClassPos++) {
            // The index is unused, so the loop counts down.
            unsigned short nRemove = ReadWORD(16);
            for (unsigned short i = nRemove; i != 0; i--) {
                unsigned int nSpellId = ReadDWORD(32);
                if (pCreature)
                    pStats->RemoveKnownSpell(nClassPos, nSpellId);
            }

            unsigned short nAdd = ReadWORD(16);
            for (unsigned short i = nAdd; i != 0; i--) {
                unsigned int nSpellId = ReadDWORD(32);
                if (pCreature)
                    pStats->AddKnownSpell(nClassPos, nSpellId);
            }
        }

#if !KOTOR1
        pStats->PayoutPowerAchievements();
#endif
        if (g_pAppManager->m_pClientExoApp->GetInGameGui())
            g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    }

    // --- bit 5: two more counted lists per class position, read and discarded.
    if (nMask & MSG_PLAYER_UPDATE_MEMORIZED_SPELLS) {
        for (unsigned char nClassPos = 0; nClassPos < 2; nClassPos++) {
            unsigned short nA = ReadWORD(16);
            for (unsigned short i = nA; i != 0; i--) {
                ReadBYTE(8);
                ReadBYTE(8);
                if (MessageReadOverflow(1))
                    return 0;
            }

            unsigned short nB = ReadWORD(16);
            for (unsigned short i = nB; i != 0; i--) {
                ReadDWORD(32);
                ReadBYTE(8);
                ReadBYTE(8);
                ReadBOOL();
                ReadBOOL();
                ReadBYTE(8);
                if (MessageReadOverflow(1))
                    return 0;
            }
        }
    }

    // --- bit 6: counted list, with the low bit of the leading word gating one extra byte.
    if (nMask & MSG_PLAYER_UPDATE_KNOWN_SPELL_USES) {
        unsigned char nCount = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned char i = 0; i < nCount; i++) {
            ReadBYTE(8);
            unsigned short nFlags = ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;

            if (nFlags & 1) {
                ReadBYTE(8);
                if (MessageReadOverflow(1))
                    return 0;
            }
        }
    }

    // --- bit 2: the feat deltas: a tagged stream over the two feat lists, then the remaining-uses
    // list. The tag chain is 'A', 'X', 'B', default in source order, so it is an if/else chain rather
    // than a switch. 'X' removes from the bonus list while the default arm removes from the ordinary
    // one.
    if (nMask & MSG_PLAYER_UPDATE_FEATS) {
        unsigned short nCount = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        for (int i = 0; i < nCount; i++) {
            char cTag = ReadCHAR(8);
            unsigned short nFeat = ReadWORD(16);
            if (MessageReadOverflow(1))
                return 0;

            if (cTag == 'A') {
                if (pCreature)
                    pStats->m_lstFeats.AddUnique(nFeat);
            } else if (cTag == 'X') {
                if (pCreature)
                    pStats->m_lstBonusFeats.Remove(nFeat);
            } else if (cTag == 'B') {
                if (pCreature)
                    pStats->m_lstBonusFeats.AddUnique(nFeat);
            } else {
                if (pCreature)
                    pStats->m_lstFeats.Remove(nFeat);
            }
        }

        unsigned short nUses = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        for (int i = 0; i < nUses; i++) {
            unsigned short nFeat = ReadWORD(16);
            unsigned char nRemaining = ReadBYTE(8);
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature)
                pStats->SetFeatRemainingUses(nFeat, nRemaining);
        }
    }

    // --- bit 8: the effect list. Every arm reads and discards; only the cursor matters. 'S' and 'D'
    // carry a trailing overflow guard that 'A' and 'U' do not.
    if (nMask & MSG_PLAYER_UPDATE_ACTION_QUEUE) {
        unsigned char nCount = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        for (int i = 0; i < nCount; i++) {
            unsigned char cTag = ReadCHAR(8);

            switch (cTag) {
                case 'A': {
                    // Target types 0 and 2 carry no object id; only 4, 12 and 19 go on to the three
                    // floats, and only 4 and 19 carry a trailing spell id.
                    unsigned short nTargetType = ReadWORD(16);
                    ReadWORD(16);
                    if (nTargetType != 0 && nTargetType != 2) {
                        ReadOBJECTIDClient();
                        if (nTargetType != 4 && nTargetType != 12 && nTargetType != 19)
                            break;
                    }
                    ReadFLOAT(1.0f, 32);
                    ReadFLOAT(1.0f, 32);
                    ReadFLOAT(1.0f, 32);
                    if (nTargetType == 19 || nTargetType == 4) {
                        int nSpellId = ReadDWORD(32);
                        g_pRules->GetSpell(nSpellId);
                    }
                    break;
                }

                case 'D':
                    ReadWORD(16);
                    ReadWORD(16);
                    if (MessageReadOverflow(1))
                        return 0;
                    break;

                case 'S':
                    ReadWORD(16);
                    ReadWORD(16);
                    ReadOBJECTIDClient();
                    if (MessageReadOverflow(1))
                        return 0;
                    break;

                case 'U':
                    ReadWORD(16);
                    ReadWORD(16);
                    ReadOBJECTIDClient();
                    break;
            }
        }
    }

    // --- bit 13: visibility-node deltas. This arm aborts the whole message on a null creature
    // instead of skipping its stores.
    if (nMask & MSG_PLAYER_UPDATE_VISIBILITY) {
        if (!pCreature)
            return 0;

        int nRemoved = ReadDWORD(32);
        if (MessageReadOverflow(1))
            return 0;

        for (int i = 0; i < nRemoved; i++) {
            unsigned long long nodeId = ReadOBJECTIDClient();
            if (MessageReadOverflow(1))
                return 0;

            // Drained back to front, like the list clear does.
            for (int j = pCreature->m_visibilityList.m_count; j > 0; j--) {
                CSWVisibilityNode* pNode = pCreature->m_visibilityList.m_pArray[j - 1];
                if (pNode->m_objectId == nodeId) {
                    pCreature->m_visibilityList.DelIndex(j - 1);
                    delete pNode;
                    break;
                }
            }
        }

        int nUpdated = ReadDWORD(32);
        if (MessageReadOverflow(1))
            return 0;

        for (int i = 0; i < nUpdated; i++) {
            unsigned long long nodeId = ReadOBJECTIDClient();
            int bA = ReadBOOL();
            int bB = ReadBOOL();
            int bC = ReadBOOL();
            if (MessageReadOverflow(1))
                return 0;

            CSWVisibilityNode* pNode = 0;
            for (int64_t j = 0; j < pCreature->m_visibilityList.m_count; j++) {
                if (pCreature->m_visibilityList.m_pArray[j]->m_objectId == nodeId) {
                    pNode = pCreature->m_visibilityList.m_pArray[j];
                    break;
                }
            }

            // The two branches do not compute the same flags. On a new node, bit 1 comes from bB and
            // bit 0 stays at the constructor's 0; on an existing node, bit 1 comes from bA and bit 0
            // from bB.
            if (pNode == 0) {
                pNode = new CSWVisibilityNode;
                pNode->m_objectId = nodeId;
                if (bA == 1)
                    pNode->m_flags |= kVisibleSeen | kVisibleHeard;
                else
                    pNode->m_flags = (pNode->m_flags & ~kVisibleHeard) | ((bB & 1) << 1);
                pNode->m_flags = (pNode->m_flags & ~kVisibleInactive) | ((bC & 1) << 4);

                pCreature->m_visibilityList.Add(pNode);
            } else {
                if (bA == 1)
                    pNode->m_flags |= kVisibleSeen | kVisibleHeard;
                else
                    pNode->m_flags = (pNode->m_flags & ~(kVisibleSeen | kVisibleHeard)) | ((bA & 1) << 1) | (bB & 1);
                pNode->m_flags = (pNode->m_flags & ~kVisibleInactive) | ((bC & 1) << 4);
            }

            pCreature->UpdateRelativePerception(pNode);
        }
    }

    // --- bit 11: ability scores and saving throws, behind their own bool gate. The base scores ship
    // first and the adjusted ones second, and the three saves arrive Will, Reflex, Fort.
    if (nMask & MSG_PLAYER_UPDATE_ABILITIES) {
        int bPresent = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        if (bPresent == 1) {
            unsigned char nStrBase = ReadBYTE(8);
            unsigned char nDexBase = ReadBYTE(8);
            unsigned char nConBase = ReadBYTE(8);
            unsigned char nIntBase = ReadBYTE(8);
            unsigned char nWisBase = ReadBYTE(8);
            unsigned char nChaBase = ReadBYTE(8);
            unsigned char nStr = ReadBYTE(8);
            unsigned char nDex = ReadBYTE(8);
            unsigned char nCon = ReadBYTE(8);
            unsigned char nInt = ReadBYTE(8);
            unsigned char nWis = ReadBYTE(8);
            unsigned char nCha = ReadBYTE(8);
            char nWillSave = ReadCHAR(8);
            char nReflexSave = ReadCHAR(8);
            char nFortSave = ReadCHAR(8);
            if (MessageReadOverflow(1))
                return 0;

            // All fifteen stores go through one pinned stats local, where the bit-0, bit-9 and bit-17
            // blocks re-read the stats pointer per store.
            if (pCreature) {
                CSWCCreatureStats* p = pCreature->m_pStats;
                p->m_abScoresBase[0] = nStrBase;
                p->m_abScoresBase[1] = nDexBase;
                p->m_abScoresBase[2] = nConBase;
                p->m_abScoresBase[3] = nIntBase;
                p->m_abScoresBase[4] = nWisBase;
                p->m_abScoresBase[5] = nChaBase;
                p->m_abScores[0] = nStr;
                p->m_abScores[1] = nDex;
                p->m_abScores[2] = nCon;
                p->m_abScores[3] = nInt;
                p->m_abScores[4] = nWis;
                p->m_abScores[5] = nCha;
                p->m_nWillSavingThrow = nWillSave;
                p->m_nReflexSavingThrow = nReflexSave;
                p->m_nFortSavingThrow = nFortSave;
            }
        }
    }

    // --- bit 15: the two per-skill rank arrays, interleaved one skill at a time. Each store re-reads
    // the stats pointer and the rules skill bound.
    if (nMask & MSG_PLAYER_UPDATE_SKILL_RANKS) {
        K2_ASSUME(pCreature != 0);   // a null creature returned above with this bit set
        for (unsigned short i = 0; i < g_pRules->m_nNumSkills; i++) {
            CSWCCreatureStats* pBaseStats = pCreature->m_pStats;
            char nBase = ReadCHAR(8);
            if (i < g_pRules->m_nNumSkills)
                pBaseStats->m_pSkillRanksBase[i] = nBase;

            CSWCCreatureStats* pRankStats = pCreature->m_pStats;
            char nRank = ReadCHAR(8);
            if (i < g_pRules->m_nNumSkills)
                pRankStats->m_pSkillRanks[i] = nRank;
        }
    }

    // --- bit 16: the portrait resref, but only for the custom-portrait sentinel pair.
    if (nMask & MSG_PLAYER_UPDATE_PORTRAIT) {
        K2_ASSUME(pCreature != 0);   // a null creature returned above with this bit set
        unsigned short nPortraitId = ReadWORD(16);
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
            pCreature->SetPortrait(ReadCResRef(16));
    }

    // --- bit 9: the four hit-point shorts: base, current, one unnamed, max.
    if (nMask & MSG_PLAYER_UPDATE_HIT_POINTS) {
        K2_ASSUME(pCreature != 0);   // a null creature returned above with this bit set
        pCreature->m_pStats->m_nHitPointsBase = ReadSHORT(16);
        pCreature->m_pStats->m_nCurrentHitPoints = ReadSHORT(16);
        pCreature->m_pStats->m_nHitPointBonus = ReadSHORT(16);
        pCreature->m_pStats->m_nMaxHitPoints = ReadSHORT(16);
    }

    // --- bit 17: the four force-point shorts, max last.
    if (nMask & MSG_PLAYER_UPDATE_FORCE_POINTS) {
        K2_ASSUME(pCreature != 0);   // a null creature returned above with this bit set
        pCreature->m_pStats->m_nForcePoints = ReadSHORT(16);
        pCreature->m_pStats->m_nCurrentForcePoints = ReadSHORT(16);
        pCreature->m_pStats->m_nTemporaryForcePoints = ReadSHORT(16);
        pCreature->m_pStats->m_nMaxForcePoints = ReadSHORT(16);
    }

    // --- bit 14: spell-like-ability list ops. 'S' sizes the list, 'U' writes one entry.
    if (nMask & MSG_PLAYER_UPDATE_SPELL_LIKE_ABILITIES) {
        unsigned short nOps = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        for (unsigned short i = nOps; i != 0; i--) {
            unsigned char cTag = ReadBYTE(8);
            if (MessageReadOverflow(1))
                return 0;

            if (cTag == 'U') {
                unsigned short nIndex = ReadWORD(16);
                int bReady = ReadBOOL();
                unsigned char nCasterLevel = ReadBYTE(8);
                unsigned short nSpellId = ReadWORD(16);
                if (MessageReadOverflow(1))
                    return 0;
                if (!pCreature)
                    continue;

                pCreature->m_pSpellLikeAbilityList->m_pArray[nIndex].m_nReady = bReady;
                pCreature->m_pSpellLikeAbilityList->m_pArray[nIndex].m_nCasterLevel = nCasterLevel;
                pCreature->m_pSpellLikeAbilityList->m_pArray[nIndex].m_nSpellId = nSpellId;
            } else if (cTag == 'S') {
                if (pCreature && pCreature->m_pSpellLikeAbilityList == 0)
                    pCreature->CreateNewSpellLikeAbilityList();

                unsigned short nSize = ReadWORD(16);
                if (MessageReadOverflow(1))
                    return 0;
                if (!pCreature)
                    continue;

                pCreature->m_pSpellLikeAbilityList->SetSize(nSize);
            }
        }
    }

    // Last, taking over a new creature, one that is not already ours and only outside a minigame,
    // re-points the control object.
    if (g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() != objId
            && g_pAppManager->m_pClientExoApp->GetMiniGame() == 0)
        HandleServerToPlayerCreatureUpdate_ControlObject(objId);

    return 1;
}

int CSWCMessage::HandleServerToPlayerCreatureUpdate_Update(unsigned long long objId,
                                                           unsigned int nFieldMask)
{
    CResRef resPortrait;

    // The attack list is staged into parallel arrays and only turned into a CSWCCombatAttackData
    // chain once the animation id says one is wanted.
    unsigned long long aTargetId[3];
    unsigned short     aReaxnAnimation[3];
    unsigned short     aReaxnAnimLength[3];
    unsigned short     aAnimationLength[3];
    unsigned char      aResult[3];
    unsigned short     aAttackType[3];
    unsigned short     aTotalDamage[3];
    int                aRangedAttack[3];
    int                aKillingBlow[3];
    unsigned char      aWeaponSound[3];

    Vector vPosition = { 0.0f, 0.0f, 0.0f };
    Vector vOrientation = { 0.0f, 0.0f, 0.0f };

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
    CSWCCreatureStats* pStats = (pCreature != NULL) ? pCreature->m_pStats : NULL;

    if (nFieldMask & MSG_OBJECT_UPDATE_POSITION)
    {
        vPosition.x = ReadFLOAT(1.0f, 32);
        vPosition.y = ReadFLOAT(1.0f, 32);
        vPosition.z = ReadFLOAT(1.0f, 32);
    }
    else if (pCreature != NULL)
    {
        vPosition = pCreature->m_position;
    }

    // oidLockTarget is uninitialized when bit 1 is clear; every read of it sits under the same
    // nFieldMask & MSG_OBJECT_UPDATE_ORIENTATION guard.
    unsigned long long oidLockTarget K2_UNSET;
    if (nFieldMask & MSG_OBJECT_UPDATE_ORIENTATION)
    {
        vOrientation.x = ReadFLOAT(1.0f, 32);
        vOrientation.y = ReadFLOAT(1.0f, 32);
        vOrientation.z = ReadFLOAT(1.0f, 32);
        if (ReadBYTE(8))
            oidLockTarget = ReadOBJECTIDClient();
        else
            oidLockTarget = CGameObjectArray::INVALIDOBJID;

        // A degenerate orientation would make normalize() produce a NaN basis, so it is snapped to +X.
        if (vOrientation.x * vOrientation.x + vOrientation.y * vOrientation.y
                + vOrientation.z * vOrientation.z < 1.0e-5f)
            vOrientation.x = 1.0f;
    }
    else if (pCreature != NULL)
    {
        vOrientation = pCreature->m_orientation;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_PORTRAIT)
    {
        CExoString sPrefix;
        CExoString sBaseResRef;

        unsigned short nPortraitId = ReadWORD(16);
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature != NULL)
            pCreature->SetPortraitId(nPortraitId);

        // The custom-portrait sentinels are the two "not a portraits.2da row" markers, where a literal
        // resref follows on the wire. CheckCustomPortrait takes the sentinel distinction as its gender
        // argument. The portraits.2da arm below builds resPortrait and drops it.
        if (nPortraitId >= MSG_PORTRAIT_ID_CUSTOM)
        {
            resPortrait = ReadCResRef(16);
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature != NULL)
            {
                CSWCCreatureStats::CheckCustomPortrait(&resPortrait,
                                                       nPortraitId == MSG_PORTRAIT_ID_CUSTOM);
                pCreature->SetPortrait(resPortrait);
            }
        }
        else
        {
            sPrefix = "po_";
            g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(
                nPortraitId, CExoString("BaseResRef"), &sBaseResRef);
            sBaseResRef = sPrefix + sBaseResRef;
            resPortrait = static_cast<char*>(sBaseResRef.CStr());
        }
    }

    unsigned short nAnim = (pCreature != NULL) ? static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation() : 0;

    int nAttackCount = 0;
    unsigned char nAttackKind = 0;
    unsigned long long oidAttackTarget = CGameObjectArray::INVALIDOBJID;
    float fAttackX = 0.0f;
    float fAttackY = 0.0f;
    float fAttackZ = 0.0f;
    int nSpellId = 0;
    int nSpellData = 0;
    int bHasAttackData = 0;

    if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
    {
        float fAnimSpeed = ReadFLOAT(1.0f, 32);
        if (MessageReadOverflow(1))
            return 0;
        if (pCreature != NULL)
            pCreature->m_animationSpeed = fAnimSpeed;

        nAnim = ReadWORD(16);

        // Only the two multi-attack animations carry an attack list.
        if (nAnim == SERVER_ANIMATION_ATTACK || nAnim == SERVER_ANIMATION_ATTACK_2)
        {
            nAttackCount = ReadBYTE(2);
            for (int i = 0; i < nAttackCount; i++)
            {
                aTargetId[i]    = ReadOBJECTIDClient();
                aReaxnAnimation[i]       = ReadWORD(16);
                aReaxnAnimLength[i]       = ReadWORD(16);
                aAnimationLength[i]       = ReadWORD(16);
                aResult[i]      = ReadBYTE(4);
                aAttackType[i]  = ReadWORD(16);
                aTotalDamage[i]       = ReadWORD(9);
                aRangedAttack[i]       = ReadBOOL();
                aKillingBlow[i]       = ReadBOOL();
                aWeaponSound[i] = ReadBYTE(4);
            }
            if (MessageReadOverflow(1))
                return 0;
        }

        // The conjure and cast family carries the spell id and its target.
        switch (static_cast<short>(nAnim))
        {
        case SERVER_ANIMATION_CONJURE_1: case SERVER_ANIMATION_CONJURE_2: case SERVER_ANIMATION_CAST_1: case SERVER_ANIMATION_CAST_2: case SERVER_ANIMATION_CAST_3: case SERVER_ANIMATION_CAST_4:
        case SERVER_ANIMATION_THROW_SABER_LOOP: case SERVER_ANIMATION_POWER_ATTACK: case SERVER_ANIMATION_CONJURE_3:
            nSpellId = ReadDWORD(32);
            oidAttackTarget = ReadOBJECTIDClient();
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature != NULL)
            {
                pCreature->m_conjureSpellId = nSpellId;
                pCreature->m_conjureTargetId = oidAttackTarget;
            }
            break;
        default:
            break;
        }

        // A narrower sub-family also carries what is being aimed at: 2 a world point, 1 an object,
        // anything else neither.
        switch (static_cast<short>(nAnim))
        {
        case SERVER_ANIMATION_CAST_1: case SERVER_ANIMATION_CAST_2: case SERVER_ANIMATION_CAST_3: case SERVER_ANIMATION_CAST_4: case SERVER_ANIMATION_THROW_SABER_LOOP: case SERVER_ANIMATION_POWER_ATTACK:
            nAttackKind = ReadBYTE(8);
            if (nAttackKind == 2)
            {
                fAttackX = ReadFLOAT(1.0f, 32);
                fAttackY = ReadFLOAT(1.0f, 32);
                fAttackZ = ReadFLOAT(1.0f, 32);
                oidAttackTarget = CGameObjectArray::INVALIDOBJID;
            }
            else if (nAttackKind == 1)
            {
                oidAttackTarget = ReadOBJECTIDClient();
                fAttackX = 0.0f;
                fAttackY = 0.0f;
                fAttackZ = 0.0f;
            }
            else
            {
                fAttackX = 0.0f;
                fAttackY = 0.0f;
                fAttackZ = 0.0f;
            }
            nSpellData = ReadDWORD(32);
            bHasAttackData = 1;
            if (MessageReadOverflow(1))
                return 0;
            break;
        default:
            bHasAttackData = 1;
            break;
        }
    }
    else
    {
        // With no animation on the wire the creature keeps whatever it is already playing, re-read
        // rather than reused because the portrait arm may have changed it.
        if (pCreature != NULL)
        {
            nAnim = static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation();
            bHasAttackData = 0;
        }
        else
        {
            bHasAttackData = 1;
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_VISUAL_EFFECTS)
    {
        if (HandleServerToPlayerUpdateVisualEffects(static_cast<CSWCObject*>(pCreature)) == 0)
            return 0;
    }

    unsigned char nMoveKind = 0;
    if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
    {
        nMoveKind = ReadBYTE(8);
        if (pCreature == NULL)
            goto LNoCreature;
        pCreature->m_nClientPathId = nMoveKind;
    }
    else
    {
        if (pCreature == NULL)
            goto LTailMask;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_ORIENTATION)
    {
        pCreature->SetDesiredStationaryOrientation(vOrientation);
        if (pCreature->m_bHasWayPointSpeed != 0
                && pCreature->m_objectId != g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
            pCreature->SetDesiredWalkOrientation(vOrientation);
        pCreature->SetLockOrientationToObject(oidLockTarget, 0);
    }

    {
        unsigned short nCurrent = static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation();

        // The way-point state is declared up front because the keep-the-loop-running arms of the
        // animation cascade below jump straight into the discard path.
        unsigned short nWayPointCount = 0;
        int bMoving = 0;
        float fSpeed = 1.0f;
        Vector* pWayPoints;
        unsigned short nWayPointIndex;
        int bDropped = 0;

        if (static_cast<CSWCObject*>(pCreature)->AnimationStationary(nCurrent) == 0
                && static_cast<CSWCObject*>(pCreature)->AnimationStationary(nAnim) == 1
                && static_cast<CSWCObject*>(pCreature)->AnimationLooping(nAnim) == 1)
        {
            // The animations below keep whatever loop is already running; everything else re-arms the
            // looping animation and then re-checks how far the creature still has to walk.
            int bKeepLoop;
            switch (static_cast<short>(nAnim))
            {
            case SERVER_ANIMATION_DEAD: case SERVER_ANIMATION_DEAD_2: case SERVER_ANIMATION_LISTEN:
            case SERVER_ANIMATION_TALK_NORMAL: case SERVER_ANIMATION_TALK_PLEADING: case SERVER_ANIMATION_TALK_FORCEFUL: case SERVER_ANIMATION_TALK_LAUGHING: case SERVER_ANIMATION_TALK_SAD:
            case SERVER_ANIMATION_TALK_INJURED: case SERVER_ANIMATION_LISTEN_INJURED: case SERVER_ANIMATION_DEAD_PRONE:
                bKeepLoop = 1;
                break;
            default:
                bKeepLoop = 0;
                break;
            }
            if (!bKeepLoop)
            {
                static_cast<CSWCObject*>(pCreature)->SetLoopingAnimation(nAnim);
                Vector* pEnd = pCreature->GetEndWayPoint();
                if (pEnd != NULL)
                {
                    if ((nFieldMask & MSG_OBJECT_UPDATE_POSITION) == 0)
                        goto LSkipWayPoints;
                    float dx = pEnd->x - vPosition.x;
                    float dy = pEnd->y - vPosition.y;
                    if (dx * dx + dy * dy + 0.0f <= 1.0e-5f)
                        goto LSkipWayPoints;
                }
            }
        }

        if (static_cast<CSWCObject*>(pCreature)->AnimationStationary(nCurrent) == 1
                && static_cast<CSWCObject*>(pCreature)->AnimationStationary(nAnim) == 0
                && dot(pCreature->m_position, pCreature->m_position) <= 1.0e-5f)
        {
            // (falls through to the position snap below)
        }
        else if (pCreature->m_bSnapPositionOnUpdate != 1)
        {
            goto LFireForget;
        }

        if (nFieldMask & MSG_OBJECT_UPDATE_POSITION)
        {
            pCreature->SetPosition(vPosition);
            pCreature->m_desiredPosition = vPosition;
        }
        pCreature->m_bSnapPositionOnUpdate = 0;

    LFireForget:
        if (static_cast<CSWCObject*>(pCreature)->AnimationFireAndForget(nAnim))
        {
            if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
            {
                switch (static_cast<short>(nAnim))
                {
                case SERVER_ANIMATION_ATTACK:
                case SERVER_ANIMATION_ATTACK_2:
                {
                    CSWCCombatAttackData* pHead = new CSWCCombatAttackData;
                    CSWCCombatAttackData* pNode = pHead;
                    for (int i = 0; i < nAttackCount; i++)
                    {
                        pNode->m_attackerObjectId = objId;
                        pNode->m_nAnimationLength = aAnimationLength[i];
                        pNode->m_nDataType = CSWCCombatAttackData::ATTACK_DATA_ATTACK;
                        pNode->m_nAttackResult = aResult[i];
                        pNode->m_reaxnGroup.m_nFeatId = aAttackType[i];
                        pNode->m_weaponObjectId = aTargetId[i];
                        pNode->m_reaxnGroup.m_nReaxnAnimation = aReaxnAnimation[i];
                        pNode->m_reaxnGroup.m_nReaxnAnimLength = aReaxnAnimLength[i];
                        pNode->m_nTotalDamage = aTotalDamage[i];
                        pNode->m_rangedAttackGroup.m_bRangedAttack = aRangedAttack[i];
                        pNode->m_killingBlowGroup.m_bKillingBlow = aKillingBlow[i];
                        pNode->m_nWeaponAttackType = aWeaponSound[i];
                        if (i < nAttackCount - 1)
                        {
                            CSWCCombatAttackData* pNext = new CSWCCombatAttackData;
                            pNode->m_pNext = pNext;
                            pNode = pNext;
                        }
                    }
                    pCreature->AddFireForgetAnimation(nAnim, pHead);
                    break;
                }
                case SERVER_ANIMATION_CAST_1: case SERVER_ANIMATION_CAST_2: case SERVER_ANIMATION_CAST_3: case SERVER_ANIMATION_CAST_4: case SERVER_ANIMATION_THROW_SABER_LOOP: case SERVER_ANIMATION_POWER_ATTACK:
                {
                    // Each of these plays a fixed cast overlay and then hands the resolved spell to the
                    // fire-and-forget queue.
                    unsigned short nOverlay;
                    switch (static_cast<short>(nAnim))
                    {
                    case SERVER_ANIMATION_CAST_1: nOverlay = SERVER_ANIMATION_CAST_1_OVERLAY; break;
                    case SERVER_ANIMATION_CAST_2: nOverlay = SERVER_ANIMATION_CAST_2_OVERLAY; break;
                    case SERVER_ANIMATION_CAST_3: nOverlay = SERVER_ANIMATION_CAST_3_OVERLAY; break;
                    case SERVER_ANIMATION_CAST_4: nOverlay = SERVER_ANIMATION_CAST_4_OVERLAY; break;
                    case SERVER_ANIMATION_THROW_SABER_LOOP: nOverlay = SERVER_ANIMATION_THROW_SABER_LOOP_OVERLAY; break;
                    default:     nOverlay = SERVER_ANIMATION_POWER_ATTACK; break;
                    }
                    pCreature->SetAnimation(nOverlay, 0, 0);

                    CSWCCombatAttackData* pData = new CSWCCombatAttackData;
                    pData->m_nDataType = CSWCCombatAttackData::ATTACK_DATA_CAST;
                    pData->m_nCastTargetType = nAttackKind;
                    pData->m_weaponObjectId = oidAttackTarget;
                    pData->m_castTargetGroup.m_fCastTargetX = *reinterpret_cast<unsigned int*>(&fAttackX);  // idiom-exempt: the orig stores the three raw floats into the int-typed +0x3c..+0x44 run
                    pData->m_castTargetGroup.m_fCastTargetY = *reinterpret_cast<unsigned int*>(&fAttackY);  // idiom-exempt: same run
                    pData->m_castTargetGroup.m_fCastTargetZ = *reinterpret_cast<unsigned int*>(&fAttackZ);                 // idiom-exempt: same run
                    pData->m_spellGroup.m_nSpellId = nSpellId;
                    pData->m_spellGroup.m_nCastLevel = nSpellData;
                    pCreature->AddFireForgetAnimation(nAnim, pData);
                    break;
                }
                default:
                    break;
                }
            }
        }
        else if (bHasAttackData)
        {
            pCreature->SetAnimation(nAnim, 0, 0);
        }

        if (static_cast<CSWCObject*>(pCreature)->AnimationStationary(nAnim))
        {
            if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
            {
                nWayPointCount = ReadWORD(16);
                if (nWayPointCount != 0)
                {
                    bMoving = ReadBOOL();
                    if (bMoving)
                        fSpeed = ReadFLOAT(1.0f, 32);
                }
            }
            // A stationary creature only takes a way-point run the server tagged, when something is
            // moving and the tag differs from the one already applied. Otherwise the points are read
            // off the wire and thrown away so the stream stays in sync.
            if (nMoveKind == 0 || (bMoving == 0 && pCreature->m_bHasWayPointSpeed == 0)
                    || pCreature->m_appliedCommandTag == nMoveKind)
            {
                pCreature->SetWayPoints(0, NULL);
#if !KOTOR1  // K1 leaves the discarded points unread on the wire
                for (int i = 0; i < nWayPointCount; i++)
                {
                    ReadFLOAT(1.0f, 32);
                    ReadFLOAT(1.0f, 32);
                    ReadFLOAT(1.0f, 32);
                }
#endif
                nWayPointCount = 0;
            }
        }
        else
        {
            if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
            {
                nWayPointCount = ReadWORD(16);
                if (nWayPointCount != 0)
                {
                    bMoving = ReadBOOL();
                    if (bMoving)
                        fSpeed = ReadFLOAT(1.0f, 32);
                }
            }
            else
            {
                unsigned short nCurrentCount;
                pCreature->GetWayPoints(&nCurrentCount, &pWayPoints, &nWayPointIndex);
                nWayPointCount = nCurrentCount;
            }
            if (MessageReadOverflow(1))
                return 0;
        }

        // An empty run is dropped; otherwise it is applied.
        if (nWayPointCount != 0)
        {
            pCreature->m_bHasWayPointSpeed = (bMoving != 0) ? 1 : (pCreature->m_bHasWayPointSpeed != 0);
            pCreature->m_fWayPointSpeed = fSpeed;
        }
        else
        {
            bDropped = 1;
        }

        Vector vGround = { 0.0f, 0.0f, 0.0f };
        if (static_cast<CSWCObject*>(pCreature)->AnimationStationary(nAnim) == 0 && !bDropped)
        {
            if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
            {
                Vector* pPoints = new Vector[nWayPointCount]();
                for (int i = 0; i < nWayPointCount; i++)
                {
                    pPoints[i].x = ReadFLOAT(1.0f, 32);
                    pPoints[i].y = ReadFLOAT(1.0f, 32);
                    pPoints[i].z = ReadFLOAT(1.0f, 32);
                    if (MessageReadOverflow(1))
                        return 0;
                }

                Vector* pEnd = pCreature->GetEndWayPoint();
                int bSameDestination = 0;
                if (pEnd != NULL)
                {
                    float dx = pEnd->x - pPoints[nWayPointCount - 1].x;
                    float dy = pEnd->y - pPoints[nWayPointCount - 1].y;
                    if (dx * dx + dy * dy + 0.0f <= 1.0e-5f)
                        bSameDestination = 1;
                }
                if (bSameDestination)
                {
                    float ex = pCreature->m_position.x - pPoints[nWayPointCount - 1].x;
                    float ey = pCreature->m_position.y - pPoints[nWayPointCount - 1].y;
                    if (ex * ex + ey * ey + 0.0f <= 1.0e-5f)
                        pCreature->SetAnimation(static_cast<CSWCObject*>(pCreature)->GetLoopingAnimation(), 0, 0);
                }
                else
                {
                    pCreature->EvaluateNewWayPoints(nWayPointCount, pPoints, nMoveKind, vPosition);
                }
                delete[] pPoints;
            }

            if ((nFieldMask & MSG_OBJECT_UPDATE_ORIENTATION) && oidLockTarget == CGameObjectArray::INVALIDOBJID)
                pCreature->SetLockOrientationToObject(oidLockTarget, 0);
        }
        else if (pCreature->m_bHasWayPointSpeed == 0 || nMoveKind == 0
                || pCreature->m_appliedCommandTag == nMoveKind)
        {
            if (pCreature->m_pArea != NULL)
                pCreature->ComputeZValue(pCreature->m_pArea, &vPosition, vPosition.z, &vGround);

            if (nFieldMask & MSG_OBJECT_UPDATE_POSITION)
            {
                float dx = pCreature->m_desiredPosition.x - vPosition.x;
                float dy = pCreature->m_desiredPosition.y - vPosition.y;
                float dz = pCreature->m_desiredPosition.z - vPosition.z;
                if (dx * dx + dy * dy + dz * dz > 1.0e-5f)
                {
                    pCreature->m_desiredPosition = vPosition;
                    pCreature->ResetDesiredPositionTimer();
                    pCreature->SetGroundNormal(vGround);
                }

                if (!(dot(pCreature->m_position, pCreature->m_position) <= 1.0e-5f))
                {
                    float qx = pCreature->m_position.x - vPosition.x;
                    float qy = pCreature->m_position.y - vPosition.y;
                    float qz = pCreature->m_position.z - vPosition.z;
                    if (!(qx * qx + qy * qy + qz * qz >= 36.0f))
                        goto LTailMask;
                }
                pCreature->SetPosition(vPosition);
                pCreature->SetGroundNormal(vGround);
                if (nFieldMask & MSG_OBJECT_UPDATE_ORIENTATION)
                    pCreature->SetOrientation(vOrientation);
            }
        }
        goto LTailMask;
    }

LNoCreature:
    // Reached either with no creature at all or from the animation cascade once it decided the
    // creature is already walking there. Either way the way-point payload still comes off the wire so
    // the stream stays in sync, though the points are thrown away, and the speed/moving pair is still
    // applied when a creature exists.
LSkipWayPoints:
    if (nFieldMask & MSG_OBJECT_UPDATE_ANIMATION)
    {
        unsigned short nSkipCount = ReadWORD(16);
        if (nSkipCount != 0)
        {
            int bSkipMoving = ReadBOOL();
            float fSkipSpeed = 1.0f;
            if (bSkipMoving)
                fSkipSpeed = ReadFLOAT(1.0f, 32);
            if (pCreature != NULL)
            {
                pCreature->m_bHasWayPointSpeed = (bSkipMoving != 0) ? 1 : (pCreature->m_bHasWayPointSpeed != 0);
                pCreature->m_fWayPointSpeed = fSkipSpeed;
            }
            for (int i = 0; i < nSkipCount; i++)
            {
                ReadFLOAT(1.0f, 32);
                ReadFLOAT(1.0f, 32);
                ReadFLOAT(1.0f, 32);
            }
        }
    }

LTailMask:
    if (nFieldMask & MSG_OBJECT_UPDATE_AI_STATE)
    {
        unsigned short nCombatFlags = ReadWORD(16);
        unsigned char nAiState = ReadBYTE(8);
        unsigned short nActivityFlags = ReadWORD(16);
        unsigned char nDisposition = ReadBYTE(8);
        unsigned char bStealth = ReadBYTE(8);
        int bStealthFeedback = ReadBOOL();
        int bCombat = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        unsigned long long oidCombatTarget = CGameObjectArray::INVALIDOBJID;
        if (nAiState == 2)
            oidCombatTarget = ReadOBJECTIDClient();
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature != NULL)
        {
            // Entering or leaving stealth plays the same feedback and "sdr_invisible" cue as the
            // combat-mode arm below, but only for the local player's own creature.
            if (bStealth == 0)
            {
                if (pCreature->m_nStealthState == 1)
                {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_STEALTH_MODE_OFF, pData, CExoString(""));
                    if (pCreature->m_objectId
                            == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                        g_pExoSound->Play3DOneShotSound(CResRef("sdr_invisible"),
                            pCreature->m_position, 0.0f, PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
                }
                pCreature->SetStealthState(0, bStealthFeedback);
            }
            else if (bStealth == 1)
            {
                if (pCreature->m_nStealthState == 0)
                {
                    CSWCCMessageData* pData = new CSWCCMessageData;
                    pData->SetObjectID(0, pCreature->m_objectId);
                    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_STEALTH_MODE_ON, pData, CExoString(""));
                    if (pCreature->m_objectId
                            == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                        g_pExoSound->Play3DOneShotSound(CResRef("sdr_invisible"),
                            pCreature->m_position, 0.0f, PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
                }
                pCreature->SetStealthState(1, bStealthFeedback);
            }
        }
        if (pCreature != NULL)
        {
            if (nCombatFlags != 0)
            {
                g_pAppManager->m_pClientExoApp->RemoveObjectFromHitcheckIgnoreList(
                    pCreature->m_objectId);
            }
            else
            {
                if (g_pAppManager->m_pClientExoApp->GetPlayerByGameObjectID(
                            pCreature->m_objectId) == NULL
                        && pCreature->m_bPlot == 0)
                    g_pAppManager->m_pClientExoApp->AddObjectToHitcheckIgnoreList(
                        pCreature->m_objectId);

                pCreature->ComputeWeaponOverlays(0, 0);

                CSWCItem* pRight = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                    pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON));
                if (pRight != NULL)
                    pRight->PowerItem(0, 0, 1, 0);
                CSWCItem* pLeft = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                    pCreature->GetEquippedItemID(MSG_SLOT_LEFT_WEAPON));
                if (pLeft != NULL)
                    pLeft->PowerItem(0, 0, 1, 0);

                if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                            pCreature->m_objectId)
                        && g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount >= 2)
                    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                        TUTORIAL_PARTY_MEMBER_DIES, CGameObjectArray::INVALIDOBJID, CGameObjectArray::INVALIDOBJID, 0);
            }

            pCreature->m_nAIStateFlags = nCombatFlags;
            pCreature->m_nAIStateAction = nAiState;
            pCreature->m_nActivityFlags = nActivityFlags;
            pCreature->m_nReputation = nDisposition;
            pCreature->m_oidCombatTarget = oidCombatTarget;
            pCreature->SetCombatState(bCombat);

            // Dropping into combat with the auto-pause-on-combat-round option selected pops the camera
            // back out and re-arms the indicator.
            if (bCombat && pCreature->m_bInParty != 0
                    && g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
            {
                g_bShowIndicator = 1;
                g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
                g_pAppManager->m_pClientExoApp->SetInputClass(0, 1);
            }
            g_pAppManager->m_pClientExoApp->GetInGameGui();
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_MOVE_RATES)
    {
        float fWalkRate = ReadFLOAT(1.0f, 32);
        float fRunRate = ReadFLOAT(1.0f, 32);
        if (MessageReadOverflow(1))
            return 0;
        if (pCreature != NULL)
        {
            pCreature->m_fWalkRate = fWalkRate;
            pCreature->m_fRunRate = fRunRate;
            pCreature->m_pCreatureAppearance->m_fDriveMaxSpeed = fRunRate / 1000.0f;
#if KOTOR1
            if (fRunRate < pCreature->m_pCreatureAppearance->GetRunLooksAwfulRate())
                pCreature->SetRunLooksAwful(1);
            else
                pCreature->SetRunLooksAwful(0);
#else
            pCreature->SetRunLooksAwful(
                pCreature->m_pCreatureAppearance->GetRunLooksAwfulRate() > fRunRate);
#endif
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_AREA)
    {
        ReadOBJECTIDClient();
        if (MessageReadOverflow(1))
            return 0;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_COARSE_POSITION)
    {
        ReadINT(10);
        ReadINT(10);
        if (MessageReadOverflow(1))
            return 0;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_HIT_POINTS)
    {
        short nCurrentHP = ReadSHORT(16);
        short nBaseHP = ReadSHORT(16);
        short nSpareHP = ReadSHORT(16);
        short nMaxHP = ReadSHORT(16);
        if (MessageReadOverflow(1))
            return 0;
        if (pCreature != NULL)
        {
            pCreature->m_pStats->m_nHitPointsBase = nBaseHP;
            pCreature->m_pStats->m_nHitPointBonus = nSpareHP;
            pCreature->m_pStats->m_nCurrentHitPoints = nCurrentHP;
            pCreature->m_pStats->m_nMaxHitPoints = nMaxHP;
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_FORCE_POINTS)
    {
        short nCurrentFP = ReadSHORT(16);
        short nBaseFP = ReadSHORT(16);
        short nSpareFP = ReadSHORT(16);
        short nMaxFP = ReadSHORT(16);
        if (MessageReadOverflow(1))
            return 0;

        // The creature pointer is resolved again rather than reusing pCreature.
        CSWCCreature* pFP = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objId);
        if (pFP != NULL)
        {
            pFP->m_pStats->m_nForcePoints = nBaseFP;
            pFP->m_pStats->m_nTemporaryForcePoints = nSpareFP;
            pFP->m_pStats->m_nCurrentForcePoints = nCurrentFP;
            pFP->m_pStats->m_nMaxForcePoints = nMaxFP;

#if !KOTOR1  // K1 models carry no force points
            Gob* pBody = pFP->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0);
            if (pBody != NULL)
                pBody->SetForcePoints(nMaxFP);
            Gob* pHead = pFP->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 0);
            if (pHead != NULL)
                pHead->SetForcePoints(nMaxFP);
#endif
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_NAME)
    {
        CExoString sFirstName = ReadCExoLocStringClient();
        CExoString sLastName = ReadCExoLocStringClient();
        if (MessageReadOverflow(1))
            return 0;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_PARTY)
    {
        int bPlayerCharacter = ReadBOOL();
        int bAIStateFlag = ReadBOOL();
        int bSingletonParty = ReadBOOL();
        int bFactionLeader = ReadBOOL();
        int bHasSelectable = ReadBOOL();

        CExoString sSelectableName;
        CExoString sUnused;
        unsigned long long oidSelectable = CGameObjectArray::INVALIDOBJID;
        if (bHasSelectable)
        {
            oidSelectable = ReadOBJECTIDClient();
            sSelectableName = ReadCExoLocStringClient();
            sUnused = ReadCExoLocStringClient();
        }
        ReadBOOL();
        ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature != NULL)
        {
            if (pCreature->m_pPartyStatus == NULL)
                pCreature->m_pPartyStatus = new CSWCCreaturePartyStatus;
            pCreature->m_pPartyStatus->m_bPlayerCharacter = bPlayerCharacter;
            pCreature->m_pPartyStatus->m_bAIStateFlag = bAIStateFlag;
            pCreature->m_pPartyStatus->m_bSingletonParty = bSingletonParty;
            pCreature->m_pPartyStatus->m_bFactionLeader = bFactionLeader;
            pCreature->m_pPartyStatus->m_bHasSelectable = bHasSelectable;
            if (bHasSelectable)
            {
                pCreature->m_pPartyStatus->m_objectId = oidSelectable;
                pCreature->m_pPartyStatus->m_sName = sSelectableName;
            }
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_VISIBILITY)
    {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        unsigned long long oidPlayer = (pPlayer != NULL) ? pPlayer->m_objectId
                                                         : CGameObjectArray::INVALIDOBJID;
        int bSeen = ReadBOOL();
        int bHeard = ReadBOOL();
        int bInactive = ReadBOOL();
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature != NULL)
        {
            CSWVisibilityNode* pNode = 0;
            int bMissing = 1;
            for (int i = 0; i < pCreature->m_visibilityList.m_count; i++)
            {
                pNode = pCreature->m_visibilityList.m_pArray[i];
                if (pNode->m_objectId == oidPlayer)
                {
                    bMissing = 0;
                    break;
                }
            }

            if (oidPlayer == objId)
            {
                // The player perceiving itself is always both seen and heard.
                if (bMissing)
                {
                    CSWVisibilityNode* pNew = new CSWVisibilityNode;
                    pNew->m_objectId = objId;
                    pNew->m_flags = static_cast<unsigned char>(((bInactive << 4) & kVisibleInactive) | kVisibleSeen | kVisibleHeard);
                    pCreature->m_visibilityList.Add(pNew);
                    pCreature->UpdateRelativePerception(pNew);
                }
                else
                {
                    pNode->m_flags = static_cast<unsigned char>((pNode->m_flags & ~(kVisibleSeen | kVisibleHeard | kVisibleInactive))
                        | ((bInactive << 4) & kVisibleInactive) | kVisibleSeen | kVisibleHeard);
                    pCreature->UpdateRelativePerception(pNode);
                }
            }
            else if (!bMissing)
            {
                pNode->m_flags = static_cast<unsigned char>((pNode->m_flags & ~(kVisibleSeen | kVisibleHeard | kVisibleInactive)) | (bSeen & 1)
                    | ((bHeard << 1) & kVisibleHeard) | ((bInactive << 4) & kVisibleInactive));
            }
            else
            {
                while (pCreature->m_visibilityList.m_count > 0)
                {
                    delete pCreature->m_visibilityList.m_pArray[0];
                    pCreature->m_visibilityList.DelIndex(0);
                }
                if (oidPlayer != CGameObjectArray::INVALIDOBJID)
                {
                    CSWVisibilityNode* pNew = new CSWVisibilityNode;
                    pNew->m_objectId = oidPlayer;
                    pNew->m_flags = static_cast<unsigned char>((bSeen & 1) | ((bHeard << 1) & kVisibleHeard)
                        | ((bInactive << 4) & kVisibleInactive));
                    pCreature->m_visibilityList.Add(pNew);
                }
            }
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_ASSOCIATE_STATE)
    {
        unsigned char nModes = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        // The eight combat-mode bits, each with its own on and off feedback pair. Bit 6 drives
        // m_bRapidShotMode and bit 5 drives m_bFlurryMode, crossed relative to every other pair, as in the
        // player info status word above.
        if (pCreature != NULL)
        {
            int bOn;

            bOn = (nModes & MSG_ASSOCIATE_STATE_DETECT) != 0;
            if (bOn) { if (pCreature->m_bDetectMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_DETECT_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_bDetectMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_DETECT_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->m_bDetectMode = bOn;

            bOn = (nModes & MSG_ASSOCIATE_STATE_STEALTH) != 0;
            if (bOn) { if (pCreature->m_nStealthState == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_STEALTH_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_nStealthState == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_STEALTH_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->SetStealthState(bOn, 0);

            bOn = (nModes & MSG_ASSOCIATE_STATE_PARRY) != 0;
            if (bOn) { if (pCreature->m_bParryMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_PARRY_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_bParryMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_PARRY_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->m_bParryMode = bOn;

            bOn = (nModes & MSG_ASSOCIATE_STATE_POWER_ATTACK) != 0;
            if (bOn) { if (pCreature->m_bPowerAttackMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_POWER_ATTACK_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_bPowerAttackMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_POWER_ATTACK_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->m_bPowerAttackMode = bOn;

            bOn = (nModes & MSG_ASSOCIATE_STATE_IMPROVED_POWER_ATTACK) != 0;
            if (bOn) { if (pCreature->m_bImprovedPowerAttackMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_IMPROVED_POWER_ATTACK_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_bImprovedPowerAttackMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_IMPROVED_POWER_ATTACK_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->m_bImprovedPowerAttackMode = bOn;

            bOn = (nModes & MSG_ASSOCIATE_STATE_RAPID_SHOT) != 0;
            if (bOn) { if (pCreature->m_bRapidShotMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_RAPID_SHOT_MODE_ON, pData, CExoString(""));
 } }
            else     { if (pCreature->m_bRapidShotMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_RAPID_SHOT_MODE_OFF, pData, CExoString(""));
 } }
            pCreature->m_bRapidShotMode = bOn;

            if (nModes & MSG_ASSOCIATE_STATE_FLURRY)
            {
                if (pCreature->m_bFlurryMode == 0) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_FLURRY_MODE_ON, pData, CExoString(""));
 }
                pCreature->m_bFlurryMode = 1;
            }
            else
            {
                if (pCreature->m_bFlurryMode == 1) {
     CSWCCMessageData* pData = new CSWCCMessageData;
     pData->SetObjectID(0, pCreature->m_objectId);
     g_pAppManager->m_pClientExoApp->AddFeedbackMessage(
         FEEDBACK_FLURRY_MODE_OFF, pData, CExoString(""));
 }
                pCreature->m_bFlurryMode = 0;
            }
        }
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_DAMAGE_LEVEL)
    {
        unsigned char nArmorClass = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;
        if (pCreature != NULL)
            pCreature->m_pStats->m_nArmorClassSent = nArmorClass;
    }

    if (nFieldMask & MSG_OBJECT_UPDATE_IDENTITY)
    {
        CExoString sSubrace;
        unsigned short nRace = ReadWORD(16);
        sSubrace = ReadCExoString(32);
        unsigned char nSubraceIndex = ReadBYTE(8);
        unsigned char nGender = ReadBYTE(8);
        short nAlignment = ReadSHORT(16);
#if !KOTOR1
        short nCNPCAlignment = ReadSHORT(16);
        int nStanceId = ReadINT(32);
        CExoString sTag = ReadCExoString(32);
#endif
        int bPlot = ReadBOOL();
        unsigned char nNumClasses = ReadBYTE(8);
        if (MessageReadOverflow(1))
            return 0;

        if (pCreature != NULL)
        {
#if !KOTOR1
            pStats->m_sTag = sTag;
#endif
            pStats->m_nRace = nRace;
            pStats->m_raceString = sSubrace;
            pStats->m_nSubraceIndex = nSubraceIndex;
            pStats->m_nGender = nGender;

            pCreature->SetAlignmentGoodEvil(nAlignment);

#if !KOTOR1
            // In the force-glow ramp, light side fades toward blue, dark side toward red, with the
            // 40/50/60 alignment bands as the neutral plateau. The two saturated ends ramp their
            // off-channels down to 0.01/0.05 against a 0.65 neutral base; the middle two hold one
            // channel at 1.0 (or 0.9) and halve the glow. Only the two inner bands take the *0.5f
            // on fGlow.
            float fGlow, fR, fG, fB;
            if (nAlignment >= 61)
            {
                float t = static_cast<float>(nAlignment - 60) / 40.0f;
                fGlow = t;
                fR = t * 0.01f + (1.0f - t) * 0.65f;
                fB = 1.0f;
                fG = t * 0.05f + (1.0f - t) * 0.65f;
            }
            else if (nAlignment >= 41)
            {
                if (nAlignment >= 50)
                {
                    float t = static_cast<float>(nAlignment - 50) / -10.0f + 1.0f;
                    fGlow = t;
                    fR = t + (1.0f - t) * 0.65f;
                    fB = 0.9f;
                    fG = fR;
                }
                else
                {
                    float t = static_cast<float>(nAlignment - 40) / 10.0f;
                    fGlow = t;
                    fR = 1.0f;
                    fG = t + (1.0f - t) * 0.65f;
                    fB = fG;
                }
                fGlow = fGlow * 0.5f;
            }
            else
            {
                float t = static_cast<float>(40 - nAlignment) / 40.0f;
                fGlow = t;
                fR = 1.0f;
                fB = t * 0.01f + (1.0f - t) * 0.65f;
                fG = t * 0.05f + (1.0f - t) * 0.65f;
            }

            Gob* pBody = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0);
            if (pBody != NULL)
            {
                pBody->SetAlignmentValue(nAlignment);
                pBody->SetForceGlowColor(fR, fG, fB, fGlow);
                pBody->EnableForceGlow(nRace != CSWRace::CRACE_DROID);
                pBody->SetNonForceBlending(true);
            }
            Gob* pHead = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 0);
            if (pHead != NULL)
            {
                pHead->SetAlignmentValue(nAlignment);
                pHead->SetForceGlowColor(fR, fG, fB, fGlow);
                pHead->EnableForceGlow(nRace != CSWRace::CRACE_DROID);
                pHead->SetNonForceBlending(true);
            }

            pCreature->SetBaseCNPCAlignment(nCNPCAlignment);

            CSWCAnimBase* pAnimBase = static_cast<CSWCObject*>(pCreature)->GetAnimationBase();
            if (pAnimBase != NULL)
                pAnimBase->m_stanceId = nStanceId;
#endif

            pCreature->UpdateMonkVisuals();
            pCreature->m_bPlot = bPlot;
            if (bPlot == 1)
            {
                g_pAppManager->m_pClientExoApp->RemoveObjectFromHitcheckIgnoreList(
                    pCreature->m_objectId);
            }
            else if (g_pAppManager->m_pClientExoApp->GetPlayerByGameObjectID(
                             pCreature->m_objectId) == NULL
                    && pCreature->m_nAIStateFlags == 0)
            {
                g_pAppManager->m_pClientExoApp->AddObjectToHitcheckIgnoreList(
                    pCreature->m_objectId);
            }
            pStats->m_nNumClasses = nNumClasses;
        }

        for (int i = 0; i < nNumClasses; i++)
        {
            unsigned char nClass = ReadBYTE(8);
            unsigned char nLevel = ReadBYTE(8);
            if (MessageReadOverflow(1))
                return 0;
            if (pCreature != NULL)
            {
                pStats->SetClass(static_cast<unsigned char>(i), nClass);
                pStats->SetClassLevel(static_cast<unsigned char>(i), nLevel);
            }
        }
    }

    // A creature whose area changed has to be re-parented into the live scene graph.
    if ((nFieldMask & MSG_OBJECT_UPDATE_AREA_CHANGED) && pCreature != NULL)
    {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != NULL)
        {
            CSWCArea* pArea = pModule->m_pArea;
            if (pArea != NULL && pCreature->m_areaObjectId == pArea->m_objectId)
            {
                Gob* pBody = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0);
                if (pBody != NULL && pBody->GetScene() != pArea->m_pScene)
                {
                    if (g_pAppManager->m_pClientExoApp->ReattachCreatureToScene(
                            pCreature->m_objectId))
                        pCreature->AttachToScene(pArea->m_pScene);
                }
            }
        }
    }

    return 1;
}
