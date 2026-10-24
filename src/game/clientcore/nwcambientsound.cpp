#include "nwcambientsound.h"

#include "exosound.h"
#include "soundprioritygroup.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "worldtimer.h"
#include "exoresman.h"
#include "nwchandlemessage.h"
#include "nwsrules.h"
#include "nwcmodule.h"
#include "nwcarea.h"

extern CExoSound* g_pExoSound;
#include "port/crtapi.h"
#include "port/libcbase.h"

const CExoString CSWCAmbientSound::THUNDER1("as_wt_thundercl1");
const CExoString CSWCAmbientSound::THUNDER2("as_wt_thundercl2");
const CExoString CSWCAmbientSound::THUNDER_ROLLING1("as_wt_thunderds1");
const CExoString CSWCAmbientSound::THUNDER_ROLLING2("as_wt_thunderds2");

CSWCAmbientSound::CSWCAmbientSound(CSWCArea* pArea)
{
    m_pMusicSource = new CExoStreamingSoundSource(1);
    m_pAmbientSource = new CExoStreamingSoundSource(1);

    m_pMusicSource->SetLooping(0);
    m_pAmbientSource->SetLooping(1);
    m_pAmbientSource->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);

    m_pArea = pArea;

    m_bMusicEnabled = 1;
    m_bAmbientEnabled = 1;
    m_bSoundsStarted = 0;
    m_bIsDay = 1;

#if !KOTOR1  // K1 leaves m_bBattleMusicPlaying unset here (LoadFromMessage writes it)
    m_bBattleMusicPlaying = 0;
#endif

    m_pStingerSource = 0;
    m_bTruncatedPlayback = 0;
    m_nCurrentPriority = 0;
    m_bAmbientPlayFailed = 0;
}

CSWCAmbientSound::~CSWCAmbientSound()
{
    if (m_pMusicSource)
    {
        delete m_pMusicSource;
        m_pMusicSource = 0;
    }

    if (m_pAmbientSource)
    {
        delete m_pAmbientSource;
        m_pAmbientSource = 0;
    }
}

void CSWCAmbientSound::StartSounds()
{
    m_bSoundsStarted = 1;
    m_bTruncatedPlayback = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMinSpecMode & 3;

    if (m_bMusicEnabled)
    {
        if (m_bBattleMusicPlaying)
            PlayBattleMusic(1);
        else
            PlayMusic(m_bMusicPlaying);
    }

    if (m_bAmbientEnabled)
        PlayAmbientSound(m_bAmbientPlaying);
}

#if KOTOR1
void CSWCAmbientSound::SetMusicDayTrack(int nTrack)
{
    CExoString sPlaying;
    m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_pMusicSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_musicDayTrack.m_sResource) != 0)
    {
        PlayMusic(0);
    }

    ReadInfo(1, nTrack, &m_musicDayTrack);
    m_nMusicDayTrack = nTrack;

    PlayMusic(1);
    m_nMusicDelayCounter = 1;
}
#else
void CSWCAmbientSound::SetMusicDayTrack(int nTrack, int bStreaming)
{
    CExoString sPlaying;
    m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);
    m_pMusicSource->SetStreamingMusicForMovie(bStreaming);

    if (m_pMusicSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_musicDayTrack.m_sResource) != 0)
    {
        PlayMusic(0);
    }

    ReadInfo(1, nTrack, &m_musicDayTrack);
    m_nMusicDayTrack = nTrack;

    PlayMusic(1);
    m_nMusicDelayCounter = 1;
}
#endif

