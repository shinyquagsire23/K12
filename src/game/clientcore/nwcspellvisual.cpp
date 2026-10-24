#include "k2stdint.h"
#include <stddef.h>
#include "nwcspellvisual.h"
#include "AURORA/aurmodel.h"
#include "nwccreature.h"

#include "exostring.h"
#include "exosound.h"
#include "Quaternion.h"
#include "soundprioritygroup.h"

extern CExoSound* g_pExoSound;
#include "port/libcstring.h"

#include "port/libcbase.h"

#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWCSpellCastVisual::CSWCSpellCastVisual()
{
    // The whole object is zeroed as one unit.
    __builtin_memset(static_cast<void*>(this), 0, sizeof(CSWCSpellCastVisual));
}

CSWCSpellCastVisual::~CSWCSpellCastVisual()
{
    UnloadConjureVisual();
    UnloadCastVisual();
}

// Detaches and deletes one owned CAurObject. The idiom appears three times per unload body, so it is
// written out rather than factored.

void CSWCSpellCastVisual::UnloadConjureVisual()
{
    if (m_pConjureHeadVisual) {
        m_pConjureHeadVisual->AttachToObject(0, 0, 0);
        delete m_pConjureHeadVisual;
        m_pConjureHeadVisual = 0;
    }
    if (m_pConjureHandVisual) {
        m_pConjureHandVisual->AttachToObject(0, 0, 0);
        delete m_pConjureHandVisual;
        m_pConjureHandVisual = 0;
    }
    if (m_pConjureGroundVisual) {
        m_pConjureGroundVisual->AttachToObject(0, 0, 0);
        delete m_pConjureGroundVisual;
        m_pConjureGroundVisual = 0;
    }
}

void CSWCSpellCastVisual::UnloadCastVisual()
{
    if (m_pCastHeadVisual) {
        m_pCastHeadVisual->AttachToObject(0, 0, 0);
        delete m_pCastHeadVisual;
        m_pCastHeadVisual = 0;
    }
    if (m_pCastHandVisual) {
        m_pCastHandVisual->AttachToObject(0, 0, 0);
        delete m_pCastHandVisual;
        m_pCastHandVisual = 0;
    }
    if (m_pCastGroundVisual) {
        m_pCastGroundVisual->AttachToObject(0, 0, 0);
        delete m_pCastGroundVisual;
        m_pCastGroundVisual = 0;
    }
}

void CSWCSpellCastVisual::UpdateTimer(unsigned int nTicks)
{
    // Each timer has its own gate flag: when set, the matching object is detached and freed now;
    // otherwise nothing happens and the pair is re-created by the load functions.
    if (m_bConjureHeadDone && m_pConjureHeadVisual) {
        m_pConjureHeadVisual->AttachToObject(0, 0, 0);
        delete m_pConjureHeadVisual;
        m_pConjureHeadVisual = 0;
    }
    if (m_bConjureHandDone && m_pConjureHandVisual) {
        m_pConjureHandVisual->AttachToObject(0, 0, 0);
        delete m_pConjureHandVisual;
        m_pConjureHandVisual = 0;
    }
    if (m_bConjureGroundDone && m_pConjureGroundVisual) {
        m_pConjureGroundVisual->AttachToObject(0, 0, 0);
        delete m_pConjureGroundVisual;
        m_pConjureGroundVisual = 0;
    }

    if (m_bCastStarted) {
        if (m_aUnloadDelay[0]) {
            if (static_cast<unsigned int>(m_aUnloadDelay[0]) <= nTicks) {
                UnloadConjureVisual();
                m_aUnloadDelay[0] = 0;
            } else {
                m_aUnloadDelay[0] -= nTicks;
            }
        } else if (m_aUnloadDelay[1] == 0) {
            m_bCastStarted = 0;
        } else if (static_cast<unsigned int>(m_aUnloadDelay[1]) <= nTicks) {
            UnloadCastVisual();
            m_aUnloadDelay[1] = 0;
            m_bCastStarted = 0;
        } else {
            m_aUnloadDelay[1] -= nTicks;
        }
    }
}

void CSWCSpellCastVisual::SetConjureDone(CAurObject* pObject)
{
    // The three conjure objects finish independently. The registered donefade callback passes the
    // object it finished, and only the matching flag is stamped.
    if (m_pConjureHeadVisual == pObject)
        m_bConjureHeadDone = 1;
    if (m_pConjureHandVisual == pObject)
        m_bConjureHandDone = 1;
    if (m_pConjureGroundVisual == pObject)
        m_bConjureGroundDone = 1;
}

