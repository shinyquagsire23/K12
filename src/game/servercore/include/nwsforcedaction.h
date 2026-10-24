#pragma once

#include "Vector.h"

class CResGFF;
struct CResStruct;

class CSWSForcedAction {
public:
    int                m_nActionId;
    unsigned short     m_nGroupId;
    unsigned long long m_nTargetArea;
    Vector             m_vPoint;
    int                _pad1c;
    unsigned long long m_nTargetObject;

    CSWSForcedAction();

    // GFF round-trip, called from CServerAIEventNode's forced-action arm.
    int SaveForcedAction(CResGFF* pGFF, CResStruct* pStruct);
    int LoadForcedAction(CResGFF* pGFF, CResStruct* pStruct);

    // Declared out of line so that deleting one stays a real call.
    ~CSWSForcedAction();
};
