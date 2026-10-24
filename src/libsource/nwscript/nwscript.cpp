// CVirtualMachine, the NWScript VM's public surface, and CScriptCompiler's facade bodies.

#include "nwscript.h"
#include "nwvmachineinternal.h"
#include "nwscriptinternal.h"

#include "port/libcstring.h"

void GarbageCollectGameDefinedStructureAll();

// Seconds of forced input lockout, consumed by the input layer.
extern float g_inputDisabledTime;
#if KOTOR1
extern float IosDisableInitiateDialogTime;
#endif

CVirtualMachine::CVirtualMachine()
{
    m_pInternal = new CVirtualMachineInternal();
}

CVirtualMachine::~CVirtualMachine()
{
    delete m_pInternal;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CVirtualMachine::SetScriptParams(int a, int b, int c, int d, int e, CExoString sParam)
{
    m_nScriptParam[0] = a;
    m_nScriptParam[1] = b;
    m_nScriptParam[2] = c;
    m_nScriptParam[3] = d;
    m_nScriptParam[4] = e;
    m_sScriptParam[0] = sParam;
}
#endif // !KOTOR1

// Runs the structure garbage-collection pass, then arms the input lockout when the script name
// contains "a_createhk47", the HK-47 assembly cutscene.
int CVirtualMachine::RunScript(CExoString* pScriptName, unsigned long long objectId, int a)
{
#if KOTOR1  // K1 has no structure GC; one Taris script holds off tap-to-talk for 3.3 seconds
    if (*pScriptName == "k_ptar_vulcow_ud")
        IosDisableInitiateDialogTime = 3.3f;
    return m_pInternal->RunScript(pScriptName, objectId, a);
#else
    GarbageCollectGameDefinedStructureAll();
    if (pScriptName != 0 && pScriptName->CStr() != 0 &&
        strstr(pScriptName->CStr(), "a_createhk47") != 0)
        g_inputDisabledTime = 1.0f;
    return m_pInternal->RunScript(pScriptName, objectId, a);
#endif
}

int CVirtualMachine::RunScriptSituation(void* pSituation, unsigned long long objectId, int a)
{
    return m_pInternal->RunScriptSituation(static_cast<CVirtualMachineScript*>(pSituation), objectId, a);
}

int CVirtualMachine::GetRunScriptReturnValue(int* pnValue, void** ppUnk)
{
    return m_pInternal->GetRunScriptReturnValue(pnValue, ppUnk);
}

void CVirtualMachine::SetCommandImplementer(CVirtualMachineCmdImplementer* p)
{
    m_pInternal->SetCommandImplementer(p);
}

int CVirtualMachine::StackPopInteger(int* pnOut)
{
    return m_pInternal->StackPopInteger(pnOut);
}

int CVirtualMachine::StackPushInteger(int nValue)
{
    return m_pInternal->StackPushInteger(nValue);
}

int CVirtualMachine::StackPopFloat(float* pfOut)
{
    return m_pInternal->StackPopFloat(pfOut);
}

int CVirtualMachine::StackPushFloat(float fValue)
{
    return m_pInternal->StackPushFloat(fValue);
}

int CVirtualMachine::StackPopVector(Vector* pvOut)
{
    return m_pInternal->StackPopVector(pvOut);
}

int CVirtualMachine::StackPushVector(Vector vValue)
{
    return m_pInternal->StackPushVector(vValue);
}

int CVirtualMachine::StackPopString(CExoString* psOut)
{
    return m_pInternal->StackPopString(psOut);
}

int CVirtualMachine::StackPushString(const CExoString& sValue)
{
    return m_pInternal->StackPushString(sValue);
}

int CVirtualMachine::StackPopEngineStructure(int nType, void** ppOut)
{
    return m_pInternal->StackPopEngineStructure(nType, ppOut);
}

int CVirtualMachine::StackPushEngineStructure(int nType, void* pValue)
{
    return m_pInternal->StackPushEngineStructure(nType, pValue);
}

int CVirtualMachine::StackPopObject(unsigned long long* poidOut)
{
    return m_pInternal->StackPopObject(poidOut);
}

int CVirtualMachine::StackPushObject(unsigned long long oid)
{
    return m_pInternal->StackPushObject(oid);
}

int CVirtualMachine::StackPopCommand(void** ppOut)
{
    return m_pInternal->StackPopCommand(reinterpret_cast<CVirtualMachineScript**>(ppOut));
}

int CVirtualMachine::SaveScriptSituation(void* pSituation, CResGFF* pGff, CResStruct* pStruct)
{
    return m_pInternal->SaveScriptSituation(static_cast<CVirtualMachineScript*>(pSituation), pGff, pStruct);
}

int CVirtualMachine::LoadScriptSituation(void** ppSituation, CResGFF* pGff, CResStruct* pStruct)
{
    return m_pInternal->LoadScriptSituation(reinterpret_cast<CVirtualMachineScript**>(ppSituation), pGff, pStruct);
}

void CVirtualMachine::DeleteScriptSituation(void* pSituation)
{
    m_pInternal->DeleteScriptSituation(static_cast<CVirtualMachineScript*>(pSituation));
}

CScriptCompiler::CScriptCompiler()
{
    m_pInternal = new CScriptCompilerInternal;
}

CScriptCompiler::~CScriptCompiler()
{
    delete m_pInternal;
}

void CScriptCompiler::SetCompileSymbolicOutput(int bEnable)
{
    m_pInternal->SetCompileSymbolicOutput(bEnable);
}

void CScriptCompiler::SetCompileDebugLevel(int bEnable)
{
    m_pInternal->SetCompileDebugLevel(bEnable);
}

void CScriptCompiler::SetIdentifierSpecification(const CExoString& sFile)
{
    m_pInternal->SetIdentifierSpecification(sFile);
}

void CScriptCompiler::SetOutputAlias(const CExoString& sAlias)
{
    m_pInternal->SetOutputAlias(sAlias);
}

void CScriptCompiler::SetOptimizeBinaryCodeLength(int bEnable)
{
    m_pInternal->SetOptimizeBinaryCodeLength(bEnable);
}

void CScriptCompiler::SetCompileConditionalFile(int bEnable)
{
    m_pInternal->SetCompileConditionalFile(bEnable);
}

int CScriptCompiler::CompileFile(const CExoString& sSource)
{
    return m_pInternal->CompileFile(sSource);
}

int CScriptCompiler::CompileScriptChunk(const CExoString& sSource)
{
    return m_pInternal->CompileScriptChunk(sSource);
}

int CScriptCompiler::CompileScriptConditional(const CExoString& sSource)
{
    return m_pInternal->CompileScriptConditional(sSource);
}

int CScriptCompiler::GetCompiledScriptCode(char** ppCode, int* pnSize)
{
    return m_pInternal->GetCompiledScriptCode(ppCode, pnSize);
}

void CScriptCompiler::ClearCompiledScriptCode()
{
    m_pInternal->ClearCompiledScriptCode();
}
