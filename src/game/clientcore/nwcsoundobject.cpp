
#include "nwcsoundobject.h"
#include "CExoArrayListImpl.h"
#include "exobase.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "nwchandlemessage.h"
#include "nwmessage.h"
#include "worldtimer.h"
#include "gameobjectarray.h"
#include "clientaimaster.h"
#include "exosound.h"
#include "nwcmodule.h"
#include "nwcarea.h"

#include "port/crtapi.h"
#include "soundprioritygroup.h"

const int CSWCSoundObject::MAX_VOLUME = 127;

// Exactly one of the two sources is non-null by construction, so neither is null-checked; the
// streaming arm is gated on m_bStreamingSound for the same reason.
int CSWCSoundObject::GetIsPlaying()
{
    if (!m_bStreamingSound && m_pSoundSource->IsHardwarePlayingSoundPleaseDontUseThis())
        return 1;
    // Gated on the flag: a non-streaming sound's null streaming source must not be dereferenced.
    else if (m_bStreamingSound && m_pStreamingSoundSource->IsPlaying())
        return 1;
    return 0;
}


CSWCSoundObject::CSWCSoundObject()
    : CSWCObject(CGameObject::OBJECT_TYPE_SOUND)
{
    m_bStreamingSound = 0;
    m_nNextSoundInterval = 0;
    m_pSoundSource = 0;
    m_pStreamingSoundSource = 0;
    m_bActive = 1;
    m_nVolume = MAX_VOLUME;
    m_nVolumeVariation = 0;
    m_nTimes = 3;
    m_fFixedVariance = 1.0f;
    m_fPitchVariation = 0.0f;
    m_nHours = 0;
    m_bTimeOfDayActive = 1;
    m_nGeneratedType = 0;
    m_nInterval = 0;
    m_nIntervalVariation = 0;
    m_bContinuous = 0;
    m_bRandom = 0;
    m_pSoundList = new CExoArrayList<CResRef>;
    m_fRandomRangeX = 0.0f;
    m_fRandomRangeY = 0.0f;
    m_nSoundIndex = 0;
    m_bRestartList = 0;
    m_bRandomPosition = 0;
    m_bLooping = 0;
    // m_fBaseZ, m_nSoundTimeLeft and the last-update stamp are deliberately not initialised.
}

// Frees the two sources and the resref list.
CSWCSoundObject::~CSWCSoundObject()
{
    if (m_pSoundSource != 0) {
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }
    if (m_pStreamingSoundSource != 0) {
        delete m_pStreamingSoundSource;
        m_pStreamingSoundSource = 0;
    }
    if (m_pSoundList != 0) {
        delete m_pSoundList;
        m_pSoundList = 0;
    }
}

// Reads the sound's description off the wire, field for field with CSWSSoundObject::PackIntoMessage.
void CSWCSoundObject::LoadFromMessage(CSWCMessage* pMsg)
{
    SetId(pMsg->ReadOBJECTIDClient());

    int bActive = pMsg->ReadBOOL();
    int bPositional = pMsg->ReadBOOL();
    int bLooping = pMsg->ReadBOOL();

    if (bLooping) {
        m_pStreamingSoundSource = new CExoStreamingSoundSource(0);
        m_bStreamingSound = 1;
    } else {
        m_pSoundSource = new CExoSoundSource;
    }

    SetIsActive(bActive);
    SetPositional(bPositional);
    SetLooping(bLooping);

    if (bLooping) {
        m_pStreamingSoundSource->SetLooping(1);
        if (bPositional) {
            m_pStreamingSoundSource->Set3D(1);
            m_pStreamingSoundSource->SetPriorityGroup(PRIORITY_GROUP_LOOPING_POSITIONAL_AMBIENTS);
        } else {
            m_pStreamingSoundSource->Set3D(0);
            m_pStreamingSoundSource->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);
        }
    } else {
        m_pSoundSource->SetPriorityGroup(bPositional ? PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL : PRIORITY_GROUP_SINGLE_SHOT_GLOBAL);
    }

    m_nVolume = pMsg->ReadBYTE(8);
    m_nVolumeVariation = pMsg->ReadBYTE(8);
    m_nTimes = pMsg->ReadBYTE(8);
    m_fPitchVariation = pMsg->ReadFLOAT(1.0f, 32);
    m_fFixedVariance = pMsg->ReadFLOAT(1.0f, 32);
    m_nHours = pMsg->ReadDWORD(32);
    m_nGeneratedType = pMsg->ReadBYTE(8);
    m_nInterval = pMsg->ReadDWORD(32);
    m_nIntervalVariation = pMsg->ReadDWORD(32);
    float fMinDistance = pMsg->ReadFLOAT(1.0f, 32);
    float fMaxDistance = pMsg->ReadFLOAT(1.0f, 32);
    SetDistance(fMinDistance, fMaxDistance);
    m_bContinuous = pMsg->ReadBOOL();
    m_bRandom = pMsg->ReadBOOL();
    m_bRandomPosition = pMsg->ReadBOOL();
    m_fRandomRangeX = pMsg->ReadFLOAT(1.0f, 32);
    m_fRandomRangeY = pMsg->ReadFLOAT(1.0f, 32);

    Vector vPosition;
    vPosition.x = pMsg->ReadFLOAT(1.0f, 32);
    vPosition.y = pMsg->ReadFLOAT(1.0f, 32);
    vPosition.z = pMsg->ReadFLOAT(1.0f, 32);
    // The z is cached so ChangePosition can offset later positions by it.
    m_fBaseZ = vPosition.z;
    SetPosition(vPosition);

    unsigned short nCount = pMsg->ReadWORD(16);
    for (unsigned short i = 0; i < nCount; i++)
        m_pSoundList->Add(pMsg->ReadCResRef(16));

    m_bRestartList = 1;
    m_nSoundTimeLeft = 0;
    // A single resref that may repeat, and does not shuffle, loops outright unless a repeat
    // interval is configured; the interval path handles the restart in that case.
    if (nCount == 1 && m_bContinuous && !m_bRandomPosition) {
        if (m_nInterval == 0)
            SetLooping(1);
    }
    UpdateIntervalVariation();
}

