#include "k2stdint.h"
#include "restype.h"

// The loudest a sound object plays.
enum {
    SOUND_VOLUME_MAX = 127,
};
#include "nwssoundobject.h"
#include "CExoArrayListImpl.h"

CSWSSoundObject::CSWSSoundObject(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_SOUND, objectId, 0) {
    m_active = 1;
    m_positional = 1;
    m_looping = 0;
    m_volume = SOUND_VOLUME_MAX;
    m_volumeVrtn = 0;
    m_times = 3;
    m_pitchVariation = 0.0f;
    m_hours = 0;
    m_fixedVariance = 1.0f;
    m_generatedType = 0;
    m_randomPosition = 0;
    m_randomRangeX = 0.0f;
    m_randomRangeY = 0.0f;
    m_interval = 0;
    m_intervalVrtn = 0;
    m_minDistance = 10.0f;
    m_maxDistance = 20.0f;
    m_continuous = 0;
    m_random = 0;
}

CSWSSoundObject::~CSWSSoundObject() {
}

CSWSSoundObject* CSWSSoundObject::AsSWSSoundObject() {
    return this;
}

void CSWSSoundObject::AIUpdate() {
}

void CSWSSoundObject::EventHandler(unsigned int eventType, unsigned long long a2, void* a3,
                                     unsigned int a4, unsigned int a5) {
                                         K2_UNUSED(eventType); K2_UNUSED(a2); K2_UNUSED(a3); K2_UNUSED(a4); K2_UNUSED(a5);
}

CSWSObjectArrayStub CSWSSoundObject::GetPeopleInSoundRange() {
    return {};
}

void CSWSSoundObject::RemoveFromArea() {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    }
    SetArea(0);
}

void CSWSSoundObject::AddToArea(CSWSArea* pArea, int a2) {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    }
    SetArea(0);
    if (pArea != 0) {
        SetArea(pArea);
        pArea->AddObjectToArea(m_objectId, a2);
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        pModule->AddObjectToLookupTable(m_sTag, m_objectId);
    }
}

void* CSWSSoundObject::GetClientSoundObject() {
    CAppManager** const ppMgr = &g_pAppManager;
    unsigned long long clientId = (*ppMgr)->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    return (*ppMgr)->m_pClientExoApp->GetSoundObjectByGameObjectID(clientId);
}

void CSWSSoundObject::FadeAndStop(unsigned int fadeTime) {
    m_active = 0;
    CAppManager** const ppMgr = &g_pAppManager;
    unsigned long long clientId = (*ppMgr)->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWCSoundObject* pClientObj =
        static_cast<CSWCSoundObject*>((*ppMgr)->m_pClientExoApp->GetSoundObjectByGameObjectID(clientId));
    if (pClientObj != 0) {
        pClientObj->FadeAndStop(fadeTime);
    }
}

void CSWSSoundObject::ChangePitchVariation(float variation) {
    if (__builtin_expect(m_pitchVariation == variation, 0)) return;
    m_pitchVariation = variation;
    CAppManager** const ppMgr = &g_pAppManager;
    unsigned long long clientId = (*ppMgr)->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWCSoundObject* pClientObj =
        static_cast<CSWCSoundObject*>((*ppMgr)->m_pClientExoApp->GetSoundObjectByGameObjectID(clientId));
    if (pClientObj != 0) {
        pClientObj->ChangePitchVariation(variation);
    }
}

void CSWSSoundObject::ChangeVolume(int volume) {
    int v = volume < SOUND_VOLUME_MAX ? volume : SOUND_VOLUME_MAX;
    if (v < 0) v = 0;
    if (v == m_volume) return;
    m_volume = v;
    CAppManager** const ppMgr = &g_pAppManager;
    unsigned long long clientId = (*ppMgr)->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWCSoundObject* pClientObj =
        static_cast<CSWCSoundObject*>((*ppMgr)->m_pClientExoApp->GetSoundObjectByGameObjectID(clientId));
    if (pClientObj != 0) {
        pClientObj->ChangeVolume(v);
    }
}

void CSWSSoundObject::ChangeFixedVariance(float variance) {
    if (__builtin_expect(m_fixedVariance == variance, 0)) return;
    m_fixedVariance = variance;
    CAppManager** const ppMgr = &g_pAppManager;
    unsigned long long clientId = (*ppMgr)->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWCSoundObject* pClientObj =
        static_cast<CSWCSoundObject*>((*ppMgr)->m_pClientExoApp->GetSoundObjectByGameObjectID(clientId));
    if (pClientObj != 0) {
        pClientObj->ChangeFixedVariance(variance);
    }
}

