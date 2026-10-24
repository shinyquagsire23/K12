#include "AURORA/aurbehavior.h"
#include "swbehaviorattachsound.h"
#include "exosound.h"
#include <stdarg.h>
#include "port/libcbase.h"
#include "soundprioritygroup.h"

CSWBehaviorAttachSound::CSWBehaviorAttachSound(CAurObject* pDummy, CAurObject* pRoom,
                                               CAurPart* pPart, const CResRef& resSound)
    : CAurBehaviorAttach(pDummy, pRoom, pPart)
{
    m_resSound = resSound;

    m_pSoundSource = new CExoStreamingSoundSource(0);
    m_pSoundSource->SetLooping(1);
    m_pSoundSource->Set3D(1);
    m_pSoundSource->SetPriorityGroup(PRIORITY_GROUP_ROOM_SOUNDS);
    m_pSoundSource->SetDistance(20.0f, 150.0f);
}

CSWBehaviorAttachSound::~CSWBehaviorAttachSound()
{
    if (m_pSoundSource != 0)
    {
        m_pSoundSource->Stop(0);
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }
}

int CSWBehaviorAttachSound::Control(float f)
{
    int nResult = CAurBehaviorAttach::Control(f);
    Vector vPosition = m_pOwner->GetPosition();
    m_pSoundSource->SetPosition(vPosition);
    return nResult;
}

#if KOTOR1
void CSWBehaviorAttachSound::PlaySound()
#else
void CSWBehaviorAttachSound::PlaySoundA()
#endif
{
    if (m_pSoundSource->IsPlaying())
        return;
    m_pSoundSource->Play(m_resSound, 4, 0, 0, 0);
}