#if KOTOR1
void CSWCAmbientSound::SetMusicNightTrack(int nTrack)
{
    CExoString sPlaying;
    m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_pMusicSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_musicNightTrack.m_sResource) != 0)
    {
        PlayMusic(0);
    }

    ReadInfo(1, nTrack, &m_musicNightTrack);
    m_nMusicNightTrack = nTrack;

    PlayMusic(1);
    m_nMusicDelayCounter = 1;
}
#else
void CSWCAmbientSound::SetMusicNightTrack(int nTrack, int bStreaming)
{
    CExoString sPlaying;
    m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);
    m_pMusicSource->SetStreamingMusicForMovie(bStreaming);

    if (m_pMusicSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_musicNightTrack.m_sResource) != 0)
    {
        PlayMusic(0);
    }

    ReadInfo(1, nTrack, &m_musicNightTrack);
    m_nMusicNightTrack = nTrack;

    PlayMusic(1);
    m_nMusicDelayCounter = 1;
}
#endif

void CSWCAmbientSound::SetBattleMusicTrack(int nTrack)
{
    CExoString sPlaying;
    m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);

    bool bRestart = false;
    if (m_pMusicSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_battleMusicTrack.m_sResource) != 0)
    {
        PlayBattleMusic(0);
        bRestart = true;
    }

    ReadInfo(1, nTrack, &m_battleMusicTrack);
    m_nBattleMusicTrack = nTrack;

    if (bRestart)
        PlayBattleMusic(1);
}

void CSWCAmbientSound::SetAmbientDayTrack(int nTrack)
{
    CExoString sPlaying;
    m_pAmbientSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_pAmbientSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_ambientDayTrack.m_sResource) != 0)
    {
        PlayAmbientSound(0);
    }

    ReadInfo(0, nTrack, &m_ambientDayTrack);
    m_nAmbientDayTrack = nTrack;
    PlayAmbientSound(1);
}

void CSWCAmbientSound::SetAmbientNightTrack(int nTrack)
{
    CExoString sPlaying;
    m_pAmbientSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_pAmbientSource->IsPlaying() &&
        sPlaying.CompareNoCase(m_ambientNightTrack.m_sResource) != 0)
    {
        PlayAmbientSound(0);
    }

    ReadInfo(0, nTrack, &m_ambientNightTrack);
    m_nAmbientNightTrack = nTrack;
    PlayAmbientSound(1);
}

void CSWCAmbientSound::LoadFromMessage(CSWCMessage* pMsg)
{
    m_bMusicPlaying = pMsg->ReadBOOL();
    m_bBattleMusicPlaying = pMsg->ReadBOOL();
    m_bAmbientPlaying = pMsg->ReadBOOL();

    unsigned int nDelay = pMsg->ReadINT(32);
    m_nMusicDelay = nDelay > 1 ? nDelay : 1;

    m_nMusicDayTrack = pMsg->ReadINT(32);
    m_nMusicNightTrack = pMsg->ReadINT(32);
    m_nBattleMusicTrack = pMsg->ReadINT(32);
    m_nAmbientDayTrack = pMsg->ReadINT(32);
    m_nAmbientNightTrack = pMsg->ReadINT(32);

    m_nAmbientDayVolume = pMsg->ReadBYTE(8);
    m_nAmbientNightVolume = pMsg->ReadBYTE(8);

    ReadInfo(1, m_nMusicDayTrack, &m_musicDayTrack);
    ReadInfo(1, m_nMusicNightTrack, &m_musicNightTrack);
    ReadInfo(1, m_nBattleMusicTrack, &m_battleMusicTrack);
    ReadInfo(0, m_nAmbientDayTrack, &m_ambientDayTrack);
    ReadInfo(0, m_nAmbientNightTrack, &m_ambientNightTrack);
}

