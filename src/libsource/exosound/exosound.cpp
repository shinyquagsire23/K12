#include "exosound.h"
#include "exosoundinternal.h"
#include "exostreamingsoundsourceinternal.h"
#include "exosoundsourceinternal.h"

const unsigned char CExoSound::CMAX_VOLUME = 127;
const int CExoSound::CEXOSOUND_MODE_GENERIC = 0;
const int CExoSound::CEXOSOUND_MODE_MUTE = 1;
const int CExoSound::CEXOSOUND_MODE_PAUSE = 2;
const int CExoSound::CEXOSOUND_MODE_PAUSE_ALL = 3;
const int CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC = 4;
const int CExoSound::CEXOSOUND_MODE_LOSTFOCUS = 5;
const int CExoSound::CEXOSOUND_MODE_GAINFOCUS = 6;

extern int g_bDisableSound;

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoSound::StartDuckingMusicAndAmbientSounds()
{
    if (m_pInternal)
        m_pInternal->StartDuckingMusicAndAmbientSounds();
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoSound::StopDuckingMusicAndAmbientSounds()
{
    if (m_pInternal)
        m_pInternal->StopDuckingMusicAndAmbientSounds();
}
#endif // !KOTOR1

CExoSound::CExoSound(unsigned char n2DVoices, unsigned char n3DVoices, int a, int b)
{
    m_pInternal = 0;
    if (g_bDisableSound == 0) {
        m_pInternal = new CExoSoundInternal();
        m_pInternal->Initialize(n2DVoices, n3DVoices, a, b);
    }
}

CExoSound::~CExoSound()
{
    if (m_pInternal) {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

void CExoSound::SetMusicVolume(float fVolume, int nChannel)
{
    if (m_pInternal) {
        fVolume = fVolume < 1.0f ? fVolume : 1.0f;
        if (fVolume < 0.0f) fVolume = 0.0f;
        m_pInternal->SetMusicVolume(fVolume, 0, nChannel);
        m_pInternal->ApplyNewVolume(0);
    }
}

void CExoSound::SetSoundEffectVolume(float fVolume)
{
    if (m_pInternal) {
        fVolume = fVolume < 1.0f ? fVolume : 1.0f;
        if (fVolume < 0.0f) fVolume = 0.0f;
        m_pInternal->SetSoundEffectVolume(fVolume, 0);
        m_pInternal->ApplyNewVolume(0);
    }
}

void CExoSound::SetDialogVolume(float fVolume)
{
    if (m_pInternal) {
        fVolume = fVolume < 1.0f ? fVolume : 1.0f;
#if KOTOR1  // K1's dialog floor is 0.0f
        if (fVolume < 0.0f) fVolume = 0.0f;
#else
        fVolume = fVolume <= 0.01f ? 0.01f : fVolume;
#endif
        m_pInternal->SetDialogVolume(fVolume, 0);
        m_pInternal->ApplyNewVolume(0);
    }
}

void CExoSound::Set2D3DBias(float fBias)
{
    if (m_pInternal) {
        fBias = fBias < 1.9f ? fBias : 1.9f;
        fBias = fBias <= 0.1f ? 0.1f : fBias;
        m_pInternal->m_f2D3DBias = fBias;
    }
}

float CExoSound::Get2D3DBias()
{
    if (m_pInternal)
        return m_pInternal->m_f2D3DBias;
    return 0.5f;
}

int CExoSound::Test2D3DBias()
{
    if (m_pInternal)
        return m_pInternal->Test2D3DBias();
    return 0;
}

float CExoSound::GetMusicVolume()
{
    if (m_pInternal)
        return m_pInternal->m_fMusicVolume;
    return 1.0f;
}

float CExoSound::GetSoundEffectVolume()
{
    if (m_pInternal)
        return m_pInternal->m_fSoundEffectVolume;
    return 1.0f;
}

float CExoSound::GetDialogVolume()
{
    if (m_pInternal)
        return m_pInternal->m_fDialogVolume;
    return 1.0f;
}

void CExoSound::SetRollOffFactor(float f)
{
    if (m_pInternal)
        m_pInternal->SetRollOffFactor(f);
}

int CExoSound::GetNumber2DVoices()
{
    if (m_pInternal)
        return m_pInternal->m_nNumber2DVoices;
    return 24;
}

int CExoSound::GetNumber3DVoices()
{
    if (m_pInternal)
        return m_pInternal->m_nNumber3DVoices;
    return 16;
}

void CExoSound::Render()
{
    if (m_pInternal)
        m_pInternal->Render();
}

void CExoSound::SetListenerOrientation(const Vector& vDir, const Vector& vUp)
{
    if (m_pInternal)
        m_pInternal->SetListenerOrientation(vDir, vUp);
}

void CExoSound::SetListenerPosition(const Vector& vPos)
{
    if (m_pInternal)
        m_pInternal->SetListenerPosition(vPos);
}

void CExoSound::GetListenerPosition(Vector* pOut)
{
    if (m_pInternal)
        *pOut = m_pInternal->m_vListenerPosition;
}

// Re-runs Initialize with the current voice counts floored at the engine minimums.
void CExoSound::ReInitialize(int a, int b)
{
    if (m_pInternal) {
        unsigned char n2D = m_pInternal->m_nNumber2DVoices;
        unsigned char n3D = m_pInternal->m_nNumber3DVoices;
        m_pInternal->Initialize(n2D > 24 ? n2D : 24, n3D > 16 ? n3D : 16, a, b);
    }
}

int CExoSound::PlayOneShotSound(const CResRef& r, unsigned char a, unsigned int b,
                                unsigned char c, float d, float e)
{
    if (m_pInternal)
        return m_pInternal->PlayOneShotSound(r, a, b, c, d, e);
    return 0;
}

int CExoSound::Play3DOneShotSound(const CResRef& r, Vector vPos, float fZOffset, unsigned char a,
                                  unsigned int b, unsigned char c, float d, float e)
{
    if (m_pInternal) {
        vPos.z += fZOffset;
        return m_pInternal->Play3DOneShotSound(r, vPos, a, b, c, d, e);
    }
    return 0;
}

void CExoSound::StopAllOneShots()
{
    if (m_pInternal)
        m_pInternal->StopAllOneShots();
}

int CExoSound::GetSpeakerType()
{
    if (m_pInternal)
        return m_pInternal->GetSpeakerType();
    return 0;
}

void CExoSound::SetEnvironment(unsigned char env)
{
    if (m_pInternal)
        m_pInternal->SetEnvironment(env);
}

void CExoSound::EnableStreamingEnvironmentEffects(int b)
{
    if (m_pInternal)
        m_pInternal->m_bStreamingEnvironmentEffects = b;
}

void CExoSound::EnableNonStreamingEnvironmentEffects(int b)
{
    if (m_pInternal)
        m_pInternal->m_bNonStreamingEnvironmentEffects = b;
}

int CExoSound::GetStreamingEnvironmentEffectsEnabled()
{
    if (m_pInternal)
        return m_pInternal->m_bStreamingEnvironmentEffects;
    return 0;
}

int CExoSound::GetNonStreamingEnvironmentEffectsEnabled()
{
    if (m_pInternal)
        return m_pInternal->m_bNonStreamingEnvironmentEffects;
    return 0;
}

void CExoSound::EnableHardwareSound(int b)
{
    if (m_pInternal)
        m_pInternal->m_bHardwareSound = b;
}

// Hardware sound counts as enabled when either the explicit flag or EAX is set.
int CExoSound::GetHardwareSoundEnabled()
{
    if (m_pInternal) {
        if (m_pInternal->m_bHardwareSound)
            return 1;
        return m_pInternal->m_nEAX != 0;
    }
    return 0;
}

void CExoSound::SetEAX(int n)
{
    if (m_pInternal)
        m_pInternal->m_nEAX = n;
}

int CExoSound::GetEAX()
{
    if (m_pInternal)
        return m_pInternal->m_nEAX;
    return 0;
}

int CExoSound::GetBestEAXAvailable()
{
    if (m_pInternal)
        return m_pInternal->m_nBestEAXAvailable;
    return 0;
}

void CExoSound::SetEnvironmentEffectLevel(float f)
{
    if (m_pInternal)
        m_pInternal->SetEnvironmentEffectLevel(f);
}

float CExoSound::GetEnvironmentEffectLevel()
{
    if (m_pInternal)
        return m_pInternal->m_fEnvironmentEffectLevel;
    return 0.0f;
}

int CExoSound::RemoveAllSounds()
{
    if (m_pInternal)
        return m_pInternal->RemoveAllSounds();
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoSound::ResumeAllStreamingMusicForMovie()
{
    if (m_pInternal)
        m_pInternal->ResumeAllStreamingMusicForMovie();
}
#endif // !KOTOR1

int CExoSound::GetDisplayOcclusionInfo()
{
    if (m_pInternal)
        return m_pInternal->m_bDisplayOcclusionInfo;
    return 0;
}

void CExoSound::SetDisplayOcclusionInfo(int b)
{
    if (m_pInternal)
        m_pInternal->m_bDisplayOcclusionInfo = b;
}

void CExoSound::SetDesiredEnvironment(int n)
{
    if (m_pInternal)
        m_pInternal->m_nDesiredEnvironment = n;
}

void CExoSound::SetDesiredAmbientScale(float f)
{
    if (m_pInternal)
        m_pInternal->m_fDesiredAmbientScale = f;
}

#if KOTOR1
void CExoSound::SetSoundMode(int mode)
{
    if (m_pInternal)
        m_pInternal->SetSoundMode(mode);
}
#else
void CExoSound::SetSoundMode(int mode, int flag)
{
    if (m_pInternal)
        m_pInternal->SetSoundMode(mode, flag);
}
#endif

// Returns the top of the sound-mode stack.
int CExoSound::GetSoundMode()
{
    if (m_pInternal)
        return m_pInternal->m_aSoundModes[m_pInternal->m_nSoundModeIndex];
    return 0;
}

// The outputs are reversed; pA receives the group's minimum distance, pDist its maximum.
void CExoSound::GetPriorityGroupDistance(int nGroup, float* pA, float* pDist)
{
    if (m_pInternal) {
        CPriorityGroup* pGroup = m_pInternal->GetPriorityGroup(static_cast<unsigned char>(nGroup));
        *pA = pGroup->m_fMinDistance;
        *pDist = pGroup->m_fMaxDistance;
    }
}

int CExoSound::GetLostFocusPaused()
{
    if (m_pInternal)
        return m_pInternal->m_bLostFocusPaused;
    return 0;
}

extern int g_bDisableSound;
extern CExoSound* g_pExoSound;

#if KOTOR1
CExoStreamingSoundSource::CExoStreamingSoundSource()
{
    if (g_bDisableSound) {
        m_pInternal = 0;
    } else {
        m_pInternal = new CExoStreamingSoundSourceInternal(g_pExoSound->m_pInternal);
    }
}
#else
CExoStreamingSoundSource::CExoStreamingSoundSource(int nType)
{
    if (g_bDisableSound) {
        m_pInternal = 0;
    } else {
        m_pInternal = new CExoStreamingSoundSourceInternal(g_pExoSound->m_pInternal, nType);
    }
}
#endif

CExoStreamingSoundSource::~CExoStreamingSoundSource()
{
    if (m_pInternal) {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

int CExoStreamingSoundSource::IsPlaying()
{
    if (m_pInternal)
        return m_pInternal->IsPlaying();
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CExoStreamingSoundSource::IsDone()
{
    if (m_pInternal)
        return m_pInternal->IsDone();
    return 0;
}
#endif // !KOTOR1

int CExoStreamingSoundSource::GetPlayFailed()
{
    if (m_pInternal)
        return m_pInternal->GetPlayFailed();
    return 0;
}

int CExoStreamingSoundSource::Play(const CResRef& resRef, unsigned short a, int b, int c, int d)
{
    if (m_pInternal)
        return m_pInternal->Play(resRef, a, b, c, d);
    return 0;
}

void CExoStreamingSoundSource::Pause()
{
    if (m_pInternal)
        m_pInternal->Pause();
}

void CExoStreamingSoundSource::Resume()
{
    if (m_pInternal)
        m_pInternal->Resume();
}

// An empty resref when there is no handle.
CResRef CExoStreamingSoundSource::GetFilePlaying()
{
    if (m_pInternal)
        return m_pInternal->m_resRef;
    return CResRef("");
}

void CExoStreamingSoundSource::SetLooping(int bLooping)
{
    if (m_pInternal)
        m_pInternal->SetLooping(bLooping);
}

int CExoStreamingSoundSource::GetLooping()
{
    if (m_pInternal)
        return m_pInternal->m_bLooping;
    return 0;
}

void CExoStreamingSoundSource::SetVolume(unsigned char nVolume)
{
    if (m_pInternal) {
        nVolume = nVolume < 127 ? nVolume : 127;
        m_pInternal->SetVolume(nVolume, 0, 1.0f);
    }
}

unsigned char CExoStreamingSoundSource::GetVolume()
{
    if (m_pInternal)
        return m_pInternal->m_nVolume;
    return 0;
}

#if KOTOR1
void CExoStreamingSoundSource::Stop()
{
    if (m_pInternal)
        m_pInternal->Stop();
}
#else
void CExoStreamingSoundSource::Stop(int bFade)
{
    if (m_pInternal)
        m_pInternal->Stop(bFade);
}
#endif

void CExoStreamingSoundSource::FadeAndStop(unsigned int nMilliseconds)
{
    if (m_pInternal)
        m_pInternal->FadeAndStop(nMilliseconds);
}

void CExoStreamingSoundSource::SetPriorityGroup(unsigned char nGroup)
{
    if (m_pInternal)
        m_pInternal->SetPriorityGroup(nGroup);
}

unsigned char CExoStreamingSoundSource::GetPriorityGroup()
{
    if (m_pInternal)
        return m_pInternal->m_nPriorityGroup;
    return 23;
}

void CExoStreamingSoundSource::Set3D(int b3D)
{
    if (m_pInternal)
        m_pInternal->m_b3D = b3D;
}

int CExoStreamingSoundSource::Get3D()
{
    if (m_pInternal)
        return m_pInternal->m_b3D;
    return 0;
}

void CExoStreamingSoundSource::SetDistance(float fMinDistance, float fMaxDistance)
{
    if (m_pInternal) {
        m_pInternal->m_fMinDistance = fMinDistance;
        m_pInternal->m_fMaxDistance = fMaxDistance;
    }
}

// Copies the position into a local first, so the stores cannot alias the source Vector.
void CExoStreamingSoundSource::SetPosition(const Vector& vPosition)
{
    if (m_pInternal) {
        Vector v = vPosition;
        m_pInternal->m_vPosition.x = v.x;
        m_pInternal->m_vPosition.y = v.y;
        m_pInternal->m_vPosition.z = v.z;
    }
}

void CExoStreamingSoundSource::SetReverbEnabled(int bEnabled)
{
    if (m_pInternal)
        m_pInternal->SetReverbEnabled(bEnabled);
}

// Always reports streaming as started.
int CExoStreamingSoundSource::GetHasStartedStreaming()
{
    return 1;
}

#if !KOTOR1  // K1 has no streaming-music-for-movie flag
void CExoStreamingSoundSource::SetStreamingMusicForMovie(int bForMovie)
{
    if (m_pInternal)
        m_pInternal->m_bStreamingMusicForMovie = bForMovie;
}

int CExoStreamingSoundSource::IsStreamingMusicForMovie()
{
    if (m_pInternal)
        return m_pInternal->m_bStreamingMusicForMovie;
    return 0;
}
#endif

extern int g_bDisableSound;
extern CExoSound* g_pExoSound;

CExoSoundSource::CExoSoundSource()
{
    if (g_bDisableSound) {
        m_pInternal = 0;
    } else {
        m_pInternal = new CExoSoundSourceInternal(g_pExoSound->m_pInternal);
    }
}

CExoSoundSource::CExoSoundSource(const CResRef& resRef)
{
    if (g_bDisableSound) {
        m_pInternal = 0;
    } else {
        m_pInternal = new CExoSoundSourceInternal(g_pExoSound->m_pInternal);
        m_pInternal->SetResRef(resRef, 0);
    }
}

CExoSoundSource::~CExoSoundSource()
{
    if (m_pInternal) {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

int CExoSoundSource::CancelRequest()
{
    if (m_pInternal && m_pInternal->m_pRes)
        return m_pInternal->m_pRes->CancelRequest();
    return 0;
}

void CExoSoundSource::SetPriorityGroup(unsigned char nGroup)
{
    if (m_pInternal)
        m_pInternal->SetPriorityGroup(nGroup);
}

int CExoSoundSource::Get3D()
{
    if (m_pInternal)
        return m_pInternal->m_b3D;
    return 0;
}

void CExoSoundSource::Set3D(int b3D)
{
    if (m_pInternal)
        m_pInternal->Set3D(b3D);
}

int CExoSoundSource::IsHardwarePlayingSoundPleaseDontUseThis()
{
    if (m_pInternal)
        return m_pInternal->IsPlaying();
    return 0;
}

int CExoSoundSource::Play()
{
    if (m_pInternal)
        return m_pInternal->Play(0, 0);
    return 0;
}

int CExoSoundSource::Request()
{
    if (m_pInternal && m_pInternal->m_pRes)
        return m_pInternal->m_pRes->Request();
    return 0;
}

void CExoSoundSource::SetVolume(unsigned char nVolume)
{
    if (m_pInternal) {
        nVolume = nVolume < 127 ? nVolume : 127;
        m_pInternal->SetVolume(nVolume, 0, 1.0f);
    }
}

void CExoSoundSource::SetPitchVariance(float fVariance)
{
    if (m_pInternal) {
        fVariance = fVariance >= 1.0f ? 1.0f : fVariance;
        if (fVariance < 0.0f) fVariance = 0.0f;
        m_pInternal->SetPitchVariance(fVariance);
    }
}

float CExoSoundSource::GetPitchVariance()
{
    if (m_pInternal)
        return m_pInternal->m_fPitchVariance;
    return 0.0f;
}

void CExoSoundSource::SetLooping(int bLooping)
{
    if (m_pInternal)
        m_pInternal->SetLooping(bLooping);
}

int CExoSoundSource::GetLooping()
{
    if (m_pInternal)
        return m_pInternal->m_bLooping;
    return 0;
}

void CExoSoundSource::SetPosition(const Vector& vPosition, float fZOffset)
{
    if (m_pInternal) {
        Vector v = vPosition;
        v.z = v.z + fZOffset;
        m_pInternal->SetPosition(v);
    }
}

Vector CExoSoundSource::GetPosition()
{
    if (m_pInternal)
        return m_pInternal->m_vPosition;
    return Vector();
}

void CExoSoundSource::SetDistance(float fMinDistance, float fMaxDistance)
{
    if (m_pInternal) {
        m_pInternal->m_fMaxDistance = fMaxDistance;
        m_pInternal->m_fMinDistance = fMinDistance;
    }
}

void CExoSoundSource::GetDistance(float* pfMinDistance, float* pfMaxDistance)
{
    if (m_pInternal) {
        *pfMinDistance = m_pInternal->m_fMinDistance;
        *pfMaxDistance = m_pInternal->m_fMaxDistance;
    }
}

void CExoSoundSource::SetResRef(const CResRef& resRef, int bRequest)
{
    if (m_pInternal) {
        m_pInternal->Stop();
        m_pInternal->m_nSoundLength = 0;
        m_pInternal->m_pSound->Render();
        m_pInternal->SetResRef(resRef, bRequest);
    }
}

void CExoSoundSource::Stop()
{
    if (m_pInternal)
        m_pInternal->Stop();
}

void CExoSoundSource::SetFixedVariance(float fVariance)
{
    if (m_pInternal)
        m_pInternal->SetFixedVariance(fVariance);
}

float CExoSoundSource::GetFixedVariance()
{
    if (m_pInternal)
        return m_pInternal->m_fFixedVariance;
    return 0.0f;
}
