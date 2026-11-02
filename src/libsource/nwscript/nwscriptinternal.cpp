// CScriptCompilerInternal, the NWScript compiler core: construction, the compile entry points and
// the source-script handle they load through, and error output.

#include "k2stdint.h"
#include "nwscriptinternal.h"

#include "exolocstringinternal.h"

#include "exofile.h"
#include "exoresman.h"
#include "gameobjectarray.h"
#include "restype.h"

#include "port/libcstring.h"

#include <stdio.h>
#include "exostringinline.h"

// CompileScriptChunk's and CompileScriptConditional's wrappers, "void main(){%s}" and
// "int main(){ return(%s);}", without the "%s".
enum { CHUNK_WRAPPER_LENGTH = 13 };
enum { CONDITIONAL_WRAPPER_LENGTH = 22 };

CScriptCompilerIdListEntry::CScriptCompilerIdListEntry()
{
    m_bImplementationInPlace = 0;
    m_nParameterSpace = 0;
    m_nIntegerData = 0;
    m_fFloatData = 0.0f;
    m_fVectorData[0] = 0.0f;
    m_fVectorData[1] = 0.0f;
    m_fVectorData[2] = 0.0f;
    m_nIdentifierType = IDENTIFIER_TYPE_CONSTANT;
    m_nReturnType = 0;
    m_nParameterCount = 0;
    m_nNonOptionalParameterCount = 0;
    m_pParameterTypes = 0;
    m_pParameterStructureNames = 0;
    m_pOptionalParameterFlags = 0;
    m_pOptionalParameterIntegerData = 0;
    m_pOptionalParameterFloatData = 0;
    m_pOptionalParameterStringData = 0;
    m_pOptionalParameterObjectData = 0;
    m_pOptionalParameterVectorData = 0;
    m_nCodeOffset = -1;
    m_nCodeEnd = -1;
    m_nOutputStart = -1;
    m_nOutputEnd = -1;
}

CScriptCompilerIdListEntry::~CScriptCompilerIdListEntry()
{
    delete[] m_pParameterTypes;
    delete[] m_pParameterStructureNames;
    delete[] m_pOptionalParameterFlags;
    delete[] m_pOptionalParameterIntegerData;
    delete[] m_pOptionalParameterFloatData;
    delete[] m_pOptionalParameterStringData;
    delete[] m_pOptionalParameterObjectData;
    delete[] m_pOptionalParameterVectorData;
}