void CSWCAmbientSound::PlayBattleMusic(int nState)
{
    // Unused, but its default construction and destruction are deliberate.
    CResRef resRef;
    CExoString sPlaying;

    if (m_bTruncatedPlayback)
    {
        PlayTruncated(nState, SOUND_PRIORITY_BATTLE);
        return;
    }

    if (nState)
    {
        if (!m_bMusicEnabled)
            return;
        if (!m_bSoundsStarted)
            return;

        if (m_battleMusicTrack.m_sResource != "")
        {
            m_bBattleMusicPlaying = 1;

            if (m_pMusicSource->IsPlaying())
            {
                m_pMusicSource->GetFilePlaying().CopyToString(sPlaying);
                if (sPlaying.CompareNoCase(m_battleMusicTrack.m_sResource))
                    return;

                m_pMusicSource->FadeAndStop(800);
                m_bMusicPlaying = 0;
                m_nMusicDelayCounter = 10;
                g_pAppManager->m_pClientExoApp->GetPauseTimer()->GetWorldTime(
                    &m_nLastUpdateDay, &m_nLastUpdateTime);
            }
            else
            {
                m_pMusicSource->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);

                if (m_pMusicSource->Play(CResRef(m_battleMusicTrack.m_sResource), 8, 0, 1, 0))
                    m_nMusicDelayCounter = 0;
                else
                    m_nMusicDelayCounter = 10000;
            }
        }
    }
    else
    {
        if (!m_bBattleMusicPlaying)
            return;

#if !KOTOR1  // K1 fades the battle track without clearing its loop flag
        m_pMusicSource->SetLooping(0);
#endif
        m_pMusicSource->FadeAndStop(800);

        if (m_bMusicEnabled)
        {
            PlayStinger(&m_battleMusicTrack);
            PlayMusic(1);
        }

        m_bBattleMusicPlaying = 0;
    }
}

void CSWCAmbientSound::PlayAmbientSound(int nState)
{
    if (!m_bSoundsStarted)
        return;

    if (m_bTruncatedPlayback)
    {
        PlayTruncated(nState, SOUND_PRIORITY_AMBIENT);
        return;
    }

    if (nState)
    {
        if (!m_bAmbientEnabled)
            return;

        if (!m_pAmbientSource->IsPlaying())
        {
            if (m_bIsDay)
            {
                m_pAmbientSource->SetVolume(m_nAmbientDayVolume);
                m_pAmbientSource->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);

                m_bAmbientPlayFailed = m_pAmbientSource->Play(CResRef(m_ambientDayTrack.m_sResource), 8, 0, 1, 0) == 0;
                if (m_bAmbientPlayFailed)
                    m_bAmbientPlayFailed = m_pAmbientSource->Play(CResRef(m_ambientDayTrack.m_sResource), 4, 0, 1, 0) == 0;
            }
            else
            {
                m_pAmbientSource->SetVolume(m_nAmbientNightVolume);
                m_pAmbientSource->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);

#if KOTOR1  // K1 starts the AL_EN_CITYEXT night loop with mode 4, not 8
                int nBuffers = m_ambientNightTrack.m_sResource == "AL_EN_CITYEXT" ? 4 : 8;
                m_bAmbientPlayFailed = m_pAmbientSource->Play(CResRef(m_ambientNightTrack.m_sResource), static_cast<unsigned short>(nBuffers), 0, 1, 0) == 0;
#else
                m_bAmbientPlayFailed = m_pAmbientSource->Play(CResRef(m_ambientNightTrack.m_sResource), 8, 0, 1, 0) == 0;
#endif
                if (m_bAmbientPlayFailed)
                    m_bAmbientPlayFailed = m_pAmbientSource->Play(CResRef(m_ambientNightTrack.m_sResource), 4, 0, 1, 0) == 0;
            }
        }

        m_bAmbientPlaying = 1;
    }
    else
    {
        m_pAmbientSource->FadeAndStop(6000);
        m_bAmbientPlaying = 0;
        m_bAmbientPlayFailed = 0;
    }
}

