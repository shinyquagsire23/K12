#pragma once
#include "k2stdint.h"

#ifndef CVIRTUALMACHINESTACK_H
#define CVIRTUALMACHINESTACK_H

// One NWScript operand stack. The VM owns one by value; each saved script situation owns a
// heap-allocated one and frees it with ClearStack + delete. Both pointers are entry counts, 64-bit
// and unsigned; m_nCapacity is a capacity in entries, which LoadStack also uses to size both cell
// arrays. m_pTypeData holds one type-tag byte per entry, m_pValueData one value per entry, and
// m_pOwner is the back-pointer ClearStack uses to reach the owner's command implementer for the
// engine-structure cleanup hook.

class CExoString;
class CResGFF;
struct CResStruct;
class CVirtualMachineInternal;

class CVirtualMachineStack {
public:
    CVirtualMachineStack();
    ~CVirtualMachineStack();

    void ClearStack();
    void CopyFromStack(CVirtualMachineStack* pFrom, int nTopBound, int nBaseBound);
    int  GetBasePointer();
    void SetBasePointer(int nBase);
    int  GetStackPointer();
    void SetStackPointer(int nSp);
    void InitializeStack();
    void AddToTopOfStack(int nType);
    void ModifyIntegerAtLocation(int nLocation, int nValue);
    void AssignLocationToLocation(int nDst, int nSrc);
    int  LoadStack(CResGFF* pGff, CResStruct* pStruct);
    int  SaveStack(CResGFF* pGff, CResStruct* pStruct);

    uint64_t m_nStackPointer;
    uint64_t m_nBasePointer;
    int       m_nCapacity;
    char*     m_pTypeData;
    void**    m_pValueData;
    CVirtualMachineInternal* m_pOwner;
};

#endif // CVIRTUALMACHINESTACK_H

#ifndef CVIRTUALMACHINESCRIPT_H
#define CVIRTUALMACHINESCRIPT_H

// One NWScript script situation: a compiled .ncs image plus the operand stack running it. The VM
// holds eight by value. InitializeScript fills one from a byte buffer, DeleteScript resets one, and
// SetUpScriptSituation moves one into the next free slot.

#include "exostring.h"

class CVirtualMachineScript {
public:
    CVirtualMachineScript()
    {
        m_sName = "";
        m_sData = 0;
        m_nDataSize = 0;
        m_nSituationIndex = 0;
        m_nCRC = 0;
        m_pStack = 0;
        m_nStackSize = 0;
        m_nInstructionPtr = 0;
        m_nSecondaryPtr = 0;
    }

    ~CVirtualMachineScript()
    {
        CVirtualMachineStack* pStack = m_pStack;
        if (pStack != 0)
            delete pStack;
        m_pStack = 0;
        if (m_sData != 0) {
            delete m_sData;
            m_sData = 0;
        }
    }

    CVirtualMachineStack* m_pStack;
    int       m_nStackSize;
    int       m_nInstructionPtr;
    int       m_nSecondaryPtr;
    char*     m_sData;
    int       m_nDataSize;
    CExoString m_sName;
    int       m_nSituationIndex;
    int       m_nCRC;
};

#endif // CVIRTUALMACHINESCRIPT_H

#ifndef CVIRTUALMACHINEINTERNAL_H
#define CVIRTUALMACHINEINTERNAL_H

// The NWScript VM engine. CVirtualMachine, its public pimpl shell, owns exactly one of these and
// forwards most of its methods here. The situation arrays are indexed by m_nSituationCount: one
// object id and one script parameter per slot, matching the second and third argument of
// RunScript/RunScriptSituation. m_sStack is the stack the embedding VM runs on, and
// m_pCmdImplementer receives the commands the interpreter executes.

#include "CResHelper.h"
#include "resncs.h"
#include "nwscript.h"

class CExoString;
class CResGFF;
struct CResStruct;
struct Vector;

class CVirtualMachineInternal : public CResHelper<CResNCS, 2010> {
public:

    virtual ~CVirtualMachineInternal();

    CVirtualMachineInternal();

    // The NWScript status, 1 or 0, for every method but DeleteScriptSituation and
    // SetCommandImplementer (void). CVirtualMachine's forwarders tail-call these, so the return types
    // are load-bearing.
    int  RunScript(CExoString* pScriptName, unsigned long long objectId, int nParam);
    int  RunScriptSituation(CVirtualMachineScript* pSituation, unsigned long long objectId, int nParam);
    int  ReadScriptFile(CExoString* pScriptName);
    int  RunScriptFile(int nArg);
    // The bytecode interpreter. Runs the image named by the current situation slot's compiled data,
    // updating *pnOffset as it goes; every negative return is a distinct VM error status.
    int  ExecuteCode(int* pnOffset, char* pCode, int nSize);
    int  SaveScriptSituation(CVirtualMachineScript* pSituation, CResGFF* pGff, CResStruct* pStruct);
    int  LoadScriptSituation(CVirtualMachineScript** ppSituation, CResGFF* pGff, CResStruct* pStruct);
    void DeleteScriptSituation(CVirtualMachineScript* pSituation);

    int  GetRunScriptReturnValue(int* pnValue, void** ppUnk);
    void SetCommandImplementer(CVirtualMachineCmdImplementer* p);

    int  DeleteScript(CVirtualMachineScript* pScript);
    int  PushInstructionPtr(int nValue);
    int  PopInstructionPtr(int* pnOut);
    void InitializeScript(CVirtualMachineScript* pScript, char* pData, int nSize);
    int  SetUpScriptSituation(CVirtualMachineScript* pSituation);

    int StackPopInteger(int* pnOut);
    int StackPushInteger(int nValue);
    int StackPopFloat(float* pfOut);
    int StackPushFloat(float fValue);
    int StackPopVector(Vector* pvOut);
    int StackPushVector(Vector vValue);
    int StackPopString(CExoString* psOut);
    int StackPushString(const CExoString& sValue);
    int StackPopEngineStructure(int nType, void** ppOut);
    int StackPushEngineStructure(int nType, void* pValue);
    int StackPopObject(unsigned long long* poidOut);
    int StackPushObject(unsigned long long oid);
    int StackPopCommand(CVirtualMachineScript** ppOut);

    int   m_nReturnValueType;
    void* m_pReturnValue;
    int   m_nInstructionsExecuted;
    int   m_nSituationCount;
    CVirtualMachineScript m_aScriptSituations[8];
    int m_anScriptParam[8];
    unsigned long long m_anScriptOid[8];
    CVirtualMachineStack m_sStack;
    int   m_nInstructionPtr;
    int   m_anInstructionPtrs[128];
    int   m_nSavedInstructionPtr;
    int   m_nStackSizeToSave;
    int   m_nBaseStackSizeToSave;
    CVirtualMachineCmdImplementer* m_pCmdImplementer;
};

#endif // CVIRTUALMACHINEINTERNAL_H