int CScriptCompilerIdListEntry::ExpandParameterSpace()
{
    if (m_nParameterSpace == MAX_PARAMETER_COUNT)
        return -SCRIPT_ERROR_TOO_MANY_PARAMETERS;

    int nNewCapacity = m_nParameterSpace == 0 ? 4 : m_nParameterSpace * 2;

    unsigned char* paNew68 = new unsigned char[nNewCapacity];
    CExoString* paNew70 = new CExoString[nNewCapacity];
    int* paNew78 = new int[nNewCapacity];
    int* paNew80 = new int[nNewCapacity];
    float* paNew88 = new float[nNewCapacity];
    CExoString* paNew90 = new CExoString[nNewCapacity];
    unsigned long long* paNew98 = new unsigned long long[nNewCapacity];
    float* paNewa0 = new float[nNewCapacity * 3];

    if (m_nParameterSpace != 0) {
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew68[i] = m_pParameterTypes[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew70[i] = m_pParameterStructureNames[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew78[i] = m_pOptionalParameterFlags[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew80[i] = m_pOptionalParameterIntegerData[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew88[i] = m_pOptionalParameterFloatData[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew90[i] = m_pOptionalParameterStringData[i];
        for (int i = 0; i < m_nParameterSpace; i++)
            paNew98[i] = m_pOptionalParameterObjectData[i];
        for (int i = 0; i < m_nParameterSpace * 3; i++)
            paNewa0[i] = m_pOptionalParameterVectorData[i];
    }

    for (int i = m_nParameterSpace; i < nNewCapacity; i++) {
        paNew68[i] = 0;
        paNew70[i] = "";
        paNew78[i] = 0;
        paNew80[i] = 0;
        paNew88[i] = 0;
        paNew90[i] = "";
        paNew98[i] = OBJECT_INVALID;
    }

    if (m_nParameterSpace * 3 < nNewCapacity * 3)
        __builtin_memset(&paNewa0[m_nParameterSpace * 3], 0,
                         static_cast<unsigned int>(nNewCapacity * 3 - 1 - m_nParameterSpace * 3) * sizeof(int) + sizeof(int));

    m_nParameterSpace = nNewCapacity;
    delete[] m_pParameterTypes;
    delete[] m_pParameterStructureNames;
    delete[] m_pOptionalParameterFlags;
    delete[] m_pOptionalParameterIntegerData;
    delete[] m_pOptionalParameterFloatData;
    delete[] m_pOptionalParameterStringData;
    delete[] m_pOptionalParameterObjectData;
    delete[] m_pOptionalParameterVectorData;
    m_pParameterTypes = paNew68;
    m_pParameterStructureNames = paNew70;
    m_pOptionalParameterFlags = paNew78;
    m_pOptionalParameterIntegerData = paNew80;
    m_pOptionalParameterFloatData = paNew88;
    m_pOptionalParameterStringData = paNew90;
    m_pOptionalParameterObjectData = paNew98;
    m_pOptionalParameterVectorData = paNewa0;
    return 0;
}

CSourceScript::CSourceScript()
{
}

int CSourceScript::LoadScript(const CExoString& sName, char** ppSourceText, unsigned int* pnSize)
{
    CResRef resRef(sName);
    SetResRef(resRef, 0);

    if (m_pRes->Demand() == 0)
        return -SCRIPT_ERROR_FILE_NOT_FOUND;

    *ppSourceText = static_cast<char*>(m_pRes->GetNSSDataPtr());
    *pnSize = m_pRes->GetNSSSize();
    return 0;
}

void CSourceScript::UnloadScript()
{
    m_pRes->Release();
}

CScriptCompilerInternal::CScriptCompilerInternal()
{
    m_bCompileDebugLevel = 0;
    m_bCompileSymbolicOutput = 0;
    m_sSourceFile = "";
    m_sOutputAlias = "OVERRIDE";
    m_bOptimizeBinaryCodeLength = 1;
    m_nIdentifierListState = IDLIST_IDLE;
    m_pSubroutineStack = 0;
    m_pIdentifierList = 0;
    m_pVarStackList = 0;
    m_pSymbolQueryList = 0;
    m_pSymbolLabelList = 0;
    m_nIncludeFileDepth = 0;
    m_bCompileConditionalFile = 0;
    m_nEngineStructureCount = 0;
    m_nIdentifierListEngineStructure = 0;
    m_pStructureList = 0;
    m_pStructureFieldList = 0;
    m_pEngineStructureValid = 0;
    m_pEngineStructureNames = 0;
    Initialize();
}

void CScriptCompilerInternal::Initialize()
{
    m_nLine = 1;
    m_nColumn = 1;
    m_nSubroutineStackCapacity = 1024;
    m_nSubroutineStackSize = -1;

    if (m_pSubroutineStack == 0)
        m_pSubroutineStack = new CScriptSRStackEntry[1024];

    PushSRStack(GRAMMAR_PROGRAM, 0, 0, 0);

    if (m_pStructureList == 0)
        m_pStructureList = new CScriptCompilerTable720Entry[256];
    if (m_pStructureFieldList == 0)
        m_pStructureFieldList = new CScriptCompilerTable728Entry[4096];

    m_nSwitchLabelCapacity = SWITCH_LABEL_COUNT;
    m_nStructureDefinition = 0;
    m_nGlobalVariableSize = 0;
    m_nVarStackRecursionLevel = 0;
    m_pSwitchLabels = 0;
    m_bGlobalVariableDefinition = 0;
    m_nGlobalVariableCount = 0;
    m_bSwitchHasDefault = 0;
    m_nSwitchLabelCount = 0;
    m_nLoopIdentifier = 0;
    m_nSwitchLevel = 0;
    m_nSwitchIdentifier = 0;
    m_nSwitchStackDepth = 0;
    m_nStructureCount = 1;
    m_nFieldCount = 0;

    m_pStructureList[0].m_nSize = 12;
    m_pStructureList[0].m_nFirstField = 0;
    m_pStructureList[0].m_nLastField = 2;
    m_pStructureList[0].m_sName = "vector";
    m_nFieldCount = 3;

    m_pStructureFieldList[0].m_sName = "x";
    m_pStructureFieldList[0].m_nOffset = 0;
    m_pStructureFieldList[0].m_nType = KEYWORD_FLOAT;
    m_pStructureFieldList[1].m_sName = "y";
    m_pStructureFieldList[1].m_nOffset = 4;
    m_pStructureFieldList[1].m_nType = KEYWORD_FLOAT;
    m_pStructureFieldList[2].m_sName = "z";
    m_pStructureFieldList[2].m_nOffset = 8;
    m_pStructureFieldList[2].m_nType = KEYWORD_FLOAT;

    m_nCompileMode = COMPILE_MODE_PARSE_TREE;
    m_bCompileIdentifierConstants = 0;

    if (m_pVarStackList == 0)
        m_pVarStackList = new CScriptCompilerTable700Entry[1024];

    m_nStackCurrentDepth = 0;
    m_nBinaryCodeOffset = 0;
    m_bAssignmentToVariable = 0;
    m_bInStructurePart = 0;
    m_nVarStackTop = -1;
    m_nVarStackVariableType = DECLARATION_TYPE_NONE;
    m_bConditionalCompile = 0;

    ClearAllSymbolLists();

    m_bInFunctionImplementation = 0;
    m_sFunctionImpName = "";
    m_nFunctionImpReturnType = 0;
    m_sFunctionImpReturnStructureName = "";
    m_nFunctionImpAbortStackPointer = 0;
    m_pGlobalVariableParseTree = 0;
    m_pCompiledCode = 0;

    TokenInitialize();
}

CScriptCompilerInternal::~CScriptCompilerInternal()
{
    ShutDown();
}

void CScriptCompilerInternal::ShutDown() throw()
{
    delete[] m_pSubroutineStack;
    delete[] m_pIdentifierList;
    delete[] m_pVarStackList;
    if (m_pStructureList) {
        delete[] m_pStructureList;
        m_pStructureList = 0;
    }
    if (m_pStructureFieldList) {
        delete[] m_pStructureFieldList;
        m_pStructureFieldList = 0;
    }
    if (m_pEngineStructureNames) {
        delete[] m_pEngineStructureNames;
        m_pEngineStructureNames = 0;
    }
    if (m_pEngineStructureValid) {
        delete[] m_pEngineStructureValid;
        m_pEngineStructureValid = 0;
    }
}

void CScriptCompilerInternal::InitializePreDefinedStructures()
{
    m_nStructureCount = 1;
    m_pStructureList[0].m_nSize = 12;
    m_pStructureList[0].m_nFirstField = 0;
    m_pStructureList[0].m_nLastField = 2;
    m_pStructureList[0].m_sName = "vector";

    m_nFieldCount = 3;
    m_pStructureFieldList[0].m_sName = "x";
    m_pStructureFieldList[0].m_nOffset = 0;
    m_pStructureFieldList[0].m_nType = KEYWORD_FLOAT;
    m_pStructureFieldList[1].m_sName = "y";
    m_pStructureFieldList[1].m_nOffset = 4;
    m_pStructureFieldList[1].m_nType = KEYWORD_FLOAT;
    m_pStructureFieldList[2].m_sName = "z";
    m_pStructureFieldList[2].m_nOffset = 8;
    m_pStructureFieldList[2].m_nType = KEYWORD_FLOAT;
}

// Saves the tokenizer cursor to include level nIndex and restarts the tokenizer at line 1
// column 1.
void CScriptCompilerInternal::InitializeIncludeFile(int nIndex)
{
    if (nIndex < 1)
        return;
    CScriptCompilerIncludeFile& slot = m_includeFiles[nIndex - 1];
    slot.m_nLine = m_nLine;
    slot.m_nColumn = m_nColumn;
    slot.m_nTokenType = m_nTokenType;
    slot.m_nTokenLength = m_nTokenLength;
    m_nLine = 1;
    m_nColumn = 1;
    TokenInitialize();
}

// Pops the tokenizer cursor saved by InitializeIncludeFile(nIndex). nIndex is 1-based; 0 is a
// no-op.
void CScriptCompilerInternal::ShutdownIncludeFile(int nIndex)
{
    if (nIndex < 1)
        return;
    CScriptCompilerIncludeFile& slot = m_includeFiles[nIndex - 1];
    m_nLine = slot.m_nLine;
    m_nColumn = slot.m_nColumn;
    m_nTokenType = slot.m_nTokenType;
    m_nTokenLength = slot.m_nTokenLength;
}

// Recursive post-order release of a parse tree; nMode == 1 also drops the global-variable
// entries the tree owns.
void CScriptCompilerInternal::DeleteParseTree(int nMode, CScriptParseTreeNode* pNode)
{
    if (pNode == 0)
        return;
    DeleteParseTree(nMode, pNode->m_pLeft);
    DeleteParseTree(nMode, pNode->m_pRight);
    if (nMode == 1) {
        for (int i = 0; i <= m_nSubroutineStackSize; i++) {
            if (m_pSubroutineStack[i].m_pNode)
                m_pSubroutineStack[i].m_pNode = 0;
            if (m_pSubroutineStack[i].m_pReturnNode)
                m_pSubroutineStack[i].m_pReturnNode = 0;
        }
    }
    delete pNode;
}

void CScriptCompilerInternal::SetCompileDebugLevel(int bEnable)
{
    m_bCompileDebugLevel = bEnable;
}

void CScriptCompilerInternal::SetCompileSymbolicOutput(int bEnable)
{
    m_bCompileSymbolicOutput = bEnable;
}

// Points the script compiler at a new identifier-specification file and rebuilds the tables it
// defines.
void CScriptCompilerInternal::SetIdentifierSpecification(const CExoString& sFile)
{
    if (m_sSourceFile != sFile) {
        m_sSourceFile = sFile;
        if (m_pIdentifierList != 0) {
            delete[] m_pIdentifierList;
            if (m_pIdentifierList != 0)
                return;
        }
        m_pIdentifierList = new CScriptCompilerIdListEntry[IDENTIFIER_LIST_COUNT];
        m_nCompileMode = COMPILE_MODE_IDENTIFIER_LIST;
        m_bCompileIdentifierConstants = 1;
        m_nIdentifierCount = 0;
        m_nIdentifierBase = 0;
        ParseIdentifierFile();
        m_nLine = 1;
        m_nColumn = 1;
        m_nSubroutineStackSize = -1;
        m_nCompileMode = COMPILE_MODE_PARSE_TREE;
        m_bCompileIdentifierConstants = 0;
    }
}

void CScriptCompilerInternal::SetOutputAlias(const CExoString& sAlias)
{
    m_sOutputAlias = sAlias;
}

void CScriptCompilerInternal::SetOptimizeBinaryCodeLength(int bEnable)
{
    m_bOptimizeBinaryCodeLength = bEnable;
}

void CScriptCompilerInternal::SetCompileConditionalFile(int bEnable)
{
    m_bCompileConditionalFile = bEnable;
}

// Compiles one source file. Drives #include by pushing include levels; only the outermost
// level generates code and writes the result out.
int CScriptCompilerInternal::CompileFile(const CExoString& sFile)
{
    if (m_nIncludeFileDepth == 0)
        Initialize();

    if (m_nIncludeFileDepth > INCLUDE_FILE_COUNT - 1)
        return -SCRIPT_ERROR_INCLUDE_TOO_MANY_LEVELS;

    for (int nIndex = 0; nIndex < m_nIncludeFileDepth; nIndex++)
    {
        if (m_includeFiles[nIndex].m_sFileName == sFile)
        {
            const char* pszFile = sFile.CStr();
            (void)pszFile;
            for (int nScan = m_nIncludeFileDepth - 1; nScan >= 0; nScan--)
            {
                const char* pszOpen = m_includeFiles[nScan].m_sFileName.CStr();
                (void)pszOpen;
            }
            return -SCRIPT_ERROR_INCLUDE_RECURSIVE;
        }
    }

    InitializeIncludeFile(m_nIncludeFileDepth);

    m_includeFiles[m_nIncludeFileDepth].m_sFileName = sFile;

    const char* pszFile = sFile.CStr();
    (void)pszFile;

    char* pSourceText;
    unsigned int nSize;
    CSourceScript& script = m_includeFiles[m_nIncludeFileDepth].m_script;
    CResRef resRef(sFile);
    script.SetResRef(resRef, 0);
    if (script.m_pRes->Demand() == 0)
    {
        if (m_nIncludeFileDepth >= 1)
        {
            CScriptCompilerIncludeFile& slot = m_includeFiles[m_nIncludeFileDepth - 1];
            m_nLine = slot.m_nLine;
            m_nColumn = slot.m_nColumn;
            m_nTokenType = slot.m_nTokenType;
            m_nTokenLength = slot.m_nTokenLength;
        }
        return -SCRIPT_ERROR_FILE_NOT_FOUND;
    }
    pSourceText = static_cast<char*>(script.m_pRes->GetNSSDataPtr());
    nSize = script.m_pRes->GetNSSSize();

    m_nIncludeFileDepth++;
    int nResult = ParseSource(pSourceText, nSize);
    if (nResult < 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        m_includeFiles[m_nIncludeFileDepth - 1].m_script.m_pRes->Release();
#else
        m_includeFiles[m_nIncludeFileDepth].m_script.m_pRes->Release();
#endif
        return nResult;
    }

    m_includeFiles[m_nIncludeFileDepth - 1].m_script.m_pRes->Release();

    m_nIncludeFileDepth--;
    if (m_nIncludeFileDepth >= 1)
    {
        CScriptCompilerIncludeFile& slot = m_includeFiles[m_nIncludeFileDepth - 1];
        m_nLine = slot.m_nLine;
        m_nColumn = slot.m_nColumn;
        m_nTokenType = slot.m_nTokenType;
        m_nTokenLength = slot.m_nTokenLength;
        return 0;
    }

    InitializeFinalCode();
    if (m_bCompileConditionalFile == 1)
        m_bConditionalCompile = 1;

    nResult = GenerateFinalCodeFromParseTree();
    if (m_bCompileConditionalFile == 1)
        m_bConditionalCompile = 0;

    if (nResult < 0)
        return nResult;

    FinalizeFinalCode();
    return WriteFinalCodeToFile(sFile);
}

// Compiles a bare expression by wrapping it in a `void main(){...}` shell.
int CScriptCompilerInternal::CompileScriptChunk(const CExoString& sSource)
{
    Initialize();

    if (m_nIncludeFileDepth != 0)
        return -SCRIPT_ERROR_INCLUDE_TOO_MANY_LEVELS;

    m_includeFiles[0].m_sFileName = "!Chunk";

#if KOTOR1
    unsigned int nLength = sSource.GetLength();
#else
    unsigned int nLength = sSource.GetLength();
#endif
    int nWrappedLength = static_cast<int>(nLength) + CHUNK_WRAPPER_LENGTH;
    char* pCode = new char[nLength + 2 * CHUNK_WRAPPER_LENGTH];
    sprintf(pCode, "void main(){%s}", sSource.CStr());

    m_nIncludeFileDepth++;
    int nResult = ParseSource(pCode, nWrappedLength);
    if (nResult < 0)
        return nResult;
    m_nIncludeFileDepth--;

    InitializeFinalCode();
    nResult = GenerateFinalCodeFromParseTree();
    if (nResult < 0)
        return nResult;

    FinalizeFinalCode();
    return 0;
}

// Like CompileScriptChunk, but wraps the expression in `int main(){ return(<cond>); }`.
int CScriptCompilerInternal::CompileScriptConditional(const CExoString& sSource)
{
    Initialize();

    if (m_nIncludeFileDepth != 0)
        return -SCRIPT_ERROR_INCLUDE_TOO_MANY_LEVELS;

    m_includeFiles[0].m_sFileName = "!Conditional";

#if KOTOR1
    unsigned int nLength = sSource.GetLength();
#else
    unsigned int nLength = sSource.GetLength();
#endif
    int nWrappedLength = static_cast<int>(nLength) + CONDITIONAL_WRAPPER_LENGTH;
    char* pCode = new char[nLength + 2 * CONDITIONAL_WRAPPER_LENGTH];
    sprintf(pCode, "int main(){ return(%s);}", sSource.CStr());

    m_nIncludeFileDepth++;
    int nResult = ParseSource(pCode, nWrappedLength);
    if (nResult < 0)
        return nResult;
    m_nIncludeFileDepth--;

    InitializeFinalCode();
    m_bConditionalCompile = 1;
    nResult = GenerateFinalCodeFromParseTree();
    m_bConditionalCompile = 0;
    if (nResult < 0)
        return nResult;

    FinalizeFinalCode();
    return 0;
}

// Returns the last compiled chunk's code pointer and byte count. Always reports success.
int CScriptCompilerInternal::GetCompiledScriptCode(char** ppCode, int* pnSize)
{
    *pnSize = m_nCompiledCodeSize;
    *ppCode = m_pCompiledCode;
    return 0;
}

// Frees the code buffer. The pointer itself is left dangling; Initialize() clears it.
void CScriptCompilerInternal::ClearCompiledScriptCode()
{
    char* pCode = m_pCompiledCode;
    m_nCompiledCodeSize = 0;
    delete[] pCode;
}

// Assembles the "file(line): message" line and returns the error code it was passed. A release build
// prints nothing: the assembled string is discarded, though the CStr() call ahead of the destructor
// stays.
//
// A '!'-prefixed file name is a synthetic chunk, "!Chunk" or "!Conditional", whose first character
// is stripped for display; a real file gets the ".nss" suffix the script compiler looks it up under.
// Each shape then splits on the line number: at least 1 gives the parenthesised form, otherwise the
// bare one.
int CScriptCompilerInternal::OutputError(int nType, const CExoString& sFile, int nLine, const CExoString& sMsg)
{
    CExoString sMessage;
    if (sFile.Left(1) == "!")
    {
        if (nLine >= 1)
        {
            sMessage.Format(const_cast<char*>("%s(%d): %s\n"), sFile.Right(sFile.GetLength() - 1).CStr(), nLine, sMsg.CStr());
        }
        else
        {
            sMessage.Format(const_cast<char*>("%s: %s\n"), sFile.Right(sFile.GetLength() - 1).CStr(), sMsg.CStr());
        }
    }
    else
    {
        if (nLine >= 1)
            sMessage.Format(const_cast<char*>("%s.nss(%d): %s\n"), sFile.CStr(), nLine, sMsg.CStr());
        else
            sMessage.Format(const_cast<char*>("%s.nss: %s\n"), sFile.CStr(), sMsg.CStr());
    }

    sMessage.CStr();
    return nType;
}

CSourceScript::~CSourceScript() {}
