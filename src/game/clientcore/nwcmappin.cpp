#include "nwcmappin.h"
#include "serveranimation.h"


CSWCMapPin::CSWCMapPin()
    : CSWCObject(CGameObject::OBJECT_TYPE_WAYPOINT)
{
    m_bActive = 0;
    m_currentAnimation = SERVER_ANIMATION_READY;
    m_nPinType = 0;
}

CSWCMapPin::~CSWCMapPin()
{
}

CSWCMapPin& CSWCMapPin::operator=(CSWCMapPin* pOther)
{
    Vector v = pOther->m_position;
    SetPosition(v);
    v = pOther->m_orientation;
    SetOrientation(v);
    m_bActive = pOther->m_bActive;
    m_sName = pOther->m_sName;
    return *this;
}

// Client-side pin update and load are unimplemented; the four overrides do nothing.
void CSWCMapPin::AIUpdate()
{
}

int CSWCMapPin::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(r); K2_UNUSED(a); K2_UNUSED(b);
    return 0;
}

void CSWCMapPin::UnloadModel()
{
}

void CSWCMapPin::UnloadModel(unsigned char a)
{
    K2_UNUSED(a);
}