void CSWCAmbientSound::PlayMusic(int nState)
{
    if (!m_bSoundsStarted)
        return;

    if (m_bTruncatedPlayback)
    {
        PlayTruncated(nState, SOUND_PRIORITY_MUSIC);
        return;
    }

    if (nState)
    {
        if (!m_bMusicEnabled)
            return;

        if (!m_pMusicSource->IsPlaying())
        {
            m_nMusicDelayCounter = 0;
            m_pMusicSource->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);

#if KOTOR1  // K1: no preloaded load-screen music; the night track MUS_AREA_TOWNINT never starts
            int bPlayed;
            if (m_bIsDay)
            {
                bPlayed = m_pMusicSource->Play(CResRef(m_musicDayTrack.m_sResource), 8, 0, 1, 0);
            }
            else
            {
                if (m_musicNightTrack.m_sResource == "MUS_AREA_TOWNINT")
                    return;
                bPlayed = m_pMusicSource->Play(CResRef(m_musicNightTrack.m_sResource), 8, 0, 1, 0);
            }
            if (!bPlayed)
                m_nMusicDelayCounter = 10000;
#else
            if (m_bIsDay)
            {
                CExoStreamingSoundSource* pSource =
                    g_pAppManager->m_pClientExoApp->AquireLoadMusic(CResRef(m_musicDayTrack.m_sResource));
                CExoStreamingSoundSource* pMusic = m_pMusicSource;
                if (pSource)
                {
                    delete pMusic;
                    m_pMusicSource = pSource;
                    m_pMusicSource->SetLooping(0);
                    m_pMusicSource->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);
                }
                else
                {
                    CResRef resRef2(m_musicDayTrack.m_sResource);
                    if (!pMusic->Play(resRef2, 8, 0, 1, 0))
                        m_nMusicDelayCounter = 10000;
                }
            }
            else
            {
                CExoStreamingSoundSource* pSource =
                    g_pAppManager->m_pClientExoApp->AquireLoadMusic(CResRef(m_musicNightTrack.m_sResource));
                CExoStreamingSoundSource* pMusic = m_pMusicSource;
                if (pSource)
                {
                    delete pMusic;
                    m_pMusicSource = pSource;
                    m_pMusicSource->SetLooping(0);
                    m_pMusicSource->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);
                }
                else
                {
                    CResRef resRef2(m_musicNightTrack.m_sResource);
                    if (!pMusic->Play(resRef2, 8, 0, 1, 0))
                        m_nMusicDelayCounter = 10000;
                }
            }
#endif
        }

        m_bMusicPlaying = 1;
    }
    else
    {
        m_pMusicSource->FadeAndStop(800);
        m_bMusicPlaying = 0;
    }
}

void CSWCAmbientSound::PlayStinger(CSWCAmbientSoundTrack* pTrack)
{
    CResRef resRef;

    if (!m_bSoundsStarted)
        return;

    if (pTrack->m_sStinger1 != "")
    {
        int nCount = 1;
        if (pTrack->m_sStinger2 != "")
            nCount = pTrack->m_sStinger3 != "" ? 3 : 2;

        switch (rand_wincompatible() % nCount)
        {
        case 0:  resRef = pTrack->m_sStinger1; break;
        case 1:  resRef = pTrack->m_sStinger2; break;
        case 2:  resRef = pTrack->m_sStinger3; break;
        }

        delete m_pStingerSource;
        m_pStingerSource = new CExoStreamingSoundSource(1);
        m_pStingerSource->SetPriorityGroup(PRIORITY_GROUP_MUSIC_STINGERS);

#if !KOTOR1  // K1 makes the single (4,0,0,0) attempt only
        if (m_pStingerSource->Play(resRef, 8, 0, 1, 0))
            return;
#endif

        if (m_pStingerSource->Play(resRef, 4, 0, 0, 0))
            return;

        delete m_pStingerSource;
        m_pStingerSource = 0;
    }
}