bool CSWCSpellCastVisual::LoadCastVisual(CResRef r1, CResRef r2, CResRef r3,
                                         CSWCCreature* pCreature)
{
    char szName[17];

    UnloadCastVisual();

    // Each arm takes a NUL-terminated name from the CResRef, skips it when empty, else instances
    // the model, hooks it to the creature's matching body part and starts the cast animation on it.
    r1.CopyToString(szName);
    if (__strlen_chk(szName, sizeof(szName)) != 0) {
        m_pCastHeadVisual = NewCAurObject(szName, const_cast<char*>("castvisual"), 0, 0);
        if (m_pCastHeadVisual) {
            m_pCastHeadVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                    const_cast<char*>("headconjure"), 0);
            m_pCastHeadVisual->PlayAnimation(const_cast<char*>("cast01"), 1.0f, 1, 0.0f);
        }
    }

    r2.CopyToString(szName);
    if (__strlen_chk(szName, sizeof(szName)) != 0) {
        m_pCastHandVisual = NewCAurObject(szName, const_cast<char*>("castvisual"), 0, 0);
        if (m_pCastHandVisual) {
            m_pCastHandVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                    const_cast<char*>("handconjure"), 0);
            m_pCastHandVisual->PlayAnimation(const_cast<char*>("cast01"), 1.0f, 1, 0.0f);
        }
    }

    r3.CopyToString(szName);
    if (__strlen_chk(szName, sizeof(szName)) != 0) {
        m_pCastGroundVisual = NewCAurObject(szName, const_cast<char*>("castgroundvisual"), 0, 0);
        if (m_pCastGroundVisual) {
            m_pCastGroundVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)), const_cast<char*>("root"), 0);
            m_pCastGroundVisual->PlayAnimation(const_cast<char*>("cast01"), 1.0f, 1, 0.0f);
        }
    }

    // Starts the fade-out on the conjure set this cast replaces (m_pConjureHandVisual is the hand, m_pConjureHeadVisual the head).
    if (m_pConjureHandVisual)
        m_pConjureHandVisual->PlayAnimation(const_cast<char*>("fade"), 1.0f, 1, 0.0f);
    if (m_pConjureHeadVisual)
        m_pConjureHeadVisual->PlayAnimation(const_cast<char*>("fade"), 1.0f, 1, 0.0f);
    if (m_pConjureGroundVisual)
        m_pConjureGroundVisual->PlayAnimation(const_cast<char*>("fade"), 1.0f, 1, 0.0f);

    return true;
}

// The donefade callback the conjure loader registers on each conjure object. The engine hands it
// (object, name, userdata) and the userdata is the visual record itself.
static void ConjureDoneCallback(CAurObject* pObject, char* pszName, void* pUserData)
{
    K2_UNUSED(pszName);
    if (!pUserData)
        return;
    static_cast<CSWCSpellCastVisual*>(pUserData)->SetConjureDone(pObject);
}

bool CSWCSpellCastVisual::LoadConjureVisual(CResRef r1, CResRef r2, CResRef r3, CResRef r4,
                                            CResRef r5, CSWCCreature* pCreature)
{
    CExoString sName;

    UnloadConjureVisual();

    // Each arm instances the conjure model under its body part, clears the matching done flag and
    // arms the effectively unfired 9999-second donefade callback on it.
    r1.CopyToString(sName);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sName.GetLength() > 0) {
#else
    if (sName.m_sString != 0 && static_cast<int>(strlen(sName.m_sString)) > 0) {
#endif
        m_pConjureHeadVisual = NewCAurObject(sName.CStr(), const_cast<char*>("conjurevisual"), 0, 0);
        if (m_pConjureHeadVisual) {
            m_bConjureHeadDone = 0;
            m_pConjureHeadVisual->RegisterCallBack(ConjureDoneCallback, const_cast<char*>("donefade"), this, 9999.0f, false);
            m_pConjureHeadVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                   const_cast<char*>("headconjure"), 0);
            m_pConjureHeadVisual->PlayAnimation(const_cast<char*>("conjure01"), 1.0f, 0, 0.0f);
        }
    }

    r2.CopyToString(sName);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sName.GetLength() > 0) {
#else
    if (sName.m_sString != 0 && static_cast<int>(strlen(sName.m_sString)) > 0) {
#endif
        m_pConjureHandVisual = NewCAurObject(sName.CStr(), const_cast<char*>("conjurevisual"), 0, 0);
        if (m_pConjureHandVisual) {
            m_bConjureHandDone = 0;
            m_pConjureHandVisual->RegisterCallBack(ConjureDoneCallback, const_cast<char*>("donefade"), this, 9999.0f, false);
            m_pConjureHandVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                   const_cast<char*>("handconjure"), 0);
            m_pConjureHandVisual->PlayAnimation(const_cast<char*>("conjure01"), 1.0f, 0, 0.0f);
        }
    }

    r3.CopyToString(sName);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sName.GetLength() > 0) {
#else
    if (sName.m_sString != 0 && static_cast<int>(strlen(sName.m_sString)) > 0) {
#endif
        m_pConjureGroundVisual = NewCAurObject(sName.CStr(), const_cast<char*>("conjuregroundvisual"), 0, 0);
        if (m_pConjureGroundVisual) {
            m_bConjureGroundDone = 0;
            m_pConjureGroundVisual->RegisterCallBack(ConjureDoneCallback, const_cast<char*>("donefade"), this, 9999.0f, false);
            m_pConjureGroundVisual->AttachToObject(static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)), const_cast<char*>("root"), 0);
            m_pConjureGroundVisual->PlayAnimation(const_cast<char*>("conjure01"), 1.0f, 0, 0.0f);
        }
    }

    // Both conjure sounds play at the hand's position, falling back to the creature's own world
    // position when the model has no handhook yet.
    Quaternion qOrientation;
    Vector vPos;
    CAurObject* pModel = static_cast<CAurObject*>(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1));
    if (!pModel || !pModel->GetPartPosition(const_cast<char*>("handconjure"), &vPos, &qOrientation))
        vPos = pCreature->m_position;

    if (r4.GetResRef()[0] != 0)
        g_pExoSound->Play3DOneShotSound(r4, vPos, 1.5f, PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
    if (r5.GetResRef()[0] != 0)
        g_pExoSound->Play3DOneShotSound(r5, vPos, 1.5f, PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);

    return true;
}