// Serializes the replicated state into a network message.
void CSWSSoundObject::PackIntoMessage(CSWSMessage* pMsg) {
    float px = m_position.x;
    float py = m_position.y;
    float pz = m_position.z;
    pMsg->WriteOBJECTIDServer(m_objectId);
    pMsg->WriteBOOL(m_active);
    pMsg->WriteBOOL(m_positional);
    pMsg->WriteBOOL(m_looping);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_volume), 8);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_volumeVrtn), 8);
    pMsg->WriteBYTE(static_cast<unsigned char>(m_times), 8);
    pMsg->WriteFLOAT(m_pitchVariation, 1.0f, 32);
    pMsg->WriteFLOAT(m_fixedVariance, 1.0f, 32);
    pMsg->WriteDWORD(m_hours, 32);
    pMsg->WriteBYTE(m_generatedType, 8);
    pMsg->WriteDWORD(static_cast<unsigned int>(m_interval), 32);
    pMsg->WriteDWORD(static_cast<unsigned int>(m_intervalVrtn), 32);
    pMsg->WriteFLOAT(m_minDistance, 1.0f, 32);
    pMsg->WriteFLOAT(m_maxDistance, 1.0f, 32);
    pMsg->WriteBOOL(m_continuous);
    pMsg->WriteBOOL(m_random);
    pMsg->WriteBOOL(m_randomPosition);
    pMsg->WriteFLOAT(m_randomRangeX, 1.0f, 32);
    pMsg->WriteFLOAT(m_randomRangeY, 1.0f, 32);
    pMsg->WriteFLOAT(px, 1.0f, 32);
    pMsg->WriteFLOAT(py, 1.0f, 32);
    pMsg->WriteFLOAT(pz, 1.0f, 32);
    unsigned short count = static_cast<unsigned short>(m_sounds.m_count);
    pMsg->WriteWORD(count, 16);
    for (unsigned short i = 0; i < count; i++) {
        pMsg->WriteCResRef(m_sounds.m_pArray[i], 16);
    }
}

// Moves the object and broadcasts the new position to nearby players.
void CSWSSoundObject::ChangePosition(Vector newPos) {
    if (equal(newPos, m_position)) return;
    SetPosition(newPos, 1, 1, 0);
    CAppManager** const ppMgr = &g_pAppManager;
    CSWSMessage* pMsg = (*ppMgr)->m_pServerExoApp->GetSWSMessage();
    CExoLinkedList<CSWSPlayerListEntry>* pList = (*ppMgr)->m_pServerExoApp->GetPlayerList();
    if (pMsg != 0 && pList != 0) {
        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        if (pNode != 0) {
            do {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
                if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
                    CGameObject* pGameObj = pPlayer->GetGameObject();
                    if (pGameObj->AsSWSCreature() != 0) {
                        pMsg->SendServerToPlayerSoundObject_ChangePosition(pPlayer, m_objectId, newPos);
                    }
                }
                pList->m_pInternal->GetNext(pNode);
            } while (pNode != 0);
        }
    }
}

// Starts the sound on every player that can hear it.
void CSWSSoundObject::Play() {
    m_active = 1;
    CAppManager** const ppMgr = &g_pAppManager;
    CSWSMessage* pMsg = (*ppMgr)->m_pServerExoApp->GetSWSMessage();
    CExoLinkedList<CSWSPlayerListEntry>* pList = (*ppMgr)->m_pServerExoApp->GetPlayerList();
    if (pMsg != 0 && pList != 0) {
        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        if (pNode != 0) {
            do {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
                if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
                    CGameObject* pGameObj = pPlayer->GetGameObject();
                    if (pGameObj->AsSWSCreature() != 0) {
                        pMsg->SendServerToPlayerSoundObject_Play(pPlayer, m_objectId);
                    }
                }
                pList->m_pInternal->GetNext(pNode);
            } while (pNode != 0);
        }
    }
}

// Stops the sound on every player that can hear it.
void CSWSSoundObject::Stop() {
    m_active = 0;
    CAppManager** const ppMgr = &g_pAppManager;
    CSWSMessage* pMsg = (*ppMgr)->m_pServerExoApp->GetSWSMessage();
    CExoLinkedList<CSWSPlayerListEntry>* pList = (*ppMgr)->m_pServerExoApp->GetPlayerList();
    if (pMsg != 0 && pList != 0) {
        CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
        if (pNode != 0) {
            do {
                CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
                if (pPlayer != 0 && pPlayer->GetGameObject() != 0) {
                    CGameObject* pGameObj = pPlayer->GetGameObject();
                    if (pGameObj->AsSWSCreature() != 0) {
                        pMsg->SendServerToPlayerSoundObject_Stop(pPlayer, m_objectId);
                    }
                }
                pList->m_pInternal->GetNext(pNode);
            } while (pNode != 0);
        }
    }
}