void CSWCAmbientSound::PlayTruncated(int nPlay, CSWCAmbientSoundPriority nPriority)
{
    if (nPriority == SOUND_PRIORITY_MUSIC)
        m_bMusicPlaying = nPlay;
    else if (nPriority == SOUND_PRIORITY_AMBIENT)
        m_bAmbientPlaying = nPlay;
    else if (nPriority == SOUND_PRIORITY_BATTLE)
        m_bBattleMusicPlaying = nPlay;

    m_nMusicDelayCounter = 10;

    if (m_pMusicSource->IsPlaying())
    {
        if (m_nCurrentPriority < nPriority)
        {
            if (nPlay)
                m_pMusicSource->FadeAndStop(1000);
        }
        else
        {
            if (nPlay)
                return;

            if (m_nCurrentPriority == nPriority)
            {
                m_pMusicSource->FadeAndStop(1000);
                if (nPriority == SOUND_PRIORITY_BATTLE)
                    PlayStinger(&m_battleMusicTrack);
            }
        }
    }
    else
    {
        if (!nPlay)
            return;

        if (m_bBattleMusicPlaying)
        {
            if (!m_pMusicSource->Play(CResRef(m_battleMusicTrack.m_sResource), 8, 0, 1, 0))
                m_nMusicDelayCounter = 10000;
            m_nCurrentPriority = SOUND_PRIORITY_BATTLE;
        }
        else if (m_bAmbientPlaying)
        {
            if (m_bIsDay)
            {
                if (!m_pMusicSource->Play(CResRef(m_ambientDayTrack.m_sResource), 4, 0, 1, 0))
                    m_nMusicDelayCounter = 10000;
            }
            else
            {
                if (!m_pMusicSource->Play(CResRef(m_ambientNightTrack.m_sResource), 4, 0, 1, 0))
                    m_nMusicDelayCounter = 10000;
            }
            m_nCurrentPriority = SOUND_PRIORITY_AMBIENT;
        }
        else if (m_bMusicPlaying)
        {
            if (m_bIsDay)
            {
                if (!m_pMusicSource->Play(CResRef(m_musicDayTrack.m_sResource), 8, 0, 1, 0))
                    m_nMusicDelayCounter = 10000;
            }
            else
            {
                if (!m_pMusicSource->Play(CResRef(m_musicNightTrack.m_sResource), 8, 0, 1, 0))
                    m_nMusicDelayCounter = 10000;
            }
            m_nCurrentPriority = SOUND_PRIORITY_MUSIC;
        }
        else
        {
            m_nMusicDelayCounter = 10000;
            m_nCurrentPriority = SOUND_PRIORITY_NONE;
        }
    }
}

void CSWCAmbientSound::ReadInfo(int bStreaming, int nTrack, CSWCAmbientSoundTrack* pTrack)
{
    CExoString sValue;
    C2DA* pTable;

    if (bStreaming)
    {
        pTable = g_pRules->m_p2DArrays->m_pAmbientMusic;

        if (pTable->GetCExoStringEntry(nTrack, "Resource", &sValue))
            pTrack->m_sResource = sValue.UpperCase();
        else
            pTrack->m_sResource = "";

        if (pTable->GetCExoStringEntry(nTrack, "Stinger1", &sValue))
            pTrack->m_sStinger1 = sValue.UpperCase();
        else
            pTrack->m_sStinger1 = "";

        if (pTable->GetCExoStringEntry(nTrack, "Stinger2", &sValue))
            pTrack->m_sStinger2 = sValue.UpperCase();
        else
            pTrack->m_sStinger2 = "";

        if (pTable->GetCExoStringEntry(nTrack, "Stinger3", &sValue))
            pTrack->m_sStinger3 = sValue.UpperCase();
        else
            pTrack->m_sStinger3 = "";
    }
    else
    {
        pTable = g_pRules->m_p2DArrays->m_pAmbientSound;

        if (pTable->GetCExoStringEntry(nTrack, "Resource", &sValue))
        {
            pTrack->m_sResource = sValue.UpperCase();
            m_bAmbientPlayFailed = 0;
        }
        else
        {
            pTrack->m_sResource = "";
        }
    }
}