void CSWCSoundObject::SetIsActive(int bActive)
{
    if (m_bActive != bActive) {
        m_nNextSoundInterval = 0;
        m_bActive = bActive;
        if (bActive) {
            if (GetLooping())
                m_bLooping = 1;
        }
    }
}

// Next countdown is the base interval less the variation plus a random roll of up to twice it.
void CSWCSoundObject::UpdateIntervalVariation()
{
    if (m_nInterval != 0) {
        if (m_nIntervalVariation != 0) {
            int nBase = m_nInterval - m_nIntervalVariation;
            unsigned int nRoll = g_pExoBase->m_pRand->Rand();
            int nInterval = nBase + nRoll % static_cast<unsigned int>(m_nIntervalVariation * 2);
            if (nInterval < 0)
                nInterval = 0;
            m_nNextSoundInterval = nInterval;
        } else {
            m_nNextSoundInterval = m_nInterval;
        }
    }
}

int CSWCSoundObject::AddToArea(CSWCArea* pArea, CGameObjectArray* pArray)
{
    CClientAIMaster* pAIMaster = g_pAppManager->m_pClientExoApp->GetClientAIMaster();

    SetArea(pArea);
    pArea->m_soundObjectIdList.AddUnique(m_objectId);

    if (pArray->AddExternalObject(m_objectId, this, 0) != CGameObjectArray::SUCCESS)
        return 0;
    if (!pAIMaster->AddObject(this))
        return 0;

    ResetLastUpdate();
    if (GetLooping() && m_bActive)
        m_bLooping = 1;
    return 1;
}

void CSWCSoundObject::ResetLastUpdate()
{
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&m_nLastUpdateDay,
                                                                 &m_nLastUpdateTime);
}

// Restarts the sound and marks it active.
void CSWCSoundObject::Play()
{
    CResRef rUnused;

    if (!GetIsPlaying()) {
        m_nSoundTimeLeft = 0;
        m_nNextSoundInterval = 0;
        m_bRestartList = 1;
        ResetLastUpdate();
    }
    SetIsActive(1);
}

void CSWCSoundObject::Stop()
{
    SetIsActive(0);
    if (m_bStreamingSound)
        m_pStreamingSoundSource->Stop(0);
    else
        m_pSoundSource->Stop();
    m_nSoundTimeLeft = 0;
    m_nNextSoundInterval = 0;
}

// Fades the streaming source out; any other source just stops.
void CSWCSoundObject::FadeAndStop(unsigned int nMilliseconds)
{
    SetIsActive(0);
    if (m_bStreamingSound)
        m_pStreamingSoundSource->FadeAndStop(nMilliseconds);
    else
        m_pSoundSource->Stop();
    m_nSoundTimeLeft = 0;
    m_nNextSoundInterval = 0;
}

void CSWCSoundObject::ChangeVolume(int nVolume)
{
    if (m_bStreamingSound)
        m_pStreamingSoundSource->SetVolume(static_cast<unsigned char>(nVolume));
    else
        m_pSoundSource->SetVolume(static_cast<unsigned char>(nVolume));
    m_nVolume = nVolume;
}

