// Ambient sound attached to a room: the sound's resref plus the streaming source that plays it.
#ifndef CSWBEHAVIORATTACHSOUND_H
#define CSWBEHAVIORATTACHSOUND_H

#include "AURORA/aurbehavior.h"
#include "exoresman.h"

class CAurObject;
class CAurPart;
class CExoStreamingSoundSource;

class CSWBehaviorAttachSound : public CAurBehaviorAttach {
public:

    virtual ~CSWBehaviorAttachSound();
    virtual int Control(float fParam);

    // Non-virtual: CSWCArea::LoadArea constructs one per room's ambient sound.
    CSWBehaviorAttachSound(CAurObject* pDummy, CAurObject* pRoom, CAurPart* pPart,
                           const CResRef& resSound);
#if KOTOR1  // K1 was built without the Windows A-suffix rename
    void PlaySound();
    void PlaySoundA() { PlaySound(); }
#else
    void PlaySoundA();
#endif

    CResRef                   m_resSound;
    CExoStreamingSoundSource* m_pSoundSource;
};

K2_LAYOUT_ASSERT(sizeof(CSWBehaviorAttachSound) == 0x80, "CSWBehaviorAttachSound size drift");

#endif // CSWBEHAVIORATTACHSOUND_H
