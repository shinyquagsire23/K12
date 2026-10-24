#include "k2stdint.h"
#include <stddef.h>
#include "appmanager.h"
#include "clientexoapp.h"
#include "exosound.h"
#include "resgff.h"
#include "swmgobject.h"
#include "swmgtrackfollower.h"
#include "nwscript.h"
#include "soundprioritygroup.h"

extern CExoSound* g_pExoSound;

#include "port/libcstring.h"
#include "port/crtapi.h"

CSWMiniGameObjectArray::CSWMiniGameObjectArray()
{
    memset(m_apObjects, 0, sizeof(m_apObjects));
    m_nNextId = 0;
}

CSWMiniGameObject* CSWMiniGameObjectArray::GetMiniGameObject(unsigned int nId, unsigned char nType)
{
    CSWMiniGameObject* pObject;
    if (nId < OBJECT_ID_NONE && (pObject = m_apObjects[nId]) != NULL)
    {
        switch (nType)
        {
        case PROBE_ANY:
            return pObject;
        case PROBE_PLAYER:
            return pObject->AsPlayer();
        case PROBE_ENEMY:
            return pObject->AsEnemy();
        case PROBE_OBSTACLE:
            return pObject->AsObstacle();
        case PROBE_FOLLOWER:
            return pObject->AsFollower();
        case PROBE_TRIGGER_ENEMY:
            {
                // CSWMiniEnemy is modelled flat in its header.
                CSWMiniEnemy* pEnemy = static_cast<CSWMiniEnemy*>(pObject->AsEnemy());
                if (pEnemy != NULL && pEnemy->m_bTrigger)
                    return static_cast<CSWMiniGameObject*>(pEnemy);
            }
        }
    }
    return NULL;
}

CSWMiniGameObject* CSWMiniGameObjectArray::GetMiniGameObjectByName(const CExoString& sName)
{
    for (int i = 0; i < OBJECT_ID_NONE; i++)
    {
        if (m_apObjects[i] != NULL)
        {
#if KOTOR1  // K1: a case-sensitive CExoString compare
            if (sName == m_apObjects[i]->GetName())
                return m_apObjects[i];
#else
            if (_stricmp(sName.CStr(), m_apObjects[i]->GetName().CStr()) == 0)
                return m_apObjects[i];
#endif
        }
    }
    return NULL;
}

// Walks the id table from the cursor; the top id doubles as the wrap point and as the no-free-slot
// sentinel.
unsigned int CSWMiniGameObjectArray::GetNextID()
{
    unsigned int nStart = m_nNextId;
    unsigned int nId = nStart;
    while (m_apObjects[nId] != NULL)
    {
        nId++;
        if (nId == OBJECT_ID_NONE)
            nId = 0;
        m_nNextId = nId;
        if (nId == nStart)
            return OBJECT_ID_NONE;
    }
    return nId;
}

CSWMiniGameObject::CSWMiniGameObject(unsigned int nObjectType)
{
    m_nObjectId = static_cast<CSWMiniGameObjectArray*>(g_pAppManager->m_pClientExoApp->GetMGOArray())->GetNextID();
    static_cast<CSWMiniGameObjectArray*>(g_pAppManager->m_pClientExoApp->GetMGOArray())->m_apObjects[m_nObjectId] = this;
    m_nObjectType = nObjectType;
    m_bPendingDelete = 0;
    m_spObject = NULL;
    m_nLastBulletHitShooter = CSWMiniGameObjectArray::OBJECT_ID_NONE;
    m_nLastFollowerHit = CSWMiniGameObjectArray::OBJECT_ID_NONE;
}


CSWMiniGameObject::~CSWMiniGameObject()
{
    static_cast<CSWMiniGameObjectArray*>(g_pAppManager->m_pClientExoApp->GetMGOArray())->m_apObjects[m_nObjectId] = NULL;
}

// Runs the heartbeat, script slot 4 ("OnHeartbeat"), if the object has one.
void CSWMiniGameObject::Update(float)
{
    CResRef resScript = GetScriptName(SCRIPT_ON_HEARTBEAT);
    if (resScript.IsValid())
    {
        CExoString sScript;
        resScript.CopyToString(sScript);
        g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
    }
}

// The follower whose bullet last hit this object, or NULL (with the field reset to the none
// sentinel) when the recorded id is stale or names a non-follower.
CSWTrackFollower* CSWMiniGameObject::GetLastBulletHitShooter()
{
    CSWMiniGameObjectArray* pObjects = static_cast<CSWMiniGameObjectArray*>(g_pAppManager->m_pClientExoApp->GetMGOArray());
    if (m_nLastBulletHitShooter < CSWMiniGameObjectArray::OBJECT_ID_NONE)
    {
        CSWMiniGameObject* pShooter = pObjects->m_apObjects[m_nLastBulletHitShooter];
        if (pShooter != NULL)
        {
            CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(pShooter->AsFollower());
            if (pFollower != NULL)
                return pFollower;
        }
    }
    m_nLastBulletHitShooter = CSWMiniGameObjectArray::OBJECT_ID_NONE;
    return NULL;
}

