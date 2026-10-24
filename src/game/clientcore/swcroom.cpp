#include "swcroom.h"

#include "aurscene.h"
#include "exostring.h"
#include "AURORA/aurmodel.h"

CSWCRoom::CSWCRoom()
    : m_pAurRoom(0),
      m_pRoomModel(0),
      m_nSceneRoomIndex(-1)
{
}

CAurRoom* CSWCRoom::GetAurRoom()
{
    return m_pAurRoom;
}

CSWCRoom::~CSWCRoom()
{
}

// Spawns the room's model into the scene, keeping both handles: the room model and the
// renderer room.
Gob* CSWCRoom::SpawnRoom(CAurScene* pScene)
{
    if (m_bRoomNameIsWildcard)
        return 0;

    CExoString sRoomName(m_resRoom.GetResRefStr());

    m_pRoomModel = pScene->SpawnRoom(static_cast<char*>(sRoomName.CStr()), m_vPosition, m_qOrientation);
    if (m_pRoomModel == 0)
        return 0;

    m_pAurRoom = pScene->FindRoom(static_cast<char*>(sRoomName.CStr()));
    return m_pRoomModel;
}

Gob* CSWCRoom::SpawnRoom(CAurScene* pScene, const CResRef& resRoom, Vector& vPosition)
{
    if (m_bRoomNameIsWildcard)
        return 0;

    if (resRoom.IsValid())
        m_resRoom = resRoom;

    if (vPosition.x != 0.0f || vPosition.y != 0.0f || vPosition.z != 0.0f)
        m_vPosition = vPosition;

    m_pRoomModel = pScene->SpawnRoom(m_resRoom.GetResRefStr(), m_vPosition, m_qOrientation);
    if (m_pRoomModel == 0)
        return 0;

    m_pAurRoom = pScene->FindRoom(m_resRoom.GetResRefStr());
    return m_pRoomModel;
}

// Loop names are "scriptloopN", zero-padded below 10.
void CSWCRoom::PlayAnimation(unsigned short nAnim)
{
    if (m_pRoomModel != 0) {
        CExoString sAnimation;
        if (nAnim <= 9)
            sAnimation.Format(const_cast<char*>("scriptloop0%d"), nAnim);
        else
            sAnimation.Format(const_cast<char*>("scriptloop%d"), nAnim);

#if !KOTOR1
        m_pRoomModel->PlayAnimation(const_cast<char*>("null"), 1.0f, 8, 0.0f);
#endif
        m_pRoomModel->PlayAnimation(static_cast<char*>(sAnimation.CStr()), 1.0f, 8, 0.0f);
    }
}
