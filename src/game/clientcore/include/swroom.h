#ifndef CSWROOM_H
#define CSWROOM_H

#include "Quaternion.h"
#include "Vector.h"
#include "exoresman.h"

class CExoString;

// Shared base of the client and server room objects: the room's identity plus its env-audio
// settings.
class CSWRoom {
public:
    CSWRoom();

    // <VTABLE-GEN: binary ground truth (_ZTV) — hand-pinned; the class has only the dtor pair */
    //   primary vtable, 2 slots
    virtual ~CSWRoom();  // the key function; body is empty
    // </VTABLE-GEN>

    void SetRoomInfo(const CResRef& resName, const Vector& vPosition);
    void SetRoomName(const CResRef& resName) { m_resRoom = resName; }
    void SetRoomEnvAudioProps(int nCount, CExoString* paNames, int* paTypes,
                              float* paValues);

    Vector      m_vPosition;
    Quaternion  m_qOrientation;
    CResRef     m_resRoom;
    int         m_nEnvAudioType;
    float       m_fEnvAudioValue;
    // Set when the room is named "****", the area loader's no-name placeholder; such a room has no
    // entry in the area's env-audio table.
    int         m_bRoomNameIsWildcard;
};

#endif // CSWROOM_H