CSWTrackFollower* CSWMiniGameObject::GetLastFollowerHit()
{
    CSWMiniGameObjectArray* pObjects = static_cast<CSWMiniGameObjectArray*>(g_pAppManager->m_pClientExoApp->GetMGOArray());
    if (m_nLastFollowerHit < CSWMiniGameObjectArray::OBJECT_ID_NONE)
    {
        CSWMiniGameObject* pHit = pObjects->m_apObjects[m_nLastFollowerHit];
        if (pHit != NULL)
        {
            CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(pHit->AsFollower());
            if (pFollower != NULL)
                return pFollower;
        }
    }
    m_nLastFollowerHit = CSWMiniGameObjectArray::OBJECT_ID_NONE;
    return NULL;
}

// nShooterId == 0 is the script-notification form (script slot 1, "OnHitBullet"); nonzero, or id 0
// with no script, is damage resolution: if this object tracks a follower whose TargetType the bullet
// accepts, the follower takes the bullet's damage and the hit sound plays at the object's position
// (on the player channel when this object is the player).
void CSWMiniGameObject::OnHitBullet(int nShooterId)
{
    if (nShooterId == 0)
    {
        CResRef resScript = GetScriptName(SCRIPT_ON_HIT_BULLET);
        if (resScript.IsValid())
        {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
            return;
        }
        // An invalid script resref falls into the damage-and-sound path below; there is no else.
    }

    CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(AsFollower());
    if (pFollower != NULL)
    {
        int nTargetType = m_bullet.m_nTargetType;
        if ((pFollower->TargetType() & nTargetType) != 0)
            pFollower->AdjustHitPoints(-m_bullet.m_nDamage);
    }

    if (m_bullet.m_resModel.IsValid())
    {
        unsigned char nChannel = (AsPlayer() == NULL) ? PRIORITY_GROUP_MINIGAME_ENEMY : PRIORITY_GROUP_DEFAULT;
        CExoSound* pExoSound = g_pExoSound;
        CResRef resSound(m_bullet.m_resModel.GetResRef());
        pExoSound->Play3DOneShotSound(resSound, m_vPosition, 0.0f, nChannel, 0, 0, 0.0f, 0.0f);
    }
}

void CSWMiniGameObject::OnHitFollower(int nFollowerId)
{
    if (nFollowerId == 0)
    {
        CResRef resScript = GetScriptName(SCRIPT_ON_HIT_FOLLOWER);
        if (resScript.IsValid())
        {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
        }
    }
}

// Followers get created through a script: slot 0 ("OnCreate") runs once at spawn.
void CSWMiniGameObject::OnCreate()
{
    CResRef resScript;
    if (AsFollower() != NULL)
    {
        resScript = GetScriptName(SCRIPT_ON_CREATE);
        if (resScript.IsValid())
        {
            CExoString sScript;
            resScript.CopyToString(sScript);
            g_pVirtualMachine->RunScript(&sScript, m_nObjectId, 1);
        }
    }
}

// Loads the five script slots, in slot order. The CResStruct* argument is unused, because the
// loader reads everything through the CResStruct reference.
void CSWMiniGameObject::LoadScripts(CResGFF* pGFF, CResStruct* pStruct, CResStruct& rStruct)
{
    K2_UNUSED(pStruct);
    int nFieldIndex;
    SetScriptName(SCRIPT_ON_CREATE, pGFF->ReadFieldCResRef(&rStruct, const_cast<char*>("OnCreate"), nFieldIndex, CResRef("")));
    SetScriptName(SCRIPT_ON_HIT_BULLET, pGFF->ReadFieldCResRef(&rStruct, const_cast<char*>("OnHitBullet"), nFieldIndex, CResRef("")));
    SetScriptName(SCRIPT_ON_HIT_FOLLOWER, pGFF->ReadFieldCResRef(&rStruct, const_cast<char*>("OnHitFollower"), nFieldIndex, CResRef("")));
    SetScriptName(SCRIPT_ON_ANIM_EVENT, pGFF->ReadFieldCResRef(&rStruct, const_cast<char*>("OnAnimEvent"), nFieldIndex, CResRef("")));
    SetScriptName(SCRIPT_ON_HEARTBEAT, pGFF->ReadFieldCResRef(&rStruct, const_cast<char*>("OnHeartbeat"), nFieldIndex, CResRef("")));
}
