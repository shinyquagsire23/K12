#include "nwsforcedaction.h"
#include "resgff.h"
#include "gameobjectarray.h"

CSWSForcedAction::CSWSForcedAction()
{
    m_vPoint.z = 0.0f;
    m_nActionId = 0;
    m_nGroupId = 0;
    m_nTargetArea = OBJECT_INVALID;
    m_vPoint.x = 0.0f;
    m_vPoint.y = 0.0f;
    m_nTargetObject = OBJECT_INVALID;
}

CSWSForcedAction::~CSWSForcedAction()
{
}

// Serializes the forced action. Target_Area and Target_Object are written before the Target_Loc
// floats, which is not the declaration order.
int CSWSForcedAction::SaveForcedAction(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_nActionId), const_cast<char*>("ActionId"));
    pGFF->WriteFieldWORD(pStruct, m_nGroupId, const_cast<char*>("GroupId"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_nTargetArea), const_cast<char*>("Target_Area"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_nTargetObject), const_cast<char*>("Target_Object"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPoint.x, const_cast<char*>("Target_LocX"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPoint.y, const_cast<char*>("Target_LocY"));
    pGFF->WriteFieldFLOAT(pStruct, m_vPoint.z, const_cast<char*>("Target_LocZ"));
    return 1;
}

// Reads the same seven fields in the same order as SaveForcedAction. The two id fields are 64-bit
// and zero-extended from the 32-bit value the reader returns.
int CSWSForcedAction::LoadForcedAction(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;

    m_nActionId = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("ActionId"), bSuccess, 0);
    m_nGroupId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("GroupId"), bSuccess, 0);
    m_nTargetArea = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Target_Area"), bSuccess, 0);
    m_nTargetObject = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Target_Object"), bSuccess, 0);
    m_vPoint.x = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_LocX"), bSuccess, 0.0f);
    m_vPoint.y = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_LocY"), bSuccess, 0.0f);
    m_vPoint.z = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("Target_LocZ"), bSuccess, 0.0f);
    return 1;
}