void CSWCAmbientSound::UpdateTimeOfDay()
{
    CExoString sMusic;
    CExoString sAmbient;

    m_pMusicSource->GetFilePlaying().CopyToString(sMusic);
    m_pAmbientSource->GetFilePlaying().CopyToString(sAmbient);

    switch (g_pAppManager->m_pClientExoApp->GetModule()->m_nTimeOfDayState)
    {
    case CSWCModule::TIME_OF_DAY_NONE:
    case CSWCModule::TIME_OF_DAY_DAY:
    case CSWCModule::TIME_OF_DAY_DAWN:
        if (m_bIsDay != 1)
        {
            if (sMusic.CompareNoCase(m_musicNightTrack.m_sResource) != 0 ||
                (m_bTruncatedPlayback && sMusic.CompareNoCase(m_ambientNightTrack.m_sResource) != 0))
            {
                PlayMusic(0);
                PlayMusic(1);
            }

            if (sAmbient.CompareNoCase(m_ambientNightTrack.m_sResource) != 0)
            {
                PlayAmbientSound(0);
                PlayAmbientSound(1);
            }
        }

        m_bIsDay = 1;
        break;

    case CSWCModule::TIME_OF_DAY_NIGHT:
    case CSWCModule::TIME_OF_DAY_DUSK:
        if (m_bIsDay != 0)
        {
            if (sMusic.CompareNoCase(m_musicDayTrack.m_sResource) != 0 ||
                (m_bTruncatedPlayback && sMusic.CompareNoCase(m_ambientDayTrack.m_sResource) != 0))
            {
                PlayMusic(0);
                PlayMusic(1);
            }

            if (sAmbient.CompareNoCase(m_ambientDayTrack.m_sResource) != 0)
            {
                PlayAmbientSound(0);
                PlayAmbientSound(1);
            }
        }

        m_bIsDay = 0;
        break;
    }
}

void CSWCAmbientSound::Thunder(int bClose)
{
    if (!m_bAmbientPlaying)
        return;

    int nRand = rand_wincompatible();

    if (bClose)
    {
        if ((nRand & 1) == 0)
            g_pExoSound->PlayOneShotSound(CResRef(THUNDER_ROLLING1), 21, 0, 0, 0.0f, 0.0f);
        else
            g_pExoSound->PlayOneShotSound(CResRef(THUNDER_ROLLING2), 21, 0, 0, 0.0f, 0.0f);
    }
    else
    {
        if ((nRand & 1) == 0)
            g_pExoSound->PlayOneShotSound(CResRef(THUNDER1), 21, 0, 0, 0.0f, 0.0f);
        else
            g_pExoSound->PlayOneShotSound(CResRef(THUNDER2), 21, 0, 0, 0.0f, 0.0f);
    }
}

void CSWCAmbientSound::Update()
{
    if (!m_pMusicSource->IsPlaying() &&
        g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE_ALL &&
        !g_pExoSound->GetLostFocusPaused())
    {
        if (m_nMusicDelayCounter)
        {
            unsigned int nDelta = GetTimeDiff();
            if (static_cast<unsigned int>(m_nMusicDelayCounter) > nDelta)
            {
                m_nMusicDelayCounter -= nDelta;
                g_pAppManager->m_pClientExoApp->GetPauseTimer()->GetWorldTime(
                    &m_nLastUpdateDay, &m_nLastUpdateTime);
            }
            else
            {
                m_nMusicDelayCounter = 0;
                if (m_bTruncatedPlayback)
                    PlayTruncated(1, SOUND_PRIORITY_NONE);
                else if (m_bBattleMusicPlaying)
                    PlayBattleMusic(1);
                else
                    PlayMusic(1);
            }
        }
        else if (m_bMusicPlaying || m_bBattleMusicPlaying)
        {
            if (m_bTruncatedPlayback)
                m_nMusicDelayCounter = 10;
            else if (m_bBattleMusicPlaying)
                m_nMusicDelayCounter = 1000;
            else
                m_nMusicDelayCounter = m_nMusicDelay > 1 ? m_nMusicDelay : 1;

            g_pAppManager->m_pClientExoApp->GetPauseTimer()->GetWorldTime(
                &m_nLastUpdateDay, &m_nLastUpdateTime);
        }
    }

    if (m_bTruncatedPlayback)
        return;

    if (!m_pAmbientSource->IsPlaying() &&
        g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE_ALL &&
        g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC &&
        m_bAmbientPlaying && !m_bAmbientPlayFailed &&
        !g_pExoSound->GetLostFocusPaused())
    {
        PlayAmbientSound(1);
    }
}