// Serializes the state to a GFF struct, with the sound list as a "Sounds" sublist.
void CSWSSoundObject::Save(CResGFF* pGFF, CResStruct* pStruct) {
    float px = m_position.x;
    float py = m_position.y;
    float pz = m_position.z;
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_active), const_cast<char*>("Active"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_positional), const_cast<char*>("Positional"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_looping), const_cast<char*>("Looping"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_volume), const_cast<char*>("Volume"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_volumeVrtn), const_cast<char*>("VolumeVrtn"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_times), const_cast<char*>("Times"));
    pGFF->WriteFieldFLOAT(pStruct, m_pitchVariation, const_cast<char*>("PitchVariation"));
    pGFF->WriteFieldDWORD(pStruct, m_hours, const_cast<char*>("Hours"));
    pGFF->WriteFieldDWORD(pStruct, m_generatedType, const_cast<char*>("GeneratedType"));
    pGFF->WriteFieldDWORD(pStruct, m_interval, const_cast<char*>("Interval"));
    pGFF->WriteFieldDWORD(pStruct, m_intervalVrtn, const_cast<char*>("IntervalVrtn"));
    pGFF->WriteFieldFLOAT(pStruct, m_minDistance, const_cast<char*>("MinDistance"));
    pGFF->WriteFieldFLOAT(pStruct, m_maxDistance, const_cast<char*>("MaxDistance"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_continuous), const_cast<char*>("Continuous"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_random), const_cast<char*>("Random"));
    pGFF->WriteFieldFLOAT(pStruct, m_fixedVariance, const_cast<char*>("FixedVariance"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_randomPosition), const_cast<char*>("RandomPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_randomRangeX, const_cast<char*>("RandomRangeX"));
    pGFF->WriteFieldFLOAT(pStruct, m_randomRangeY, const_cast<char*>("RandomRangeY"));
    pGFF->WriteFieldFLOAT(pStruct, px, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, py, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, pz, const_cast<char*>("ZPosition"));
    CResList soundsList;
    pGFF->AddList(&soundsList, pStruct, const_cast<char*>("Sounds"));
    for (int64_t i = 0; i < m_sounds.m_count; i++) {
        CResStruct soundStruct;
        pGFF->AddListElement(&soundStruct, &soundsList, 0);
        pGFF->WriteFieldCResRef(&soundStruct, m_sounds.m_pArray[i], const_cast<char*>("Sound"));
    }
}

// Reads the GFF fields Save writes, keeping the current value for any field the file omits.
// Returns the last read's success flag.
int CSWSSoundObject::Load(CResGFF* pGFF, CResStruct* pStruct) {
    Vector pos = {};
    CResList soundsList;
    int bSuccess;
    CResRef rr;

    m_sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, m_sTag).LowerCase();
    m_active         = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Active"), bSuccess, static_cast<unsigned char>(m_active));
    m_positional     = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Positional"), bSuccess, static_cast<unsigned char>(m_positional));
    m_looping        = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Looping"), bSuccess, static_cast<unsigned char>(m_looping));
    m_volume         = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Volume"), bSuccess, static_cast<unsigned char>(m_volume));
    m_volumeVrtn     = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("VolumeVrtn"), bSuccess, static_cast<unsigned char>(m_volumeVrtn));
    m_times          = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Times"), bSuccess, static_cast<unsigned char>(m_times));
    m_pitchVariation = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PitchVariation"), bSuccess, m_pitchVariation);
    m_hours          = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Hours"), bSuccess, m_hours);
    m_generatedType  = static_cast<unsigned char>(pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("GeneratedType"), bSuccess, m_generatedType));
    m_interval       = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Interval"), bSuccess, m_interval);
    m_intervalVrtn   = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("IntervalVrtn"), bSuccess, m_intervalVrtn);
    m_minDistance    = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("MinDistance"), bSuccess, m_minDistance);
    m_maxDistance    = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("MaxDistance"), bSuccess, m_maxDistance);
    m_continuous     = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Continuous"), bSuccess, static_cast<unsigned char>(m_continuous));
    m_random         = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Random"), bSuccess, static_cast<unsigned char>(m_random));
    m_fixedVariance  = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("FixedVariance"), bSuccess, m_fixedVariance);
    m_randomPosition = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("RandomPosition"), bSuccess, static_cast<unsigned char>(m_randomPosition));
    m_randomRangeX   = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("RandomRangeX"), bSuccess, m_randomRangeX);
    m_randomRangeY   = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("RandomRangeY"), bSuccess, m_randomRangeY);
    pos.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XPosition"), bSuccess, 0.0f);
    pos.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YPosition"), bSuccess, 0.0f);
    pos.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
    SetPosition(pos, 1, 1, 0);

    if (pGFF->GetList(&soundsList, pStruct, const_cast<char*>("Sounds"))) {
        int count = pGFF->GetListCount(&soundsList);
        for (int i = 0; i < count; i++) {
            CResStruct soundStruct;
            if (pGFF->GetListElement(&soundStruct, &soundsList, i)) {
                rr = pGFF->ReadFieldCResRef(&soundStruct, const_cast<char*>("Sound"), bSuccess, CResRef(""));
                if (bSuccess)
                    m_sounds.Add(rr);
            }
        }
    }
    return bSuccess;
}

int CSWSSoundObject::LoadFromTemplate(const CResRef& resRef) {
    CResStruct topStruct;
    CResGFF* pGff = new CResGFF(RESTYPE_UTS, const_cast<char*>("UTS "), resRef);
    int result;
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        result = Load(pGff, &topStruct);
        delete pGff;
    } else {
        delete pGff;
        result = 0;
    }
    return result;
}
