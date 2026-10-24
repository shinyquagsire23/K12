#pragma once

#include "exostring.h"

// The script compiler facade. g_pScriptCompiler is the singleton; every member forwards to the
// CScriptCompilerInternal the constructor allocates.
class CScriptCompilerInternal;

class CScriptCompiler {
public:
    CScriptCompiler();
    ~CScriptCompiler();

    void SetCompileSymbolicOutput(int bEnable);
    void SetCompileDebugLevel(int bEnable);
    void SetIdentifierSpecification(const CExoString& sFile);
    void SetOutputAlias(const CExoString& sAlias);
    void SetOptimizeBinaryCodeLength(int bEnable);
    void SetCompileConditionalFile(int bEnable);
    int  CompileFile(const CExoString& sSource);
    int  CompileScriptChunk(const CExoString& sSource);
    int  CompileScriptConditional(const CExoString& sSource);
    int  GetCompiledScriptCode(char** ppCode, int* pnSize);
    void ClearCompiledScriptCode();

    CScriptCompilerInternal* m_pInternal;
};

// NWScript bytecode interpreter, a pimpl shell over CVirtualMachineInternal.
//
// The forwarders are unguarded tail calls, so each return type has to match its internal's: int
// everywhere except DeleteScriptSituation and SetCommandImplementer, which return void, with the
// Stack* family answering true or false.

#include "exostring.h"
#include "Vector.h"

class CResGFF;
struct CResStruct;
class CVirtualMachineInternal;
class CVirtualMachineCmdImplementer;

class CVirtualMachine {
public:
    CVirtualMachine();
    ~CVirtualMachine();

#if !KOTOR1  // K1's VM is the pimpl alone: no script parameters
    void SetScriptParams(int a, int b, int c, int d, int e, CExoString sParam);
#endif

    // Runs the named compiled script (.ncs) with the given game object as OBJECT_SELF.
    int RunScript(CExoString* pScriptName, unsigned long long objectId, int a);

    // Resumes a stored script situation (a suspended action closure) on the given object.
    int  RunScriptSituation(void* pSituation, unsigned long long objectId, int a);
    int  SaveScriptSituation(void* pSituation, CResGFF* pGff, CResStruct* pStruct);
    int  LoadScriptSituation(void** ppSituation, CResGFF* pGff, CResStruct* pStruct);
    void DeleteScriptSituation(void* pSituation);

    int  GetRunScriptReturnValue(int* pnValue, void** ppUnk);
    void SetCommandImplementer(CVirtualMachineCmdImplementer* p);

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
    int StackPopCommand(void** ppOut);

#if !KOTOR1
    int         m_nScriptParam[5];
    // Padding to the string array's alignment.
    CExoString  m_sScriptParam[1];  // a one-element array: the by-value CExoString argument
#endif
    CVirtualMachineInternal* m_pInternal;
};

extern CVirtualMachine* g_pVirtualMachine;


// The abstract callback interface the NWScript VM reaches its outside world through.
// CVirtualMachineInternal keeps one and calls it for everything the .ncs command set cannot do alone.
// CSWVirtualMachineCommands is the concrete implementation. The declaration order below is the
// vtable slot order the VM calls through, so it must not change.
class CExoString;
class CResGFF;
struct CResStruct;

class CVirtualMachineCmdImplementer {
public:
    virtual ~CVirtualMachineCmdImplementer() {}

    virtual void  InitializeCommands() = 0;
    virtual int   ExecuteCommand(int nCommand, int nArgCount) = 0;
    virtual void  RunScriptCallback(CExoString& sName) = 0;
    virtual void  ReportError(CExoString& sName, int nArg) = 0;
    virtual void* CreateGameDefinedStructure(int nType) = 0;
    virtual void  DestroyGameDefinedStructure(int nType, void* pStructure) = 0;
    virtual int   GetEqualGameDefinedStructure(int nType, void* pA, void* pB) = 0;
    virtual void* CopyGameDefinedStructure(int nType, void* pSource) = 0;
    virtual int   SaveGameDefinedStructure(int nType, void* pStructure, CResGFF* pGff,
                                           CResStruct* pStruct) = 0;
    virtual int   LoadGameDefinedStructure(int nType, void** ppStructure, CResGFF* pGff,
                                           CResStruct* pStruct) = 0;

    // Restamped per script frame from RunScript's third argument, alongside m_oidSelf.
    int m_nScriptRunArg;

    unsigned long long m_oidSelf;
};