int CSWCAmbientSound::GetTimeDiff()
{
    unsigned int nDay, nTime;
    g_pAppManager->m_pClientExoApp->GetPauseTimer()->GetWorldTime(&nDay, &nTime);

    unsigned int dDay, dTime;
    g_pAppManager->m_pClientExoApp->GetPauseTimer()->SubtractWorldTimes(
        nDay, nTime, m_nLastUpdateDay, m_nLastUpdateTime, &dDay, &dTime);

    return dTime;
}

void CSWCAmbientSound::ResetLastUpdate()
{
    g_pAppManager->m_pClientExoApp->GetPauseTimer()->GetWorldTime(
        &m_nLastUpdateDay, &m_nLastUpdateTime);
}

void CSWCAmbientSound::SetAmbientDayVolume(int nVolume)
{
    CExoString sPlaying;
    m_pAmbientSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_nAmbientDayVolume != nVolume)
    {
        if (m_pAmbientSource->IsPlaying() &&
            sPlaying.CompareNoCase(m_ambientDayTrack.m_sResource) != 0)
        {
            m_pAmbientSource->SetVolume(static_cast<unsigned char>(nVolume));
        }

        m_nAmbientDayVolume = static_cast<unsigned char>(nVolume);
    }
}

void CSWCAmbientSound::SetAmbientNightVolume(int nVolume)
{
    CExoString sPlaying;
    m_pAmbientSource->GetFilePlaying().CopyToString(sPlaying);

    if (m_nAmbientNightVolume != nVolume)
    {
        if (m_pAmbientSource->IsPlaying() &&
            sPlaying.CompareNoCase(m_ambientNightTrack.m_sResource) != 0)
        {
            m_pAmbientSource->SetVolume(static_cast<unsigned char>(nVolume));
        }

        m_nAmbientNightVolume = static_cast<unsigned char>(nVolume);
    }
}

void CSWCAmbientSound::StopSounds(int a)
{
    StopAllMusic();

    if (a)
    {
        m_pAmbientSource->Stop(0);
        m_bAmbientPlaying = 0;
        m_bAmbientPlayFailed = 0;
    }
    else
    {
        PlayAmbientSound(0);
    }
}

void CSWCAmbientSound::StopAllMusic()
{
    PlayMusic(0);
    m_bBattleMusicPlaying = 0;
    m_nMusicDelayCounter = 0;
}

void CSWCAmbientSound::SetMusicVolume(unsigned char nVolume)
{
    nVolume = CExoSound::CMAX_VOLUME > nVolume ? nVolume : CExoSound::CMAX_VOLUME;

    m_pMusicSource->SetVolume(nVolume);
}

void CSWCAmbientSound::SetMusicDelay(int delay)
{
    if (delay > 0)
        m_nMusicDelay = delay;
}

const char* SetMusicVolume(char* pszArgs)
{
    if (g_pAppManager->m_pClientExoApp == 0)
        return "Failed";
    if (g_pAppManager->m_pClientExoApp->GetModule() == 0)
        return "Failed";
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea == 0)
        return "Failed";
    CSWCAmbientSound* pAmbientSound =
        g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pAmbientSound;
    if (pAmbientSound == 0)
        return "Failed";

    int nVolume = atoi(pszArgs);
    if (nVolume < 0 || nVolume > CExoSound::CMAX_VOLUME)
        return "Failed";

    pAmbientSound->m_pMusicSource->SetVolume(
        CExoSound::CMAX_VOLUME > static_cast<unsigned char>(nVolume) ? static_cast<unsigned char>(nVolume) : CExoSound::CMAX_VOLUME);
    return "Success";
}
