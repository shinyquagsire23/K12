
#include "nwsambientsound.h"

#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"

#include "appmanager.h"
#include "serverexoapp.h"
#include "exolinkedlistinternal.h"
#include "nwsplayer.h"
#include "nwsobject.h"
#include "nwscreature.h"
#include "nwarea.h"
#include "nwsarea.h"
#include "nwssendplayermessage.h"
#include "exostring.h"
#include "resgff.h"
#include "nwsrules.h"
#include "nwrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "exoresman.h"

CSWSAmbientSound::CSWSAmbientSound(unsigned long long nObjectId)
{
    m_oidArea = nObjectId;
}

// Broadcasts the music state to every player in the area; unlike the setters there is no
// changed-value guard.
void CSWSAmbientSound::PlayMusic(int nState)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    m_bMusicPlaying = nState;

    int nCount = GetPlayersInArea(&oidPlayers);
    for (int i = 0; i < nCount; i++)
        pMsg->SendServerToPlayerAmbientMusicPlay(oidPlayers.m_pArray[i], nState);
}

// Fills the list with the players in the area and returns their count.
int CSWSAmbientSound::GetPlayersInArea(CExoArrayList<unsigned long long>* pPlayers)
{
    pPlayers->Clear();

    CExoLinkedList<CSWSPlayerListEntry>* ppList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    if (ppList == 0)
        return pPlayers->m_count;

    CExoLinkedListNode* pNode = ppList->m_pInternal->m_pHead;
    while (pNode != 0)
    {
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppList->m_pInternal->GetAtPos(pNode));
        if (pPlayer != 0 && pPlayer->GetGameObject() != 0)
        {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            if (pCreature != 0 && pCreature->GetArea() != 0 &&
                static_cast<CSWSArea*>(pCreature->GetArea())->m_objectId == m_oidArea)
            {
                pPlayers->Add(pPlayer->m_nPlayerId);
            }
        }

        ppList->m_pInternal->GetNext(pNode);
    }

    return pPlayers->m_count;
}

void CSWSAmbientSound::SetMusicDelay(int nDelay)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nMusicDelay != nDelay)
    {
        m_nMusicDelay = nDelay;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientMusicSetDelay(oidPlayers.m_pArray[i], nDelay);
    }
}

#if KOTOR1
void CSWSAmbientSound::SetMusicDayTrack(int nTrack)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    m_nMusicDayTrack = nTrack;

    int nCount = GetPlayersInArea(&oidPlayers);
    for (int i = 0; i < nCount; i++)
        pMsg->SendServerToPlayerAmbientMusicChangeTrack(oidPlayers.m_pArray[i], 1, nTrack);
}
#else
void CSWSAmbientSound::SetMusicDayTrack(int nTrack, int bStreaming)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    m_nMusicDayTrack = nTrack;

    int nCount = GetPlayersInArea(&oidPlayers);
    for (int i = 0; i < nCount; i++)
        pMsg->SendServerToPlayerAmbientMusicChangeTrack(oidPlayers.m_pArray[i], 1, nTrack,
                                                        bStreaming);
}
#endif

#if KOTOR1
void CSWSAmbientSound::SetMusicNightTrack(int nTrack)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nMusicNightTrack != nTrack)
    {
        m_nMusicNightTrack = nTrack;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientMusicChangeTrack(oidPlayers.m_pArray[i], 0, nTrack);
    }
}
#else
void CSWSAmbientSound::SetMusicNightTrack(int nTrack, int bStreaming)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nMusicNightTrack != nTrack)
    {
        m_nMusicNightTrack = nTrack;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientMusicChangeTrack(oidPlayers.m_pArray[i], 0, nTrack,
                                                            bStreaming);
    }
}
#endif

void CSWSAmbientSound::PlayBattleMusic(int nState)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_bBattleMusicPlaying != nState)
    {
        m_bBattleMusicPlaying = nState;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientBattleMusicPlay(oidPlayers.m_pArray[i], nState);
    }
}

void CSWSAmbientSound::SetBattleMusicTrack(int nTrack)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nBattleMusicTrack != nTrack)
    {
        m_nBattleMusicTrack = nTrack;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientBattleMusicChange(oidPlayers.m_pArray[i], nTrack);
    }
}

// Unconditional, like PlayMusic. Also sets the flag the client reads to start its sounds.
void CSWSAmbientSound::PlayAmbientSound(int nState)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    m_bAmbientPlaying = nState;

    int nCount = GetPlayersInArea(&oidPlayers);
    for (int i = 0; i < nCount; i++)
        pMsg->SendServerToPlayerAmbientSoundLoopPlay(oidPlayers.m_pArray[i], nState);
}

void CSWSAmbientSound::SetAmbientDayTrack(int nTrack)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nAmbientDayTrack != nTrack)
    {
        m_nAmbientDayTrack = nTrack;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientSoundLoopChange(oidPlayers.m_pArray[i], 1, nTrack);
    }
}

void CSWSAmbientSound::SetAmbientNightTrack(int nTrack)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (m_nAmbientNightTrack != nTrack)
    {
        m_nAmbientNightTrack = nTrack;

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientSoundLoopChange(oidPlayers.m_pArray[i], 0, nTrack);
    }
}