// Only the non-streaming source has a pitch variance, but both cache the value.
void CSWCSoundObject::ChangePitchVariation(float fVariation)
{
    if (!m_bStreamingSound)
        m_pSoundSource->SetPitchVariance(fVariation);
    m_fPitchVariation = fVariation;
}

void CSWCSoundObject::ChangeFixedVariance(float fVariance)
{
    if (!m_bStreamingSound)
        m_pSoundSource->SetFixedVariance(fVariance);
    m_fFixedVariance = fVariance;
}

// The z is offset by the value LoadFromMessage cached, so the server's absolute position does not
// reach the source verbatim.
void CSWCSoundObject::ChangePosition(const Vector& v)
{
    Vector vPosition = v;
    vPosition.z = m_fBaseZ + vPosition.z;

    if (m_bStreamingSound)
        m_pStreamingSoundSource->SetPosition(vPosition);
    else
        m_pSoundSource->SetPosition(vPosition, 0.0f);

    SetPosition(vPosition);
}

// One tick: advances the countdowns for the playing sound and for the gap before the next one, and
// takes the next resref off the list once both have expired.
void CSWCSoundObject::AIUpdate()
{
    CResRef r;

    if (!m_bActive) {
        if (m_bStreamingSound) {
            if (m_pStreamingSoundSource->IsPlaying())
                m_pStreamingSoundSource->Stop(0);
        } else {
            if (m_pSoundSource->IsHardwarePlayingSoundPleaseDontUseThis())
                m_pSoundSource->Stop();
        }
        return;
    }

    if (m_nTimes == 0)
        UpdateTimeOfDay();

    if (m_bTimeOfDayActive == 0) {
        // Wrong time of day: shut the sound down and stay inactive.
        if (m_bStreamingSound)
            m_pStreamingSoundSource->Stop(0);
        else if (m_pSoundSource->IsHardwarePlayingSoundPleaseDontUseThis())
            m_pSoundSource->Stop();
        return;
    }

    if (GetLooping()) {
        if (m_bLooping) {
            m_bLooping = 0;
            m_bRestartList = 1;
            if (m_bStreamingSound) {
                m_pStreamingSoundSource->SetVolume(static_cast<unsigned char>(m_nVolume));
                m_nSoundTimeLeft = m_pStreamingSoundSource->Play(GetNextSound(), 4, 0, 0, 0);
            } else {
                m_pSoundSource->SetResRef(GetNextSound(), 0);
                m_pSoundSource->SetVolume(static_cast<unsigned char>(m_nVolume));
                m_nSoundTimeLeft = m_pSoundSource->Play();
            }
            if (m_nSoundTimeLeft == 0) {
                SetIsActive(0);
                return;
            }
        }
        return;
    }

    unsigned int nElapsed = GetTimeDiff();
    ResetLastUpdate();

    if (m_nSoundTimeLeft != 0) {
        if (static_cast<unsigned int>(m_nSoundTimeLeft) > nElapsed) {
            m_nSoundTimeLeft -= nElapsed;
            return;
        }
        m_nSoundTimeLeft = 0;
        nElapsed = 0;
    }
    if (m_nNextSoundInterval != 0) {
        if (static_cast<unsigned int>(m_nNextSoundInterval) > nElapsed) {
            m_nNextSoundInterval -= nElapsed;
            return;
        }
        m_nNextSoundInterval = 0;
    }

    if ((r = GetNextSound()) == const_cast<char*>("0")) {
        SetIsActive(0);
        return;
    }

    if (m_bStreamingSound) {
        m_pStreamingSoundSource->SetVolume(static_cast<unsigned char>(m_nVolume));
        UpdateVolumeVariation();
        m_nSoundTimeLeft = m_pStreamingSoundSource->Play(r, 4, 0, 0, 0);
    } else {
        m_pSoundSource->SetResRef(r, 0);
        m_pSoundSource->SetVolume(static_cast<unsigned char>(m_nVolume));
        UpdateVolumeVariation();
        m_pSoundSource->SetPitchVariance(m_fPitchVariation);
        m_pSoundSource->SetFixedVariance(m_fFixedVariance);
        m_nSoundTimeLeft = m_pSoundSource->Play();
    }

    if (m_nSoundTimeLeft == 0) {
        if (m_pSoundList->m_count >= 2) {
            // Consume the entry that failed to play and step the cursor back onto the next one.
            m_pSoundList->DelIndex(m_nSoundIndex);
            if (m_nSoundIndex > 0)
                m_nSoundIndex--;
        } else {
            SetIsActive(0);
            return;
        }
    }

    if (m_bContinuous || m_nSoundIndex < m_pSoundList->m_count - 1)
        UpdateIntervalVariation();
}

