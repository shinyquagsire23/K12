// Client/server-shared room base.

#include "swroom.h"

#include "exostring.h"

// The area loader writes "****" for rooms that carry no per-room data.
static const char* const NO_ROOM_NAME = "****";

CSWRoom::CSWRoom()
    : m_vPosition{0.0f, 0.0f, 0.0f},
      m_qOrientation{1.0f, 0.0f, 0.0f, 0.0f},
      m_nEnvAudioType(0),
      m_fEnvAudioValue(1.0f),
      m_bRoomNameIsWildcard(0)
{
}

CSWRoom::~CSWRoom()
{
}

void CSWRoom::SetRoomInfo(const CResRef& resName, const Vector& vPosition)
{
    m_bRoomNameIsWildcard = resName == const_cast<char*>(NO_ROOM_NAME);
    m_resRoom = resName;
    m_vPosition = vPosition;
}

// Copies the area's matching (type, value) env-audio pair for this room; wildcard rooms are skipped.
void CSWRoom::SetRoomEnvAudioProps(int nCount, CExoString* paNames, int* paTypes, float* paValues)
{
    if (m_bRoomNameIsWildcard)
        return;

    CExoString sRoomName;
    m_resRoom.CopyToString(sRoomName);

    int i;
    for (i = 0; i < nCount; i++) {
        if (sRoomName.CompareNoCase(paNames[i].CStr()))
            break;
    }

    if (i != nCount) {
        m_nEnvAudioType = paTypes[i];
        m_fEnvAudioValue = paValues[i];
    }
}