// Accepts only 0..100; the volume fields are bytes.
void CSWSAmbientSound::SetAmbientDayVolume(int nVolume)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (nVolume < 0 || nVolume > 100)
        return;

    if (m_nAmbientDayVolume != nVolume)
    {
        m_nAmbientDayVolume = static_cast<unsigned char>(nVolume);

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientSoundVolumeChange(oidPlayers.m_pArray[i], 1, nVolume);
    }
}

void CSWSAmbientSound::SetAmbientNightVolume(int nVolume)
{
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoArrayList<unsigned long long> oidPlayers;

    if (nVolume < 0 || nVolume > 100)
        return;

    if (m_nAmbientNightVolume != nVolume)
    {
        m_nAmbientNightVolume = static_cast<unsigned char>(nVolume);

        int nCount = GetPlayersInArea(&oidPlayers);
        for (int i = 0; i < nCount; i++)
            pMsg->SendServerToPlayerAmbientSoundVolumeChange(oidPlayers.m_pArray[i], 0, nVolume);
    }
}

// Reads each field with the member's current value as the default, so a field missing from the
// file keeps it. Returns the last value read.
int CSWSAmbientSound::Load(CResGFF* pGff, CResStruct* pStruct)
{
    int nValue;

    m_nMusicDelay = pGff->ReadFieldINT(pStruct, const_cast<char*>("MusicDelay"), nValue, m_nMusicDelay);
    m_nMusicDayTrack = pGff->ReadFieldINT(pStruct, const_cast<char*>("MusicDay"), nValue, m_nMusicDayTrack);
    m_nMusicNightTrack = pGff->ReadFieldINT(pStruct, const_cast<char*>("MusicNight"), nValue, m_nMusicNightTrack);
    m_nBattleMusicTrack = pGff->ReadFieldINT(pStruct, const_cast<char*>("MusicBattle"), nValue, m_nBattleMusicTrack);
    m_nAmbientDayTrack = pGff->ReadFieldINT(pStruct, const_cast<char*>("AmbientSndDay"), nValue, m_nAmbientDayTrack);
    m_nAmbientNightTrack = pGff->ReadFieldINT(pStruct, const_cast<char*>("AmbientSndNight"), nValue,
                                              m_nAmbientNightTrack);
    m_nAmbientDayVolume = static_cast<unsigned char>(pGff->ReadFieldINT(pStruct, const_cast<char*>("AmbientSndDayVol"), nValue,
                                             m_nAmbientDayVolume));
    m_nAmbientNightVolume = static_cast<unsigned char>(pGff->ReadFieldINT(pStruct, const_cast<char*>("AmbientSndNitVol"), nValue,
                                               m_nAmbientNightVolume));

    return nValue;
}

// Formats the 2DA Resource entry as an HD0 streaming-music path.
void CSWSAmbientSound::CacheSound(int nTrack, int bStreaming)
{
    CExoString sResource;
    CExoString sPath;

    C2DA* pTable = bStreaming ? g_pRules->m_p2DArrays->m_pAmbientMusic
                              : g_pRules->m_p2DArrays->m_pAmbientSound;

    if (pTable->GetCExoStringEntry(nTrack, "Resource", &sResource))
    {
        sPath.Format(const_cast<char*>("HD0:STREAMMUSIC\\%s"), sResource.CStr());
        g_pExoResMan->CacheResource(sPath, 11);
    }
}

// Writes the same fields Load reads; the three boolean flags are never persisted.
void CSWSAmbientSound::Save(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldINT(pStruct, m_nMusicDelay, const_cast<char*>("MusicDelay"));
    pGff->WriteFieldINT(pStruct, m_nMusicDayTrack, const_cast<char*>("MusicDay"));
    pGff->WriteFieldINT(pStruct, m_nMusicNightTrack, const_cast<char*>("MusicNight"));
    pGff->WriteFieldINT(pStruct, m_nBattleMusicTrack, const_cast<char*>("MusicBattle"));
    pGff->WriteFieldINT(pStruct, m_nAmbientDayTrack, const_cast<char*>("AmbientSndDay"));
    pGff->WriteFieldINT(pStruct, m_nAmbientNightTrack, const_cast<char*>("AmbientSndNight"));
    pGff->WriteFieldINT(pStruct, m_nAmbientDayVolume, const_cast<char*>("AmbientSndDayVol"));
    pGff->WriteFieldINT(pStruct, m_nAmbientNightVolume, const_cast<char*>("AmbientSndNitVol"));
}

// Writes three booleans, six ints and the two volume bytes to the message.
void CSWSAmbientSound::PackIntoMessage(CSWSMessage* pMsg)
{
    pMsg->WriteBOOL(m_bMusicPlaying);
    pMsg->WriteBOOL(m_bBattleMusicPlaying);
    pMsg->WriteBOOL(m_bAmbientPlaying);

    pMsg->WriteINT(m_nMusicDelay, 32);
    pMsg->WriteINT(m_nMusicDayTrack, 32);
    pMsg->WriteINT(m_nMusicNightTrack, 32);
    pMsg->WriteINT(m_nBattleMusicTrack, 32);
    pMsg->WriteINT(m_nAmbientDayTrack, 32);
    pMsg->WriteINT(m_nAmbientNightTrack, 32);

    pMsg->WriteBYTE(m_nAmbientDayVolume, 8);
    pMsg->WriteBYTE(m_nAmbientNightVolume, 8);
}
