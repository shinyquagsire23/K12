#include "nwswaypoint.h"
#include "serveranimation.h"
#include "restype.h"

CSWSWaypoint::CSWSWaypoint(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_WAYPOINT, objectId, 0) {
    m_nAnimation = SERVER_ANIMATION_READY;
    m_bHasMapNote = 0;
    m_bMapNoteEnabled = 0;
}

CSWSWaypoint::~CSWSWaypoint() {
    if (GetArea() != 0) {
        RemoveFromArea();
    }
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    }
}

CSWSWaypoint* CSWSWaypoint::AsSWSWaypoint() {
    return this;
}

CExoLocString& CSWSWaypoint::GetFirstName() {
    return m_firstName;
}

void CSWSWaypoint::AIUpdate() {
}

void CSWSWaypoint::RemoveFromArea() {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    }
    SetArea(0);
    Vector zero = {};
    SetPosition(zero, 1, 1, 0);
}

void CSWSWaypoint::EventHandler(unsigned int eventType, unsigned long long a2, void* a3,
                                  unsigned int a4, unsigned int a5) {
    K2_UNUSED(a2); K2_UNUSED(a3); K2_UNUSED(a4); K2_UNUSED(a5);
    if (eventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT) && m_bDestroyable != 0 && reinterpret_cast<uintptr_t>(this) != 0) {
        delete this;
    }
}

void CSWSWaypoint::AddToArea(CSWSArea* pArea, float x, float y, float z, int a5) {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    }
    SetArea(0);
    {
        Vector zero = {};
        SetPosition(zero, 1, 1, 0);
    }
    if (pArea != 0) {
        CExoString tag;
        SetArea(pArea);
        Vector pos = {x, y, z};
        SetPosition(pos, 0, 1, 0);
        static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, a5);
    }
}

int CSWSWaypoint::LoadFromTemplate(const CResRef& resRef) {
    CResStruct topStruct;
    CResGFF* pGff = new CResGFF(RESTYPE_UTW, const_cast<char*>("UTW "), resRef);
    int result;
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        LoadWaypoint(pGff, &topStruct);
        delete pGff;
        result = 1;
    } else {
        delete pGff;
        CExoString tag1;
        CExoString tag2;
        resRef.CopyToString(tag2);
        tag1.Format(const_cast<char*>("Waypoint template %s doesn't exist.\n"), tag2.CStr());
        result = 0;
    }
    return result;
}

int CSWSWaypoint::SaveWaypoint(CResGFF* pGFF, CResStruct* pStruct) {
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCExoLocString(pStruct, m_firstName, const_cast<char*>("LocalizedName"));
    Vector pos = m_position;
    pGFF->WriteFieldFLOAT(pStruct, pos.x, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, pos.y, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, pos.z, const_cast<char*>("ZPosition"));
    Vector ori = m_orientation;
    pGFF->WriteFieldFLOAT(pStruct, ori.x, const_cast<char*>("XOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, ori.y, const_cast<char*>("YOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, ori.z, const_cast<char*>("ZOrientation"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bHasMapNote), const_cast<char*>("HasMapNote"));
    if (m_bHasMapNote) {
        pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bMapNoteEnabled), const_cast<char*>("MapNoteEnabled"));
        pGFF->WriteFieldCExoLocString(pStruct, m_mapNote, const_cast<char*>("MapNote"));
    }
    return 1;
}

// "LocalizedName" reads into its own unused out-parameter, kept separate from `found`.
int CSWSWaypoint::LoadWaypoint(CResGFF* pGFF, CResStruct* pStruct) {
    int found;
    int foundName;
    Vector position = {};
    Vector orientation = {};
    CExoLocString mapNoteText;
    unsigned char hasMapNote;
    unsigned char mapNoteEnabled;
    CSWSModule* pModule;

    SetTag(pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), found, CExoString("")));
    m_firstName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocalizedName"), foundName, CExoLocString());
    position.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XPosition"), found, 0.0f);
    position.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YPosition"), found, 0.0f);
    position.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZPosition"), found, 0.0f);
    orientation.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("XOrientation"), found, 0.0f);
    orientation.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("YOrientation"), found, 0.0f);
    orientation.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ZOrientation"), found, 0.0f);
    if (orientation.x * orientation.x + orientation.y * orientation.y + orientation.z * orientation.z != 1.0f) {
        static_cast<Vector*>(&orientation)->Normalize();
    }
    SetPosition(position, 1, 1, 0);
    SetOrientation(orientation);

    hasMapNote = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("HasMapNote"), found, 0);
    if (hasMapNote != 0 && found != 0) {
        mapNoteEnabled = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MapNoteEnabled"), found, 0);
        mapNoteText = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("MapNote"), found, CExoLocString());
        if (found != 0) {
            m_bHasMapNote = hasMapNote;
            m_bMapNoteEnabled = mapNoteEnabled;
            m_mapNote = mapNoteText;
        }
    }

    pModule = g_pAppManager->m_pServerExoApp->GetModule();
    pModule->AddObjectToLookupTable(m_sTag, m_objectId);
    return 1;
}

