#pragma once

// Client-side spell-cast visual/effect record.

#include "exoresman.h"

class CAurObject;
class CSWCCreature;

class CSWCSpellCastVisual {
public:
    CAurObject* m_pConjureHeadVisual;
    CAurObject* m_pConjureHandVisual;
    CAurObject* m_pConjureGroundVisual;
    CAurObject* m_pCastHeadVisual;
    CAurObject* m_pCastHandVisual;
    CAurObject* m_pCastGroundVisual;
    int   m_aUnloadDelay[2];
    int   m_bCastStarted;
    int   m_bConjureHeadDone;
    int   m_bConjureHandDone;
    int   m_bConjureGroundDone;

    CSWCSpellCastVisual();
    ~CSWCSpellCastVisual();
    void UnloadConjureVisual();
    void UnloadCastVisual();
    void UpdateTimer(unsigned int a);
    // r1..r3 are the three conjure objects; r4/r5 are the conjure sounds, played as 3D one-shots.
    // Returns true.
    bool LoadConjureVisual(CResRef r1, CResRef r2, CResRef r3, CResRef r4, CResRef r5,
                           CSWCCreature* pCreature);
    // The cast-time twin of LoadConjureVisual, with three ResRefs instead of five.
    bool LoadCastVisual(CResRef r1, CResRef r2, CResRef r3, CSWCCreature* pCreature);
    // Registered on the head conjure object's "donefade" callback: marks whichever conjure object
    // finished so UpdateTimer detaches and frees it.
    void SetConjureDone(CAurObject* pObject);
};