// Decides whether the current time of day lets this sound play: either from the day/night state the
// message carried, or from the bitmask of states the sound is allowed in.
void CSWCSoundObject::UpdateTimeOfDay()
{
    if (m_nTimes != 0) {
        unsigned char nState = g_pAppManager->m_pClientExoApp->GetModule()->m_nTimeOfDayState;
        if (nState > 4)
            return;
        if ((1 << nState) & ((1 << 0) | (1 << CSWCModule::TIME_OF_DAY_DAY) | (1 << CSWCModule::TIME_OF_DAY_DAWN)))
            m_bTimeOfDayActive = (m_nTimes != 2);
        else if ((1 << nState) & ((1 << CSWCModule::TIME_OF_DAY_NIGHT) | (1 << CSWCModule::TIME_OF_DAY_DUSK)))
            m_bTimeOfDayActive = (m_nTimes != 1);
    } else {
        int nMask = m_nHours;
        if ((1 << g_pAppManager->m_pClientExoApp->GetModule()->m_nCalendarHour) & nMask)
            m_bTimeOfDayActive = 1;
        else
            m_bTimeOfDayActive = 0;
    }
}

// Takes the next resref off the list, reshuffling at the end when the list repeats; the literal
// "0" is the sentinel for an exhausted list.
CResRef CSWCSoundObject::GetNextSound()
{
    CResRef r;

    if (m_bRestartList) {
        m_bRestartList = 0;
        m_nSoundIndex = 0;
        r = m_pSoundList->m_pArray[0];
    } else if (m_nSoundIndex < m_pSoundList->m_count - 1) {
        m_nSoundIndex++;
        r = m_pSoundList->m_pArray[m_nSoundIndex];
    } else if (m_bContinuous) {
        if (m_bRandom)
            ShuffleList();
        m_nSoundIndex = 0;
        r = m_pSoundList->m_pArray[0];
    } else {
        r = const_cast<char*>("0");
    }
    return r;
}

// Elapsed time since the last update, in milliseconds.
int CSWCSoundObject::GetTimeDiff()
{
    unsigned int nDay;
    unsigned int nTime;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);

    unsigned int nUnused;
    unsigned int nElapsed;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTime, m_nLastUpdateDay, m_nLastUpdateTime, &nUnused, &nElapsed);
    return nElapsed;
}

// Volume jitter, uniform over [v - var, v + var) and clamped to [0, MAX_VOLUME].
void CSWCSoundObject::UpdateVolumeVariation()
{
    if (m_nVolumeVariation >= 1) {
        int nVolume = m_nVolume - m_nVolumeVariation +
                      rand_wincompatible() % (m_nVolumeVariation * 2);
        if (nVolume > MAX_VOLUME)
            nVolume = MAX_VOLUME;
        if (nVolume < 0)
            nVolume = 0;

        if (m_bStreamingSound)
            m_pStreamingSoundSource->SetVolume(static_cast<unsigned char>(nVolume));
        else
            m_pSoundSource->SetVolume(static_cast<unsigned char>(nVolume));
    }
}

void CSWCSoundObject::UpdateRandomPosition()
{
}

// Rebuilds the sound list as a random permutation of itself, picking a random surviving entry each
// pass and erasing it from the old list.
void CSWCSoundObject::ShuffleList()
{
    CExoArrayList<CResRef>* pShuffled = new CExoArrayList<CResRef>;

    int nCount = m_pSoundList->m_count;
    for (int i = 0; i < nCount; i++) {
        int nIndex = rand_wincompatible() % (nCount - i);
        pShuffled->Add(m_pSoundList->m_pArray[nIndex]);
        m_pSoundList->DelIndex(nIndex);
    }
    delete m_pSoundList;
    m_pSoundList = pShuffled;
}

CSWSSoundObject* CSWCSoundObject::GetServerSoundObject()
{
    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    unsigned long long nServerId = pServerExoApp->ClientToServerObjectId(m_objectId);
    return g_pAppManager->m_pServerExoApp->GetSoundObjectByGameObjectID(nServerId);
}

CSWCSoundObject* CSWCSoundObject::AsSWCSoundObject()
{
    return this;
}

int CSWCSoundObject::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(r); K2_UNUSED(a); K2_UNUSED(b);
    return 0;
}

void CSWCSoundObject::UnloadModel()
{
}

void CSWCSoundObject::UnloadModel(unsigned char a)
{
    K2_UNUSED(a);
}

void CSWCSoundObject::SetPosition(const Vector& v)
{
    CSWCObject::SetPosition(v);

    if (!m_bStreamingSound && m_pSoundSource != 0)
        m_pSoundSource->SetPosition(v, 0.0f);
    else if (m_pStreamingSoundSource != 0)
        m_pStreamingSoundSource->SetPosition(v);
}
