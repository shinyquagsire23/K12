// CScriptCompilerInternal, the NWScript compiler core.

#include "k2stdint.h"
#include "nwscriptinternal.h"

#include "exolocstringinternal.h"

#include "exofile.h"
#include "exoresman.h"
#include "gameobjectarray.h"
#include "restype.h"

#include "port/libcstring.h"

#include <stdio.h>

// Maps a constant's identifier class (TOKEN_INTEGER_IDENTIFIER ... TOKEN_STRING_IDENTIFIER) to the
// literal token it stands for.
static const int s_IdentifierKeywordTypes[3] = { TOKEN_NUMBER, TOKEN_FLOAT, TOKEN_STRING };

enum { SWITCH_LABEL_COUNT = 16 };
enum { IDENTIFIER_LIST_COUNT = 4096 };
enum { SYMBOL_LIST_GROWTH = 32 };

// The code buffer starts at CODE_BUFFER_SIZE bytes and grows by as much whenever a statement leaves
// less than CODE_BUFFER_SLACK bytes free.
enum { CODE_BUFFER_SIZE = 256 * 1024 };
enum { CODE_BUFFER_SLACK = 200 };

// CompileScriptChunk's and CompileScriptConditional's wrappers, "void main(){%s}" and
// "int main(){ return(%s);}", without the "%s".
enum { CHUNK_WRAPPER_LENGTH = 13 };
enum { CONDITIONAL_WRAPPER_LENGTH = 22 };

// The argument types an action call accepts, as bits indexed by (type - KEYWORD_INT).
enum : unsigned long long {
    ACTION_ARGUMENT_TYPE_MASK =
          ((1ULL << (KEYWORD_OBJECT - KEYWORD_INT + 1)) - 1)       // int, float, string, object
        | (1ULL << (KEYWORD_ACTION - KEYWORD_INT))
        | (1ULL << (KEYWORD_STRUCT - KEYWORD_INT))
        | (((1ULL << ENGINE_STRUCTURE_COUNT) - 1) << (KEYWORD_ENGINE_STRUCTURE0 - KEYWORD_INT)),
};

// The symbolic listing (m_bCompileSymbolicOutput): each instruction line opens with its byte
// offset, "  %08x   ", and the header line is "NCS V1.0T %08x\n".
enum { SYMBOLIC_ADDRESS_WIDTH = 13 };
enum { SYMBOLIC_HEADER_SIZE = 20 };

// m_nVarStackVariableType outside a declaration: a NODE_VARIABLE then names an existing variable.
enum { DECLARATION_TYPE_NONE = 2 };

// GenerateIdentifierList's state (m_nIdentifierListState) while it reads the identifier file.
enum IdentifierListState {
    IDLIST_IDLE                  = 0,
    IDLIST_RETURN_TYPE           = 1,    // a type read; the name follows
    IDLIST_NAME                  = 2,    // a name read; '(' or '=' follows
    IDLIST_PARAMETERS            = 3,
    IDLIST_CONSTANT_VALUE        = 4,    // `name =` read; the constant's value follows
    IDLIST_CONSTANT_DONE         = 5,
    IDLIST_DEFINE                = 6,    // `#define` read
    IDLIST_ENGINE_NUM_STRUCTURES = 7,
    IDLIST_ENGINE_STRUCTURE      = 8,    // ENGINE_STRUCTURE_<n> read; its name follows
    IDLIST_PARAMETER_VECTOR      = 9,    // a parameter's default `[x, y, z]`
    IDLIST_CONSTANT_VECTOR       = 10,   // a constant's `[x, y, z]`
    IDLIST_PARAMETER_DEFAULT     = 11,   // a parameter's `=` read; its default follows
};

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

// Out of line (noinline), so its callers keep a real call to this copy.
__attribute__((noinline))
void CScriptCompilerInternal::TokenInitialize()
{
    m_nTokenType = TOKEN_NONE;
    m_nTokenLength = 0;
}

void CScriptCompilerInternal::SetCompileDebugLevel(int bEnable)
{
    m_bCompileDebugLevel = bEnable;
}

void CScriptCompilerInternal::SetCompileSymbolicOutput(int bEnable)
{
    m_bCompileSymbolicOutput = bEnable;
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

// Runs the codegen pass: loads the finished expression tree, then runs loader, emit, label resolve
// and relocate, and tears the compile down.
int CScriptCompilerInternal::GenerateFinalCodeFromParseTree()
{
    int nType;
    int nSubType;
    int nLocation;
    CScriptParseTreeNode* pNode;
    CScriptParseTreeNode* pReturnNode;

    PopSRStack(&nType, &nSubType, &nLocation, &pNode, &pReturnNode);

    m_bGeneratingFinalCode = 1;
    int nResult = InstallLoader();

    CScriptParseTreeNode* pTree = pReturnNode;
    if (m_pGlobalVariableParseTree != 0) {
        pTree = CreateScriptParseTreeNode(NODE_FUNCTIONAL_UNIT, m_pGlobalVariableParseTree, pReturnNode);
        m_pGlobalVariableParseTree = 0;
    }

#if KOTOR1
    if (nResult < 0) {
        OutputWalkTreeError(nResult, 0);
    }
    else {
        nResult = WalkParseTree(pTree);
        if (nResult >= 0) {
            if (m_bCompileSymbolicOutput == 0) {
                nResult = DetermineLocationOfCode();
                if (nResult < 0)
                    return CleanUpAfterCompile(nResult, pTree);
                nResult = ResolveLabels();
                if (nResult < 0)
                    return CleanUpAfterCompile(nResult, pTree);
                WriteResolvedOutput();
            }
            CleanUpAfterCompile(0, pTree);
            return 0;
        }
    }
    return CleanUpAfterCompile(nResult, pTree);
#else
    if (nResult < 0) {
        OutputWalkTreeError(nResult, 0);
        return CleanUpAfterCompile(nResult, pTree);
    }

    nResult = WalkParseTree(pTree);
    if (nResult < 0)
        return CleanUpAfterCompile(nResult, pTree);

    if (m_bCompileSymbolicOutput != 0)
        return CleanUpAfterCompile(0, pTree);

    nResult = DetermineLocationOfCode();
    if (nResult < 0)
        return CleanUpAfterCompile(nResult, pTree);

    nResult = ResolveLabels();
    if (nResult < 0)
        return CleanUpAfterCompile(nResult, pTree);

    WriteResolvedOutput();
    return CleanUpAfterCompile(0, pTree);
#endif
}

// Emits the load stub the VM calls: locate the entry point, reserve the #loader slot for the stub
// itself, then emit the jump to the user's body and the return.
int CScriptCompilerInternal::InstallLoader()
{
    int nLoaderIndex;
    if (m_bCompileConditionalFile != 0)
    {
        nLoaderIndex = GetIdentifierByName(CExoString("StartingConditional"));
        if (nLoaderIndex < 0)
            return -SCRIPT_ERROR_NO_FUNCTION_STARTING_CONDITIONAL_IN_SCRIPT;
        if (m_pIdentifierList[nLoaderIndex].m_nReturnType != TOKEN_INTEGER_IDENTIFIER)
            return -SCRIPT_ERROR_FUNCTION_STARTING_CONDITIONAL_MUST_HAVE_AN_INT_RETURN_VALUE;
        if (m_pIdentifierList[nLoaderIndex].m_nParameterCount != 0)
            return -SCRIPT_ERROR_FUNCTION_STARTING_CONDITIONAL_MUST_HAVE_NO_PARAMETERS;
    }
    else
    {
        nLoaderIndex = GetIdentifierByName(CExoString("main"));
        if (nLoaderIndex < 0)
            return -SCRIPT_ERROR_NO_FUNCTION_MAIN_IN_SCRIPT;
        if (m_pIdentifierList[nLoaderIndex].m_nReturnType != TOKEN_VOID_IDENTIFIER)
        {
            if (m_pIdentifierList[nLoaderIndex].m_nReturnType != TOKEN_INTEGER_IDENTIFIER)
                return -SCRIPT_ERROR_FUNCTION_MAIN_MUST_HAVE_A_VOID_RETURN_VALUE;
            if (m_bConditionalCompile != 1)
                return -SCRIPT_ERROR_FUNCTION_MAIN_MUST_HAVE_A_VOID_RETURN_VALUE;
        }
        if (m_pIdentifierList[nLoaderIndex].m_nParameterCount != 0)
            return -SCRIPT_ERROR_FUNCTION_MAIN_MUST_HAVE_NO_PARAMETERS;
    }

    CScriptParseTreeNode* pGlobalVariables = m_pGlobalVariableParseTree;
    m_pIdentifierList[m_nIdentifierCount].m_sName = "#loader";
    m_pIdentifierList[m_nIdentifierCount].m_nNameLength = 7;
    m_pIdentifierList[m_nIdentifierCount].m_nCodeOffset = m_nBinaryCodeOffset;
    m_pIdentifierList[m_nIdentifierCount].m_nOutputStart = -1;
    m_pIdentifierList[m_nIdentifierCount].m_nOutputEnd = -1;

    if (m_pIdentifierList[nLoaderIndex].m_nReturnType == TOKEN_INTEGER_IDENTIFIER)
    {
        m_nVarStackTop++;
        m_nVarStackRecursionLevel++;
        m_nGlobalVariableCount++;
        m_pVarStackList[m_nVarStackTop].m_sName = "#retval";
        m_pVarStackList[m_nVarStackTop].m_nType = KEYWORD_INT;
        m_pVarStackList[m_nVarStackTop].m_nRecursionLevel = m_nVarStackRecursionLevel;
        m_pVarStackList[m_nVarStackTop].m_nRunTimeLocation = m_nStackCurrentDepth * 4;
        AddVariableToStack(KEYWORD_INT, CExoString(""), 1);
        m_nStackCurrentDepth--;
    }

    if (m_bCompileSymbolicOutput != 0)
    {
        PrintBinaryAddress();
        if (pGlobalVariables != 0)
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_#globals\n");
            m_nCompiledCodeLocation += 29;
        }
        else if (m_bCompileConditionalFile != 0)
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_StartingConditional\n");
            m_nCompiledCodeLocation += 40;
        }
        else
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_main\n");
            m_nCompiledCodeLocation += 25;
        }
    }
    else
    {
        m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JSR;
        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;

        CExoString sLoaderSymbol;
        if (pGlobalVariables != 0)
            sLoaderSymbol.Format(const_cast<char*>("FE_#globals"));
        else if (m_bCompileConditionalFile == 1)
            sLoaderSymbol.Format(const_cast<char*>("FE_StartingConditional"));
        else
            sLoaderSymbol.Format(const_cast<char*>("FE_main"));

        AddSymbolToQueryList(sLoaderSymbol, m_nCompiledCodeLocation + 2);
        m_nCompiledCodeLocation += 6;
    }
    m_nBinaryCodeOffset += 6;

    if (m_bCompileSymbolicOutput != 0)
    {
        PrintBinaryAddress();
        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RETN\n");
        m_nCompiledCodeLocation += 18;
    }
    else
    {
        m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RETN;
        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
        m_nCompiledCodeLocation += 2;
    }
    m_nBinaryCodeOffset += 2;

    m_pIdentifierList[m_nIdentifierCount].m_nCodeEnd = m_nBinaryCodeOffset;
    if (m_nIdentifierCount++ >= IDENTIFIER_LIST_COUNT - 1)
        return OutputWalkTreeError(-SCRIPT_ERROR_IDENTIFIER_LIST_FULL, 0);
    return 0;
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
    unsigned int nLength = sSource.m_sString ? static_cast<unsigned int>(strlen(sSource.m_sString)) : 0;
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
    unsigned int nLength = sSource.m_sString ? static_cast<unsigned int>(strlen(sSource.m_sString)) : 0;
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

void CScriptCompilerInternal::ModifySRStackReturnTree(CScriptParseTreeNode* pReturnNode)
{
    if (m_nSubroutineStackSize >= 0)
        m_pSubroutineStack[m_nSubroutineStackSize].m_pReturnNode = pReturnNode;
}

// Pops the tokenizer cursor saved by InitializeIncludeFile(nIndex). nIndex is 1-based; 0 is a
// no-op.
__attribute__((noinline))
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

void CScriptCompilerInternal::InitializeSwitchLabelList()
{
    m_bSwitchHasDefault = 0;
    m_nSwitchLabelCount = 0;
    m_nSwitchLabelCapacity = SWITCH_LABEL_COUNT;
    m_pSwitchLabels = new int[SWITCH_LABEL_COUNT];
}

// Wraps the pending global-variable query node in a NODE_FUNCTIONAL_UNIT node and returns the
// node to carry on with.
CScriptParseTreeNode* CScriptCompilerInternal::InsertGlobalVariablesInParseTree(CScriptParseTreeNode* pNode)
{
    if (m_pGlobalVariableParseTree != 0) {
        CScriptParseTreeNode* pQuery = CreateScriptParseTreeNode(NODE_FUNCTIONAL_UNIT, m_pGlobalVariableParseTree, pNode);
        m_pGlobalVariableParseTree = 0;
        return pQuery;
    }
    return pNode;
}

// Finishes every ParseCharacter* as their shared tail: runs the current mode's consumer, then
// clears the half-built token. A negative consumer result is passed straight through.
int CScriptCompilerInternal::HandleToken()
{
    if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST) {
        int nRet = GenerateIdentifierList();
        if (nRet < 0)
            return nRet;
    } else {
        int nRet = GenerateParseTree();
        if (nRet < 0)
            return nRet;
    }
    TokenInitialize();
    return 0;
}

int CScriptCompilerInternal::HandleIdentifierToken()
{
    if (m_nTokenType == TOKEN_IDENTIFIER) {
        int nRet = TestIdentifierToken();
        if (nRet)
            return nRet;
    }
    return HandleToken();
}

int CScriptCompilerInternal::TestIdentifierToken()
{
    switch (m_nTokenLength)
    {
    case 2:
        if (strncmp(m_szTokenText, "if", 2) == 0)
            m_nTokenType = KEYWORD_IF;
        if (strncmp(m_szTokenText, "do", 2) == 0)
            m_nTokenType = KEYWORD_DO;
        break;
    case 3:
        if (strncmp(m_szTokenText, "int", 3) == 0)
            m_nTokenType = KEYWORD_INT;
        if (strncmp(m_szTokenText, "for", 3) == 0)
            m_nTokenType = KEYWORD_FOR;
        break;
    case 4:
        if (strncmp(m_szTokenText, "else", 4) == 0)
            m_nTokenType = KEYWORD_ELSE;
        if (strncmp(m_szTokenText, "void", 4) == 0)
            m_nTokenType = KEYWORD_VOID;
        if (strncmp(m_szTokenText, "case", 4) == 0)
            m_nTokenType = KEYWORD_CASE;
        break;
    case 5:
        if (strncmp(m_szTokenText, "float", 5) == 0)
            m_nTokenType = KEYWORD_FLOAT;
        if (strncmp(m_szTokenText, "while", 5) == 0)
            m_nTokenType = KEYWORD_WHILE;
        if (strncmp(m_szTokenText, "break", 4) == 0)
            m_nTokenType = KEYWORD_BREAK;
        break;
    case 6:
        if (strncmp(m_szTokenText, "string", 6) == 0)
            m_nTokenType = KEYWORD_STRING;
        if (strncmp(m_szTokenText, "object", 6) == 0)
            m_nTokenType = KEYWORD_OBJECT;
        if (strncmp(m_szTokenText, "return", 6) == 0)
            m_nTokenType = KEYWORD_RETURN;
        if (strncmp(m_szTokenText, "struct", 6) == 0)
            m_nTokenType = KEYWORD_STRUCT;
        if (strncmp(m_szTokenText, "action", 6) == 0)
            m_nTokenType = KEYWORD_ACTION;
        if (strncmp(m_szTokenText, "switch", 6) == 0)
            m_nTokenType = KEYWORD_SWITCH;
        if (strncmp(m_szTokenText, "vector", 6) == 0)
            m_nTokenType = KEYWORD_VECTOR;
        break;
    case 7:
        if (strncmp(m_szTokenText, "default", 7) == 0)
            m_nTokenType = KEYWORD_DEFAULT;
        if (strncmp(m_szTokenText, "#define", 7) == 0)
            m_nTokenType = KEYWORD_DEFINE;
        break;
    case 8:
        if (strncmp(m_szTokenText, "#include", 8) == 0)
            m_nTokenType = KEYWORD_INCLUDE;
        if (strncmp(m_szTokenText, "continue", 8) == 0)
            m_nTokenType = KEYWORD_CONTINUE;
        break;
    case 11:
        if (strncmp(m_szTokenText, "OBJECT_SELF", 11) == 0)
            m_nTokenType = KEYWORD_OBJECT_SELF;
        break;
    case 14:
        if (strncmp(m_szTokenText, "OBJECT_INVALID", 14) == 0)
            m_nTokenType = KEYWORD_OBJECT_INVALID;
        break;
    case 18:
        if (strncmp(m_szTokenText, "ENGINE_STRUCTURE_", 17) == 0 &&
            static_cast<unsigned int>(static_cast<unsigned char>(m_szTokenText[17]) - '0') <= 9)
            m_nTokenType = KEYWORD_ENGINE_STRUCTURE;
        break;
    case 21:
        if (strncmp(m_szTokenText, "ENGINE_NUM_STRUCTURES", 21) == 0)
            m_nTokenType = KEYWORD_ENGINE_NUM_STRUCTURES;
        break;
    }

    for (int nIndex = 0; nIndex < m_nEngineStructureCount; nIndex++)
    {
        if (m_pEngineStructureValid[nIndex] != 1)
            continue;
#if KOTOR1
        if (m_nTokenLength != m_pEngineStructureNames[nIndex].GetLength())
#else
        if (m_nTokenLength != static_cast<int>(m_pEngineStructureNames[nIndex].m_sString
                                     ? static_cast<int>(strlen(m_pEngineStructureNames[nIndex].m_sString)) : 0))
#endif
            continue;
        if (strncmp(m_szTokenText, m_pEngineStructureNames[nIndex].CStr(), m_nTokenLength) != 0)
            continue;

        m_nTokenType = nIndex + KEYWORD_ENGINE_STRUCTURE0;
    }

    if (m_nTokenType == TOKEN_IDENTIFIER && m_szTokenText[0] == '#')
        return -SCRIPT_ERROR_ELLIPSIS_IN_IDENTIFIER;

    int nMatch = 0;
    for (int nIndex = 0; nIndex < m_nIdentifierCount && nMatch == 0; nIndex++)
    {
        int64_t nNameLength = m_pIdentifierList[nIndex].m_nNameLength;
        nMatch = m_pIdentifierList[nIndex].m_nNameLength == m_nTokenLength;
        if (nMatch && nNameLength >= 1) {
            nMatch = nMatch & (m_pIdentifierList[nIndex].m_sName[0] == m_szTokenText[0]);
            for (int64_t k = 1; nMatch != 0 && k < nNameLength; k++)
                nMatch = m_pIdentifierList[nIndex].m_sName[static_cast<int>(k)] == m_szTokenText[k];
        }
        if (nMatch == 0)
            continue;

        if (m_pIdentifierList[nIndex].m_nIdentifierType != IDENTIFIER_TYPE_CONSTANT) {
            m_nTokenType = m_pIdentifierList[nIndex].m_nReturnType;
            continue;
        }
        if (static_cast<unsigned int>(m_pIdentifierList[nIndex].m_nReturnType - TOKEN_INTEGER_IDENTIFIER) <= 2)
            m_nTokenType = s_IdentifierKeywordTypes[m_pIdentifierList[nIndex].m_nReturnType - TOKEN_INTEGER_IDENTIFIER];

#if KOTOR1
        int nNewLength = m_pIdentifierList[nIndex].m_sStringData.GetLength();
#else
        int nNewLength = m_pIdentifierList[nIndex].m_sStringData.m_sString
                             ? static_cast<int>(strlen(m_pIdentifierList[nIndex].m_sStringData.m_sString))
                             : 0;
#endif
        for (int k = 0; k < nNewLength; k++)
            m_szTokenText[k] = m_pIdentifierList[nIndex].m_sStringData.CStr()[k];
        m_nTokenLength = nNewLength;
    }

    if (m_nTokenType == TOKEN_IDENTIFIER) {
        m_nTokenType = TOKEN_VARIABLE;
        return 0;
    }
    return 0;
}

int CScriptCompilerInternal::CheckForBadLValue(CScriptParseTreeNode* pNode)
{
    if (pNode->m_nType == NODE_VARIABLE)
        return 0;
    if (pNode->m_nType != NODE_STRUCTURE_PART)
        return 1;
    CScriptParseTreeNode* p = pNode->m_pLeft;
    if (!p)
        return 1;
    int nType;
    do {
        nType = p->m_nType;
        if (nType == NODE_VARIABLE)
            break;
        p = p->m_pLeft;
    } while (p != 0);
    return nType != NODE_VARIABLE;
}

// Runs the tokeniser-driven shift/reduce step, once per token: drains the subroutine stack,
// running one reduction rule per popped entry. A rule either pushes the next expected operand
// and returns 0 to wait for the next token, or reduces a construct and loops. Running the stack
// dry returns -2.
int CScriptCompilerInternal::GenerateParseTree()
{
    int nType;
    int nSubType;
    int nLocation;
    CScriptParseTreeNode* pNode;
    CScriptParseTreeNode* pReturnNode;

    while (m_nSubroutineStackSize >= 0) {
        nType = m_pSubroutineStack[m_nSubroutineStackSize].m_nType;
        nSubType = m_pSubroutineStack[m_nSubroutineStackSize].m_nSubType;
        nLocation = m_pSubroutineStack[m_nSubroutineStackSize].m_nLocation;
        pNode = m_pSubroutineStack[m_nSubroutineStackSize].m_pNode;
        pReturnNode = m_pSubroutineStack[m_nSubroutineStackSize].m_pReturnNode;
        m_nSubroutineStackSize = m_nSubroutineStackSize - 1;

        switch (nType) {
case GRAMMAR_PROGRAM:
    if (nSubType == 0 && nLocation == 0) {
        if (pNode == 0 && m_nIncludeFileDepth <= 1) PushSRStack(GRAMMAR_END_OF_PROGRAM, 0, 0, 0);
        if (m_nTokenType == TOKEN_END_OF_SOURCE) {
            ModifySRStackReturnTree(0);
            if (m_nIncludeFileDepth > 1) return 0;
        } else if (m_nTokenType == KEYWORD_INCLUDE) {
            PushSRStack(GRAMMAR_PROGRAM, 3, 1, pNode);
            return 0;
        } else {
            PushSRStack(GRAMMAR_PROGRAM, 1, 2, 0);
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 0, 0, 0);
        }
    }
    if (nSubType == 1 && nLocation == 2) {
        pNode = CreateScriptParseTreeNode(NODE_FUNCTIONAL_UNIT, pReturnNode, 0);
        PushSRStack(GRAMMAR_PROGRAM, 1, 3, pNode);
        PushSRStack(GRAMMAR_PROGRAM, 0, 0, pNode);
    } else if (nSubType == 1 && nLocation == 3) {
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 3 && nLocation == 1) {
        if (m_nTokenType == TOKEN_STRING) {
            CExoString sTemp;
            PushSRStack(GRAMMAR_PROGRAM, 3, 2, pNode);
            PushSRStack(GRAMMAR_PROGRAM, 0, 0, pNode);
            m_szTokenText[m_nTokenLength] = 0;
            sTemp.Format(const_cast<char*>("%s"), m_szTokenText);
            m_nTokenType = TOKEN_NONE;
            m_nTokenLength = 0;
            return CompileFile(sTemp);
        }
    }
    if (nSubType == 3 && nLocation == 2) {
        CScriptParseTreeNode* pLast = pReturnNode;
        if (pLast != 0) {
            CScriptParseTreeNode* p = pLast;
            while (p != 0) {
                PushSRStack(GRAMMAR_PROGRAM, 1, 3, p);
                pLast = p;
                p = p->m_pRight;
                pLast->m_pRight = 0;
            }
            PushSRStack(GRAMMAR_PROGRAM, 0, 0, pLast);
        } else {
            PushSRStack(GRAMMAR_PROGRAM, 0, 0, pNode);
        }
    }
    break;

case GRAMMAR_FUNCTIONAL_UNIT:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 0, 2, 0);
        PushSRStack(GRAMMAR_TYPE_SPECIFIER, 0, 0, 0);
    }
    if (nSubType == 0 && nLocation == 2) {
        if (pReturnNode->m_nType == NODE_TYPE_STRUCT && m_nTokenType == TOKEN_LEFT_BRACE) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STRUCTURE_DEFINITION, pReturnNode, 0);
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 3, 4, pNew);
            PushSRStack(GRAMMAR_STRUCTURE_BODY, 0, 0, 0);
            return 0;
        }
        if ((m_nTokenType >= TOKEN_VARIABLE && m_nTokenType <= TOKEN_VOID_IDENTIFIER) || m_nTokenType == TOKEN_STRUCTURE_IDENTIFIER ||
                        (m_nTokenType >= TOKEN_ENGINE_STRUCTURE0_IDENTIFIER && m_nTokenType <= TOKEN_ENGINE_STRUCTURE9_IDENTIFIER)) {
            CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_FUNCTION_IDENTIFIER, pReturnNode, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pA->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_FUNCTION_DECLARATION, pA, 0);
            CScriptParseTreeNode* pC = CreateScriptParseTreeNode(NODE_FUNCTION, pB, 0);
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 0, 3, pC);
            return 0;
        }
        return -SCRIPT_ERROR_FUNCTION_DEFINITION_MISSING_NAME;
    }
    if (nSubType == 0 && nLocation == 3) {
        if (m_nTokenType == TOKEN_SEMICOLON || m_nTokenType == TOKEN_COMMA || m_nTokenType == TOKEN_ASSIGN) {
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 4, 0, pNode);
        } else if (m_nTokenType == TOKEN_LEFT_BRACKET) {
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 0, 5, pNode);
            PushSRStack(GRAMMAR_PARAMETER_LIST, 0, 0, 0);
            return 0;
        } else {
            return -SCRIPT_ERROR_FUNCTION_DEFINITION_MISSING_PARAMETER_LIST;
        }
    }
    if (nSubType == 0 && nLocation == 5) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_MALFORMED_PARAMETER_LIST;
        pNode->m_pLeft->m_pRight = pReturnNode;
        PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 0, 6, pNode);
        return 0;
    }
    if (nSubType == 0 && nLocation == 6) {
        if (m_nTokenType == TOKEN_LEFT_BRACE) {
            int nResult = AddUserDefinedIdentifier(pNode, 1);
            if (nResult < 0) return nResult;
            PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 2, 8, pNode);
            PushSRStack(GRAMMAR_STATEMENT, 1, 2, 0);
            PushSRStack(GRAMMAR_COMPOUND_STATEMENT_BODY, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_SEMICOLON) {
            int nResult = AddUserDefinedIdentifier(pNode, 0);
            ModifySRStackReturnTree(pNode);
            return nResult;
        }
        return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
    }
    if (nSubType == 2 && nLocation == 8) {
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 3 && nLocation == 4) {
        if (m_nTokenType != TOKEN_RIGHT_BRACE) return -SCRIPT_ERROR_BAD_TYPE_SPECIFIER;
        pNode->m_pRight = pReturnNode;
        PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 3, 5, pNode);
        return 0;
    }
    if (nSubType == 3 && nLocation == 5) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_STRUCTURE;
        return AddToGlobalVariableList(pNode);
    }
    if (nSubType == 4 && nLocation == 0) {
        CScriptParseTreeNode* pDecl = CreateScriptParseTreeNode(NODE_DECLARATION, 0, 0);
        CScriptParseTreeNode* pParams = CreateScriptParseTreeNode(NODE_STATEMENT, pDecl, 0);
        CScriptParseTreeNode* pFunc = CreateScriptParseTreeNode(NODE_STATEMENT_LIST, pParams, 0);
        PushSRStack(GRAMMAR_FUNCTIONAL_UNIT, 4, 3, pFunc);
        pDecl->m_pLeft = pNode->m_pLeft->m_pLeft->m_pLeft;
        pNode->m_pLeft->m_pLeft->m_pLeft = 0;
        if (pDecl->m_pLeft->m_nType == NODE_TYPE_VOID)
            return -SCRIPT_ERROR_INVALID_DECLARATION_TYPE;
        pDecl->m_sName = pNode->m_pLeft->m_pLeft->m_sName;
        int nSavedToken = m_nTokenType;
        m_nTokenType = TOKEN_IDENTIFIER;
#if KOTOR1
        m_nTokenLength = pDecl->m_sName.GetLength();
#else
        m_nTokenLength = pDecl->m_sName.m_sString ? static_cast<int>(strlen(pDecl->m_sName.m_sString)) : 0;
#endif
        memcpy(m_szTokenText, pDecl->m_sName.CStr(), m_nTokenLength);  // idiom-exempt: runtime length (token length)
        int nResult = TestIdentifierToken();
        if (nResult != 0)
            return nResult;
        if (m_nTokenType != TOKEN_VARIABLE)
            return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
        CScriptParseTreeNode* pName = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
        m_szTokenText[m_nTokenLength] = 0;
        pName->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        CScriptParseTreeNode* pRef = CreateScriptParseTreeNode(NODE_VARIABLE_LIST, pName, 0);
        pDecl->m_pRight = pRef;
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR, 1, 2, pParams);
        m_nTokenType = nSavedToken;
        delete pNode->m_pLeft->m_pLeft;
        delete pNode->m_pLeft;
        delete pNode;
    }
    if (nSubType == 4 && nLocation == 3) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        return AddToGlobalVariableList(pNode);
    }
    break;

case GRAMMAR_END_OF_PROGRAM:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_END_OF_SOURCE)
            return -SCRIPT_ERROR_AFTER_END_COMPOUND_STATEMENT;
        PushSRStack(GRAMMAR_END_OF_PROGRAM, 1, 1, pReturnNode);
        ModifySRStackReturnTree(pReturnNode);
        return 0;
    }
    if (nSubType == 1 && nLocation == 1)
        ModifySRStackReturnTree(pReturnNode);
    break;

case GRAMMAR_PARAMETER_LIST:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) {
            PushSRStack(GRAMMAR_PARAMETER_LIST, 1, 2, 0);
            PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
        }
    }
    if (nSubType == 1 && nLocation == 2) {
        if (m_nTokenType != TOKEN_VARIABLE) return -SCRIPT_ERROR_BAD_VARIABLE_NAME;
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_FUNCTION_PARAMETER_NAME, 0, pReturnNode);
        m_szTokenText[m_nTokenLength] = 0;
        pNew->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        PushSRStack(GRAMMAR_PARAMETER_LIST, 1, 3, pNew);
        return 0;
    }
    if (nSubType == 1 && nLocation == 3) {
        if (m_nTokenType == TOKEN_ASSIGN) {
            PushSRStack(GRAMMAR_PARAMETER_LIST, 2, 4, pNode);
            PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 0, 0, 0);
            return 0;
        }
        PushSRStack(GRAMMAR_PARAMETER_LIST, 0, 1, pNode);
        PushSRStack(GRAMMAR_PARAMETER_LIST_TAIL, 0, 0, 0);
    } else if (nSubType == 2 && nLocation == 4) {
        pNode->m_pRight->m_pRight = pReturnNode;
        PushSRStack(GRAMMAR_PARAMETER_LIST, 0, 1, pNode);
        PushSRStack(GRAMMAR_PARAMETER_LIST_TAIL, 0, 0, 0);
    } else if (nSubType == 0 && nLocation == 1) {
        pNode->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_PARAMETER_LIST_TAIL:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) {
            if (m_nTokenType != TOKEN_COMMA) return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
            PushSRStack(GRAMMAR_PARAMETER_LIST, 0, 0, 0);
            return 0;
        }
        ModifySRStackReturnTree(0);
    }
    break;

case GRAMMAR_STRUCTURE_BODY:
    if (nSubType == 0 && nLocation == 0) {
        switch (m_nTokenType) {
        case KEYWORD_INT: case KEYWORD_FLOAT: case KEYWORD_STRING: case KEYWORD_OBJECT:
        case KEYWORD_STRUCT: case KEYWORD_VECTOR: {
            // Falls through to the test below rather than returning.
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STATEMENT, 0, 0);
            PushSRStack(GRAMMAR_STRUCTURE_BODY, 2, 2, pNew);
            PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
            break;
        }
        default:
            if (m_nTokenType >= KEYWORD_ENGINE_STRUCTURE0 && m_nTokenType <= KEYWORD_ENGINE_STRUCTURE9) {
                CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STATEMENT, 0, 0);
                PushSRStack(GRAMMAR_STRUCTURE_BODY, 2, 2, pNew);
                PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
                break;
            }
            if (m_nTokenType != TOKEN_RIGHT_BRACE) return -SCRIPT_ERROR_BAD_TYPE_SPECIFIER;
            ModifySRStackReturnTree(pNode);
            break;
        }
    }
    if (nSubType == 2 && nLocation == 2) {
        pNode->m_pLeft = CreateScriptParseTreeNode(NODE_DECLARATION, pReturnNode, 0);
        PushSRStack(GRAMMAR_STRUCTURE_BODY, 2, 3, pNode);
        PushSRStack(GRAMMAR_FIELD_LIST, 0, 0, 0);
    } else if (nSubType == 2 && nLocation == 3) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        pNode->m_pLeft->m_pRight = pReturnNode;
        PushSRStack(GRAMMAR_STRUCTURE_BODY, 2, 4, pNode);
        PushSRStack(GRAMMAR_STRUCTURE_BODY, 0, 0, 0);
        return 0;
    } else if (nSubType == 2 && nLocation == 4) {
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_FIELD_LIST:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_VARIABLE) return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
        pReturnNode = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
        m_szTokenText[m_nTokenLength] = 0;
        pReturnNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_VARIABLE_LIST, pReturnNode, 0);
        if (pNode != 0)
            pNode->m_pRight = pNew;
        PushSRStack(GRAMMAR_FIELD_LIST, 1, 2, pNew);
        PushSRStack(GRAMMAR_FIELD_LIST_TAIL, 0, 0, pNew);
        return 0;
    }
    if (nSubType == 1 && nLocation == 2) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_FIELD_LIST_TAIL:
    if (nSubType != 0 || nLocation != 0)
        return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
    if (m_nTokenType == TOKEN_SEMICOLON) {
        ModifySRStackReturnTree(pNode);
    } else if (m_nTokenType == TOKEN_COMMA) {
        PushSRStack(GRAMMAR_FIELD_LIST, 0, 0, pNode);
        return 0;
    } else {
        return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
    }
    break;

case GRAMMAR_COMPOUND_STATEMENT_BODY:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_RIGHT_BRACE) {
            PushSRStack(GRAMMAR_COMPOUND_STATEMENT_BODY, 2, 1, 0);
            PushSRStack(GRAMMAR_STATEMENT_LIST, 0, 0, 0);
        } else {
            ModifySRStackReturnTree(pNode);
        }
    }
    if (nSubType == 2 && nLocation == 1) {
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STATEMENT_LIST, pReturnNode, 0);
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pNew);
    }
    break;

case GRAMMAR_STATEMENT_LIST:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType == TOKEN_RIGHT_BRACE) {
            ModifySRStackReturnTree(0);
        } else {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STATEMENT, 0, 0);
            if (pNode != 0)
                pNode->m_pRight = pNew;
            PushSRStack(GRAMMAR_STATEMENT_LIST, 1, 1, pNew);
            PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
        }
    }
    if (nSubType == 1 && nLocation == 1) {
        PushSRStack(GRAMMAR_STATEMENT_LIST, 1, 2, pNode);
        pNode->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT_LIST, 0, 0, pNode);
    } else if (nSubType == 1 && nLocation == 2) {
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_STATEMENT:
    if (nSubType == 0 && nLocation == 0) {
        switch (m_nTokenType) {
        case TOKEN_LEFT_BRACE:
            PushSRStack(GRAMMAR_STATEMENT, 1, 2, 0);
            PushSRStack(GRAMMAR_COMPOUND_STATEMENT_BODY, 0, 0, 0);
            return 0;
        case KEYWORD_IF:
            PushSRStack(GRAMMAR_STATEMENT, 3, 2, 0);
            return 0;
        case KEYWORD_ELSE:
            return -SCRIPT_ERROR_ELSE_WITHOUT_IF_STATEMENT;
        case KEYWORD_INT: case KEYWORD_FLOAT: case KEYWORD_STRING: case KEYWORD_OBJECT:
        case KEYWORD_STRUCT: case KEYWORD_VECTOR:
            PushSRStack(GRAMMAR_STATEMENT, 10, 2, 0);
            PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
            break;
        case TOKEN_VOID_IDENTIFIER: {
            pNode = CreateScriptParseTreeNode(NODE_ACTION_ID, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_ACTION, 0, pNode);
            PushSRStack(GRAMMAR_STATEMENT, 2, 2, pNew);
            return 0;
        }
        case KEYWORD_RETURN:
            PushSRStack(GRAMMAR_STATEMENT, 6, 2, CreateScriptParseTreeNode(NODE_RETURN, 0, 0));
            return 0;
        case KEYWORD_WHILE:
            PushSRStack(GRAMMAR_STATEMENT, 7, 2, 0);
            return 0;
        case KEYWORD_FOR:
            PushSRStack(GRAMMAR_STATEMENT, 9, 2, 0);
            return 0;
        case KEYWORD_DO:
            PushSRStack(GRAMMAR_STATEMENT, 8, 2, 0);
            PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
            return 0;
        case KEYWORD_CASE:
            PushSRStack(GRAMMAR_STATEMENT, 13, 3, CreateScriptParseTreeNode(NODE_CASE, 0, 0));
            PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 0, 0, 0);
            return 0;
        case KEYWORD_BREAK:
            PushSRStack(GRAMMAR_STATEMENT, 14, 2, CreateScriptParseTreeNode(NODE_BREAK, 0, 0));
            return 0;
        case KEYWORD_SWITCH:
            PushSRStack(GRAMMAR_STATEMENT, 5, 2, CreateScriptParseTreeNode(NODE_SWITCH_BLOCK, 0, 0));
            return 0;
        case KEYWORD_DEFAULT:
            PushSRStack(GRAMMAR_STATEMENT, 12, 2, CreateScriptParseTreeNode(NODE_DEFAULT, 0, 0));
            return 0;
        case KEYWORD_CONTINUE:
            PushSRStack(GRAMMAR_STATEMENT, 15, 2, CreateScriptParseTreeNode(NODE_CONTINUE, 0, 0));
            return 0;
        default:
            if (m_nTokenType >= KEYWORD_ENGINE_STRUCTURE0 && m_nTokenType <= KEYWORD_ENGINE_STRUCTURE9) {
                PushSRStack(GRAMMAR_STATEMENT, 10, 2, 0);
                PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
            } else if (m_nTokenType == TOKEN_SEMICOLON) {
                ModifySRStackReturnTree(pNode);
                return 0;
            } else {
                PushSRStack(GRAMMAR_STATEMENT, 11, 2, 0);
                PushSRStack(GRAMMAR_EXPRESSION, 0, 0, 0);
            }
            break;
        }
    }
    // A flat sequence of independent ifs: non-returning arms fall through.
    if (nSubType == 1 && nLocation == 2) {
        if (m_nTokenType != TOKEN_RIGHT_BRACE) return -SCRIPT_ERROR_UNEXPECTED_END_COMPOUND_STATEMENT;
        PushSRStack(GRAMMAR_STATEMENT, 1, 3, CreateScriptParseTreeNode(NODE_COMPOUND_STATEMENT, pReturnNode, 0));
    }
    if (nSubType == 1 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACE) return -SCRIPT_ERROR_UNEXPECTED_END_COMPOUND_STATEMENT;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 2 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_ARG_LIST;
        PushSRStack(GRAMMAR_STATEMENT, 2, 3, pNode);
        PushSRStack(GRAMMAR_ARGUMENT_LIST, 0, 0, 0);
        return 0;
    }
    if (nSubType == 2 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_ARG_LIST;
        pNode->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT, 2, 4, pNode);
        return 0;
    }
    if (nSubType == 2 && nLocation == 4) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 3 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION;
        PushSRStack(GRAMMAR_STATEMENT, 3, 3, 0);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
        return 0;
    }
    if (nSubType == 3 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_EXPRESSION;
        pNode = CreateScriptParseTreeNode(NODE_IF_CONDITION, pReturnNode, 0);
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_IF_CHOICE, 0, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_IF_BLOCK, pNode, pA);
        PushSRStack(GRAMMAR_STATEMENT, 3, 5, pB);
        PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
        return 0;
    }
    if (nSubType == 3 && nLocation == 5) {
        if (pReturnNode == 0) return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        pNode->m_pRight->m_pLeft = CreateScriptParseTreeNode(NODE_STATEMENT, pReturnNode, 0);
        if (m_nTokenType == KEYWORD_ELSE) {
            PushSRStack(GRAMMAR_STATEMENT, 3, 7, pNode);
            PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
            return 0;
        }
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 3 && nLocation == 7) {
        pNode->m_pRight->m_pRight = CreateScriptParseTreeNode(NODE_STATEMENT, pReturnNode, 0);
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 5 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION;
        PushSRStack(GRAMMAR_STATEMENT, 5, 4, pNode);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
        return 0;
    }
    if (nSubType == 5 && nLocation == 4) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_EXPRESSION;
        pNode->m_pLeft = CreateScriptParseTreeNode(NODE_SWITCH_CONDITION, pReturnNode, 0);
        PushSRStack(GRAMMAR_STATEMENT, 5, 5, pNode);
        PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
        return 0;
    }
    if (nSubType == 5 && nLocation == 5) {
        pNode->m_pRight = CreateScriptParseTreeNode(NODE_STATEMENT, pReturnNode, 0);
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 6 && nLocation == 2) {
        if (m_nTokenType == TOKEN_SEMICOLON) {
            ModifySRStackReturnTree(pNode);
            return 0;
        }
        PushSRStack(GRAMMAR_STATEMENT, 6, 3, pNode);
        PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 6 && nLocation == 3) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_PARSING_RETURN_STATEMENT;
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 7 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION;
        PushSRStack(GRAMMAR_STATEMENT, 7, 3, 0);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
        return 0;
    }
    if (nSubType == 7 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) {
            if (pReturnNode != 0 && m_nTokenType == TOKEN_LEFT_BRACKET && pReturnNode->m_pLeft != 0 &&
                pReturnNode->m_pLeft->m_nType == NODE_VARIABLE) {
                m_sUndefinedIdentifier = pReturnNode->m_pLeft->m_sName;
                return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
            }
            return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_EXPRESSION;
        }
        pNode = CreateScriptParseTreeNode(NODE_WHILE_CONDITION, pReturnNode, 0);
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_WHILE_CHOICE, 0, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_WHILE_BLOCK, pNode, pA);
        PushSRStack(GRAMMAR_STATEMENT, 7, 5, pB);
        PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
        return 0;
    }
    if (nSubType == 7 && nLocation == 5) {
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_CONTINUE_TARGET, 0, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_STATEMENT, pA, 0);
        pNode->m_pRight->m_pLeft = CreateScriptParseTreeNode(NODE_STATEMENT, pReturnNode, pB);
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 8 && nLocation == 2) {
        if (m_nTokenType != KEYWORD_WHILE) return -SCRIPT_ERROR_NO_WHILE_AFTER_DO_KEYWORD;
        pNode = CreateScriptParseTreeNode(NODE_STATEMENT, pReturnNode, 0);
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_DOWHILE_CONDITION, 0, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_DOWHILE_BLOCK, pNode, pA);
        PushSRStack(GRAMMAR_STATEMENT, 8, 3, pB);
        return 0;
    }
    if (nSubType == 8 && nLocation == 4) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION;
        PushSRStack(GRAMMAR_STATEMENT, 8, 5, pNode);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
        return 0;
    }
    if (nSubType == 8 && nLocation == 5) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_EXPRESSION;
        pNode->m_pRight->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT, 8, 6, pNode);
        return 0;
    }
    if (nSubType == 8 && nLocation == 6) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 9 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET) return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION;
        pNode = CreateScriptParseTreeNode(NODE_CONTINUE_TARGET, 0, 0);
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_STATEMENT, 0, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_STATEMENT, 0, 0);
        CScriptParseTreeNode* pC = CreateScriptParseTreeNode(NODE_STATEMENT, pNode, pA);
        CScriptParseTreeNode* pD = CreateScriptParseTreeNode(NODE_WHILE_CHOICE, pB, pC);
        pNode = pD;
        CScriptParseTreeNode* pE = CreateScriptParseTreeNode(NODE_WHILE_CONDITION, 0, 0);
        CScriptParseTreeNode* pF = CreateScriptParseTreeNode(NODE_WHILE_BLOCK, pE, pNode);
        CScriptParseTreeNode* pG = CreateScriptParseTreeNode(NODE_STATEMENT, pF, 0);
        CScriptParseTreeNode* pH = CreateScriptParseTreeNode(NODE_STATEMENT, 0, pG);
        PushSRStack(GRAMMAR_STATEMENT, 9, 3, pH);
        return 0;
    }
    if (nSubType == 9 && nLocation == 3) {
        if (m_nTokenType == TOKEN_SEMICOLON) {
            PushSRStack(GRAMMAR_STATEMENT, 9, 5, pNode);
            return 0;
        }
        PushSRStack(GRAMMAR_STATEMENT, 9, 4, pNode);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 9 && nLocation == 4) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        pNode->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT, 9, 5, pNode);
        return 0;
    }
    if (nSubType == 9 && nLocation == 5) {
        if (m_nTokenType == TOKEN_SEMICOLON) {
            CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_CONSTANT_INTEGER, 0, 0);
            pA->m_nValue = 1;
            CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_NON_VOID_EXPRESSION, pA, 0);
            CScriptParseTreeNode* pC = CreateScriptParseTreeNode(NODE_INTEGER_EXPRESSION, pB, 0);
            pNode->m_pRight->m_pLeft->m_pLeft->m_pLeft = pC;
            PushSRStack(GRAMMAR_STATEMENT, 9, 7, pNode);
            return 0;
        }
        PushSRStack(GRAMMAR_STATEMENT, 9, 6, pNode);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 9 && nLocation == 6) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        pNode->m_pRight->m_pLeft->m_pLeft->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT, 9, 7, pNode);
        return 0;
    }
    if (nSubType == 9 && nLocation == 7) {
        if (m_nTokenType == TOKEN_RIGHT_BRACKET) {
            PushSRStack(GRAMMAR_STATEMENT, 9, 9, pNode);
            return 0;
        }
        PushSRStack(GRAMMAR_STATEMENT, 9, 8, pNode);
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 9 && nLocation == 8) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        pNode->m_pRight->m_pLeft->m_pRight->m_pRight->m_pRight->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_STATEMENT, 9, 9, pNode);
        return 0;
    }
    if (nSubType == 9 && nLocation == 9) {
        PushSRStack(GRAMMAR_STATEMENT, 9, 10, pNode);
        PushSRStack(GRAMMAR_STATEMENT, 0, 0, 0);
    }
    if (nSubType == 9 && nLocation == 10) {
        pNode->m_pRight->m_pLeft->m_pRight->m_pLeft->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    if (nSubType == 10 && nLocation == 2) {
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_DECLARATION, pReturnNode, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_STATEMENT, pA, 0);
        CScriptParseTreeNode* pC = CreateScriptParseTreeNode(NODE_STATEMENT_LIST, pB, 0);
        PushSRStack(GRAMMAR_STATEMENT, 10, 3, pC);
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR, 0, 0, pB);
    }
    if (nSubType == 11 && nLocation == 2) {
        if (m_nTokenType != TOKEN_SEMICOLON) {
            if (pReturnNode != 0 && m_nTokenType == TOKEN_LEFT_BRACKET) {
                m_sUndefinedIdentifier = pReturnNode->m_sName;
                return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
            }
            return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION;
        }
        ModifySRStackReturnTree(pReturnNode);
        return 0;
    }
    if (nSubType == 12 && nLocation == 2) {
        if (m_nTokenType != TOKEN_COLON) return -SCRIPT_ERROR_NO_COLON_AFTER_DEFAULT_LABEL;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 13 && nLocation == 3) {
        if (m_nTokenType != TOKEN_COLON) return -SCRIPT_ERROR_NO_COLON_AFTER_CASE_LABEL;
        pNode->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if ((nSubType == 14 || nSubType == 15) && nLocation == 2) {
        if (m_nTokenType != TOKEN_SEMICOLON) return -SCRIPT_ERROR_NO_SEMICOLON_AFTER_STATEMENT;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    break;

case GRAMMAR_TYPE_SPECIFIER:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType == KEYWORD_VOID) {
            pNode = CreateScriptParseTreeNode(NODE_TYPE_VOID, 0, 0);
            ModifySRStackReturnTree(pNode);
            return 0;
        }
        PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 0, 0, 0);
    }
    break;

case GRAMMAR_NON_VOID_TYPE_SPECIFIER:
    if (nSubType == 0 && nLocation == 0) {
        if ((m_nTokenType >= KEYWORD_INT && m_nTokenType <= KEYWORD_OBJECT)
            || (m_nTokenType >= KEYWORD_ENGINE_STRUCTURE0 && m_nTokenType <= KEYWORD_ENGINE_STRUCTURE9)) {
            pNode = CreateScriptParseTreeNode(0, 0, 0);
            if (m_nTokenType >= KEYWORD_INT && m_nTokenType <= KEYWORD_OBJECT)
                pNode->m_nType = m_nTokenType - (KEYWORD_INT - NODE_TYPE_INT);
            else if (m_nTokenType >= KEYWORD_ENGINE_STRUCTURE0 && m_nTokenType <= KEYWORD_ENGINE_STRUCTURE9)
                pNode->m_nType = m_nTokenType - (KEYWORD_ENGINE_STRUCTURE0 - NODE_TYPE_ENGINE_STRUCTURE0);
            ModifySRStackReturnTree(pNode);
            return 0;
        }
        if (m_nTokenType == KEYWORD_STRUCT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_TYPE_STRUCT, 0, 0);
            PushSRStack(GRAMMAR_NON_VOID_TYPE_SPECIFIER, 5, 1, pNew);
            return 0;
        }
        if (m_nTokenType == KEYWORD_VECTOR) {
            pNode = CreateScriptParseTreeNode(NODE_TYPE_STRUCT, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            ModifySRStackReturnTree(pNode);
            return 0;
        }
        return -SCRIPT_ERROR_INVALID_DECLARATION_TYPE;
    }
    if (nSubType == 5 && nLocation == 1) {
        if (m_nTokenType != TOKEN_VARIABLE)
            return -SCRIPT_ERROR_INVALID_DECLARATION_TYPE;
        m_szTokenText[m_nTokenLength] = 0;
        pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    break;

case GRAMMAR_VARIABLE_DECLARATOR:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_VARIABLE) return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
        pReturnNode = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
        m_szTokenText[m_nTokenLength] = 0;
        pReturnNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_VARIABLE_LIST, pReturnNode, 0);
        if (pNode != 0 && pNode->m_pLeft != 0)
            pNode->m_pLeft->m_pRight = pNew;
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR, 1, 2, pNode);
        return 0;
    }
    if (nSubType == 1 && nLocation == 2) {
        if (m_nTokenType == TOKEN_ASSIGN) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
            if (pNode != 0 && pNode->m_pLeft != 0 && pNode->m_pLeft->m_pRight != 0 &&
                pNode->m_pLeft->m_pRight->m_pLeft != 0)
                pNew->m_sName.Format(const_cast<char*>("%s"), pNode->m_pLeft->m_pRight->m_pLeft->m_sName.CStr());
            CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_ASSIGNMENT, 0, pNew);
            CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_STATEMENT, pA, 0);
            pNode->m_pRight = pB;
            PushSRStack(GRAMMAR_VARIABLE_DECLARATOR, 2, 2, pB);
            PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 0, 0, 0);
            return 0;
        }
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR_TAIL, 0, 0, pNode);
    } else if (nSubType == 2 && nLocation == 2) {
        pNode->m_pLeft->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR_TAIL, 0, 0, pNode);
    }
    break;

case GRAMMAR_VARIABLE_DECLARATOR_TAIL:
    if (nSubType != 0 || nLocation != 0)
        return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
    if (m_nTokenType == TOKEN_SEMICOLON) {
        ModifySRStackReturnTree(pNode);
    } else if (m_nTokenType == TOKEN_COMMA) {
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(0, 0, 0);
        if (pNode != 0 && pNode->m_pLeft != 0 && pNode->m_pLeft->m_pLeft != 0) {
            CScriptParseTreeNode* p = pNode->m_pLeft->m_pLeft;
            pNew->m_nType = p->m_nType;
            if (p->m_nType == NODE_TYPE_STRUCT)
                pNew->m_sName = p->m_sName;
        }
        CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_DECLARATION, pNew, 0);
        CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_STATEMENT, pA, 0);
        pNode->m_pRight = pB;
        PushSRStack(GRAMMAR_VARIABLE_DECLARATOR, 0, 0, pB);
        return 0;
    } else {
        return -SCRIPT_ERROR_PARSING_VARIABLE_LIST;
    }
    break;

case GRAMMAR_ARGUMENT_LIST:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET) {
            if (m_nTokenType == TOKEN_RIGHT_BRACE)
                return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
            PushSRStack(GRAMMAR_ARGUMENT_LIST, 2, 1, 0);
            PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 0, 0, 0);
        }
    }
    if (nSubType == 2 && nLocation == 1) {
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_ACTION_ARG_LIST, 0, pReturnNode);
        PushSRStack(GRAMMAR_ARGUMENT_LIST, 2, 2, pNew);
        PushSRStack(GRAMMAR_ARGUMENT_LIST_TAIL, 0, 0, 0);
    }
    if (nSubType == 2 && nLocation == 2) {
        pNode->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_ARGUMENT_LIST_TAIL:
    if (nSubType == 0 && nLocation == 0) {
        if (m_nTokenType == TOKEN_RIGHT_BRACKET) {
            ModifySRStackReturnTree(0);
        } else if (m_nTokenType == TOKEN_COMMA) {
            PushSRStack(GRAMMAR_ARGUMENT_LIST_TAIL, 2, 2, 0);
            PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 0, 0, 0);
            return 0;
        } else {
            CScriptParseTreeNode* p = m_pSubroutineStack[m_nSubroutineStackSize].m_pNode;
            if (p == 0)
                return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
            p = p->m_pRight;
            if (p == 0)
                return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
            if (p->m_nType != NODE_VARIABLE)
                return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
            m_sUndefinedIdentifier = p->m_sName;
            return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
        }
    }
    if (nSubType == 2 && nLocation == 2) {
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_ACTION_ARG_LIST, 0, pReturnNode);
        PushSRStack(GRAMMAR_ARGUMENT_LIST_TAIL, 2, 3, pNew);
        PushSRStack(GRAMMAR_ARGUMENT_LIST_TAIL, 0, 0, 0);
    }
    break;

case GRAMMAR_BOOLEAN_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_BOOLEAN_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 1 && nLocation == 1) {
        CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_INTEGER_EXPRESSION, pReturnNode, 0);
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pNew);
    }
    break;

case GRAMMAR_NON_VOID_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_NON_VOID_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 1 && nLocation == 1) {
        CreateScriptParseTreeNode(NODE_NON_VOID_EXPRESSION, pReturnNode, 0);
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pReturnNode);
    }
    break;

case GRAMMAR_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_ASSIGNMENT_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 1 && nLocation == 1) {
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pReturnNode);
    }
    break;

case GRAMMAR_ASSIGNMENT_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_ASSIGNMENT_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 1 && nLocation == 1) {
        if ((m_nTokenType >= TOKEN_MINUS_ASSIGN && m_nTokenType <= TOKEN_TRIPLE_SHIFT_RIGHT_ASSIGN) || m_nTokenType == TOKEN_ASSIGN) {
            if (CheckForBadLValue(pReturnNode) != 0)
                return -SCRIPT_ERROR_CANNOT_ASSIGN_TO_LEFT_SIDE;
            if (m_nTokenType == TOKEN_ASSIGN) {
                CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_ASSIGNMENT, 0, pReturnNode);
                PushSRStack(GRAMMAR_ASSIGNMENT_EXPRESSION, 2, 3, pNew);
                PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 0, 0, 0);
                return 0;
            }
            CScriptParseTreeNode* pDup = DuplicateScriptParseTree(pReturnNode);
            CScriptParseTreeNode* pArg = pDup;
            int nSub = 0;
            if (m_nTokenType == TOKEN_MINUS_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_SUBTRACT, pDup, 0); nSub = 3;  }
            if (m_nTokenType == TOKEN_PLUS_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_ADD, pDup, 0); nSub = 4;  }
            if (m_nTokenType == TOKEN_ASTERISK_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_MULTIPLY, pDup, 0); nSub = 5;  }
            if (m_nTokenType == TOKEN_SLASH_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_DIVIDE, pDup, 0); nSub = 6;  }
            if (m_nTokenType == TOKEN_PERCENT_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_MODULUS, pDup, 0); nSub = 7;  }
            if (m_nTokenType == TOKEN_AMPERSAND_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_BOOLEAN_AND, pDup, 0); nSub = 8;  }
            if (m_nTokenType == TOKEN_CARAT_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_EXCLUSIVE_OR, pDup, 0); nSub = 9;  }
            if (m_nTokenType == TOKEN_VERTICALBAR_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_INCLUSIVE_OR, pDup, 0); nSub = 10; }
            if (m_nTokenType == TOKEN_SHIFT_LEFT_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_SHIFT_LEFT, pDup, 0); nSub = 11; }
            if (m_nTokenType == TOKEN_SHIFT_RIGHT_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_SHIFT_RIGHT, pDup, 0); nSub = 12; }
            if (m_nTokenType == TOKEN_TRIPLE_SHIFT_RIGHT_ASSIGN) { pArg = CreateScriptParseTreeNode(NODE_UNSIGNED_SHIFT_RIGHT, pDup, 0); nSub = 13; }
            CScriptParseTreeNode* pOuter = CreateScriptParseTreeNode(NODE_ASSIGNMENT, pArg, pReturnNode);
            PushSRStack(GRAMMAR_ASSIGNMENT_EXPRESSION, nSub, 3, pOuter);
            PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pReturnNode);
    }
    if (nLocation == 3) {
        if (pReturnNode != 0 && m_nTokenType == TOKEN_LEFT_BRACKET && pReturnNode->m_nType == NODE_VARIABLE) {
            m_sUndefinedIdentifier = pReturnNode->m_sName;
            return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
        }
        if (nSubType == 2)
            pNode->m_pLeft = pReturnNode;
        else
            pNode->m_pLeft->m_pRight = pReturnNode;
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_CONDITIONAL_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_LOGICAL_OR_EXPRESSION, 0, 0, 0);
    }
    if (nSubType == 1 && nLocation == 1) {
        if (m_nTokenType != TOKEN_QUESTION_MARK) {
            if (m_nSubroutineStackSize < 0)
                return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
            ModifySRStackReturnTree(pReturnNode);
        } else {
            pNode = CreateScriptParseTreeNode(NODE_CONDITIONAL_CONDITION, pReturnNode, 0);
            CScriptParseTreeNode* pA = CreateScriptParseTreeNode(NODE_CONDITIONAL_CHOICE, 0, 0);
            CScriptParseTreeNode* pB = CreateScriptParseTreeNode(NODE_CONDITIONAL_BLOCK, pNode, pA);
            PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 2, 3, pB);
            return 0;
        }
    } else if (nSubType == 2 && nLocation == 3) {
        if (m_nTokenType != TOKEN_COLON) return -SCRIPT_ERROR_CONDITIONAL_REQUIRES_SECOND_EXPRESSION;
        pNode->m_pRight->m_pLeft = pReturnNode;
        PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 2, 5, pNode);
        PushSRStack(GRAMMAR_CONDITIONAL_EXPRESSION, 0, 0, 0);
        return 0;
    } else if (nSubType == 2 && nLocation == 5) {
        pNode->m_pRight->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    }
    break;

case GRAMMAR_LOGICAL_OR_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_LOGICAL_OR_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_LOGICAL_AND_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_LOGICAL_OR) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_LOGICAL_OR, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_LOGICAL_OR_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_LOGICAL_AND_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    break;

case GRAMMAR_LOGICAL_AND_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_LOGICAL_AND_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_INCLUSIVE_OR_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_LOGICAL_AND) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_LOGICAL_AND, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_LOGICAL_AND_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_INCLUSIVE_OR_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    break;

case GRAMMAR_INCLUSIVE_OR_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_INCLUSIVE_OR_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_EXCLUSIVE_OR_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_VERTICAL_BAR) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_INCLUSIVE_OR, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_INCLUSIVE_OR_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_EXCLUSIVE_OR_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    if (nLocation == 3) {
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    } else {
        ModifySRStackReturnTree(pReturnNode);
    }
    break;

case GRAMMAR_EXCLUSIVE_OR_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_EXCLUSIVE_OR_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_BOOLEAN_AND_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_CARAT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_EXCLUSIVE_OR, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_EXCLUSIVE_OR_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_BOOLEAN_AND_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    if (nLocation == 3) {
        pNode->m_pRight = pReturnNode;
        ModifySRStackReturnTree(pNode);
    } else {
        ModifySRStackReturnTree(pReturnNode);
    }
    break;

case GRAMMAR_BOOLEAN_AND_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_BOOLEAN_AND_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_EQUALITY_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_AMPERSAND) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_BOOLEAN_AND, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_BOOLEAN_AND_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_EQUALITY_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (nLocation == 3) {
            pNode->m_pRight = pReturnNode;
            ModifySRStackReturnTree(pNode);
        } else {
            ModifySRStackReturnTree(pReturnNode);
        }
    }
    break;

case GRAMMAR_EQUALITY_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_EQUALITY_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_EQUAL) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_EQUAL, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_EQUALITY_EXPRESSION, 3, 3, pNew);
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_NOT_EQUAL) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_NOT_EQUAL, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_EQUALITY_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    break;

case GRAMMAR_RELATIONAL_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        switch (m_nTokenType) {
        case TOKEN_GREATER_EQUAL: {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_GREATER_EQUAL, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 0, 0, 0);
            return 0;
        }
        case TOKEN_LESS_EQUAL: {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_LESS_EQUAL, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 3, 3, pNew);
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 0, 0, 0);
            return 0;
        }
        case TOKEN_GREATER_THAN: {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_GREATER_THAN, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 5, 3, pNew);
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 0, 0, 0);
            return 0;
        }
        case TOKEN_LESS_THAN: {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_LESS_THAN, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_RELATIONAL_EXPRESSION, 4, 3, pNew);
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 0, 0, 0);
            return 0;
        }
        }
        if (nLocation == 3) {
            pNode->m_pRight = pReturnNode;
            ModifySRStackReturnTree(pNode);
        } else {
            ModifySRStackReturnTree(pReturnNode);
        }
    }
    break;

case GRAMMAR_SHIFT_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_TRIPLE_SHIFT_RIGHT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_UNSIGNED_SHIFT_RIGHT, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 4, 3, pNew);
            PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_SHIFT_RIGHT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_SHIFT_RIGHT, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 3, 3, pNew);
            PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_SHIFT_LEFT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_SHIFT_LEFT, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_SHIFT_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (nLocation == 3) {
            pNode->m_pRight = pReturnNode;
            ModifySRStackReturnTree(pNode);
        } else {
            ModifySRStackReturnTree(pReturnNode);
        }
    }
    break;

case GRAMMAR_ADDITIVE_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_MINUS) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_SUBTRACT, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 3, 3, pNew);
            PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_PLUS) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_ADD, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_ADDITIVE_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 0, 0, 0);
            return 0;
        }
    }
    break;

case GRAMMAR_MULTIPLICATIVE_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 1, 1, 0);
        PushSRStack(GRAMMAR_UNARY_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 1) || nLocation == 3) {
        if (m_nTokenType == TOKEN_DIVIDE) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_DIVIDE, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 3, 3, pNew);
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_PERCENT) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_MODULUS, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 4, 3, pNew);
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (m_nTokenType == TOKEN_ASTERISK) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_MULTIPLY, pReturnNode, 0);
            if (nLocation == 3) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            }
            PushSRStack(GRAMMAR_MULTIPLICATIVE_EXPRESSION, 2, 3, pNew);
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 0, 0, 0);
            return 0;
        }
        if (nLocation == 3) {
            pNode->m_pRight = pReturnNode;
            ModifySRStackReturnTree(pNode);
        } else {
            ModifySRStackReturnTree(pReturnNode);
        }
    }
    break;

case GRAMMAR_UNARY_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        switch (m_nTokenType) {
        case TOKEN_MINUS:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 1, 1, CreateScriptParseTreeNode(NODE_NEGATE, 0, 0));
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            return 0;
        case TOKEN_TILDE:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 2, 1, CreateScriptParseTreeNode(NODE_ONES_COMPLEMENT, 0, 0));
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            return 0;
        case TOKEN_EXCLAMATION:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 3, 1, CreateScriptParseTreeNode(NODE_BOOLEAN_NOT, 0, 0));
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            return 0;
        case TOKEN_PLUS_PLUS:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 4, 1, CreateScriptParseTreeNode(NODE_PRE_INCREMENT, 0, 0));
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            return 0;
        case TOKEN_MINUS_MINUS:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 5, 1, CreateScriptParseTreeNode(NODE_PRE_DECREMENT, 0, 0));
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            return 0;
        default:
            PushSRStack(GRAMMAR_UNARY_EXPRESSION, 6, 1, 0);
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 0, 0, 0);
            break;
        }
    }
    if (nSubType >= 1 && nSubType <= 5 && nLocation == 1) {
        pNode->m_pLeft = pReturnNode;
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pNode);
    } else if (nSubType == 6 && nLocation == 1) {
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pReturnNode);
    }
    break;

case GRAMMAR_POSTFIX_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 1, 2, 0);
        PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 0, 0, 0);
    }
    if ((nSubType == 1 && nLocation == 2) || nLocation == 4) {
        if (m_nTokenType == TOKEN_MINUS_MINUS) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_POST_DECREMENT, pReturnNode, 0);
            if (nLocation == 4) { pNew->m_pLeft = pNode; pNode->m_pRight = pReturnNode; }
            else pNew->m_pLeft = pReturnNode;
            ModifySRStackReturnTree(pNew);
            return 0;
        }
        if (m_nTokenType == TOKEN_PLUS_PLUS) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_POST_INCREMENT, pReturnNode, 0);
            if (nLocation == 4) { pNew->m_pLeft = pNode; pNode->m_pRight = pReturnNode; }
            else pNew->m_pLeft = pReturnNode;
            ModifySRStackReturnTree(pNew);
            return 0;
        }
        if (m_nTokenType == TOKEN_PERIOD) {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STRUCTURE_PART, 0, 0);
            if (nLocation == 4) {
                pNew->m_pLeft = pNode;
                pNode->m_pRight = pReturnNode;
            } else if (pReturnNode == 0 && pNode != 0) {
                pNew->m_pLeft = pNode;
            } else {
                pNew->m_pLeft = pReturnNode;
            }
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 2, 4, pNew);
            PushSRStack(GRAMMAR_POSTFIX_EXPRESSION, 2, 3, 0);
            return 0;
        }
        if (nLocation == 4) {
            pNode->m_pRight = pReturnNode;
            ModifySRStackReturnTree(pNode);
        } else {
            ModifySRStackReturnTree(pReturnNode);
        }
    }
    if (nSubType == 2 && nLocation == 3) {
        pNode = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
        m_szTokenText[m_nTokenLength] = 0;
        pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    break;

case GRAMMAR_PRIMARY_EXPRESSION:
    if (nSubType == 0 && nLocation == 0) {
        switch (m_nTokenType) {
        case TOKEN_NUMBER:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_HEX_NUMBER:
        case KEYWORD_OBJECT_SELF:
        case KEYWORD_OBJECT_INVALID:
        case TOKEN_LEFT_SQUARE:
            PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 2, 1, 0);
            PushSRStack(GRAMMAR_CONSTANT, 0, 0, 0);
            return 0;

        case TOKEN_VARIABLE: {
            pNode = CreateScriptParseTreeNode(NODE_VARIABLE, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            ModifySRStackReturnTree(pNode);
            return 0;
        }

        case TOKEN_INTEGER_IDENTIFIER:
        case TOKEN_FLOAT_IDENTIFIER:
        case TOKEN_STRING_IDENTIFIER:
        case TOKEN_OBJECT_IDENTIFIER:
        case TOKEN_VOID_IDENTIFIER:
        case TOKEN_STRUCTURE_IDENTIFIER:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 1:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 2:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 3:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 4:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 5:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 6:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 7:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 8:
        case TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 9: {
            CScriptParseTreeNode* p1 = CreateScriptParseTreeNode(NODE_ACTION_ID, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            p1->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            CScriptParseTreeNode* p2 = CreateScriptParseTreeNode(NODE_ACTION, 0, p1);
            if (m_nTokenType == TOKEN_VOID_IDENTIFIER) {
                CScriptParseTreeNode* p3 = CreateScriptParseTreeNode(NODE_ACTION_PARAMETER, p2, 0);
                PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 5, 2, p3);
            } else {
                PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 1, 2, p2);
            }
            return 0;
        }

        case TOKEN_LEFT_BRACKET:
            PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 3, 2, 0);
            PushSRStack(GRAMMAR_EXPRESSION, 0, 0, 0);
            return 0;

        default:
            return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
        }
    }
    if (nSubType == 1 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET)
            return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_ARG_LIST;
        PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 1, 3, pNode);
        PushSRStack(GRAMMAR_ARGUMENT_LIST, 0, 0, 0);
        return 0;
    }
    if (nSubType == 1 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET)
            return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_ARG_LIST;
        pNode->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 5 && nLocation == 2) {
        if (m_nTokenType != TOKEN_LEFT_BRACKET)
            return -SCRIPT_ERROR_NO_LEFT_BRACKET_ON_ARG_LIST;
        PushSRStack(GRAMMAR_PRIMARY_EXPRESSION, 5, 3, pNode);
        PushSRStack(GRAMMAR_ARGUMENT_LIST, 0, 0, 0);
        return 0;
    }
    if (nSubType == 5 && nLocation == 3) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET)
            return -SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_ARG_LIST;
        pNode->m_pLeft->m_pLeft = pReturnNode;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 2 && nLocation == 1) {
        if (m_nSubroutineStackSize < 0)
            return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
        ModifySRStackReturnTree(pReturnNode);
    }
    if (nSubType == 3 && nLocation == 2) {
        if (m_nTokenType != TOKEN_RIGHT_BRACKET)
            return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
        ModifySRStackReturnTree(pReturnNode);
        return 0;
    }
    break;

case GRAMMAR_CONSTANT:
    if (nSubType == 0 && nLocation == 0) {
        switch (m_nTokenType) {
        case TOKEN_NUMBER: {
            // For a decimal literal, m_nValue accumulates the digits; a leading '-' sets the sign
            // flag.
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_CONSTANT_INTEGER, 0, 0);
            pNew->m_nValue = 0;
            int nSign = 1;
            for (int i = 0; i < m_nTokenLength; i++) {
                if (m_szTokenText[i] == '-') {
                    nSign = -1;
                } else {
                    pNew->m_nValue *= 10;
                    pNew->m_nValue += m_szTokenText[i] - '0';
                }
            }
            if (nSign == -1)
                pNew->m_nValue = -pNew->m_nValue;
            ModifySRStackReturnTree(pNew);
            return 0;
        }
        case TOKEN_FLOAT:
            pNode = CreateScriptParseTreeNode(NODE_CONSTANT_FLOAT, 0, 0);
            pNode->m_fValue = ParseFloatFromTokenString();
            ModifySRStackReturnTree(pNode);
            return 0;
        case TOKEN_STRING:
            pNode = CreateScriptParseTreeNode(NODE_CONSTANT_STRING, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pNode->m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            ModifySRStackReturnTree(pNode);
            return 0;
        case TOKEN_HEX_NUMBER: {
            // For a hex literal, the scan starts after an "0x" prefix and accepts lowercase hex.
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_CONSTANT_INTEGER, 0, 0);
            m_szTokenText[m_nTokenLength] = 0;
            pNew->m_nValue = 0;
            int nValue = 0;
            for (int i = 2; i < m_nTokenLength; i++) {
                int nChar = m_szTokenText[i];
                int nDigit;
                if (nChar >= '0' && nChar <= '9')
                    nDigit = nChar - '0';
                else
                    nDigit = nChar - 'a' + 10;
                nValue = nValue * 16 + nDigit;
                pNew->m_nValue = nValue;
            }
            ModifySRStackReturnTree(pNew);
            return 0;
        }
        case KEYWORD_OBJECT_SELF:
            pNode = CreateScriptParseTreeNode(NODE_CONSTANT_OBJECT, 0, 0);
            pNode->m_nValue = 0;
            ModifySRStackReturnTree(pNode);
            return 0;
        case KEYWORD_OBJECT_INVALID:
            pNode = CreateScriptParseTreeNode(NODE_CONSTANT_OBJECT, 0, 0);
            pNode->m_nValue = 1;
            ModifySRStackReturnTree(pNode);
            return 0;
        case TOKEN_LEFT_SQUARE: {
            CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_CONSTANT_VECTOR, 0, 0);
            pNew->m_fVectorX = 0;
            pNew->m_fVectorY = 0;
            pNew->m_fVectorZ = 0;
            PushSRStack(GRAMMAR_CONSTANT, 6, 2, pNew);
            return 0;
        }
        default:
            return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;
        }
    }
    if (nSubType == 6 && nLocation == 2) {
        if (m_nTokenType == TOKEN_FLOAT) {
            pNode->m_fVectorX = ParseFloatFromTokenString();
            PushSRStack(GRAMMAR_CONSTANT, 6, 3, pNode);
            return 0;
        }
        if (m_nTokenType != TOKEN_RIGHT_SQUARE) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 6 && nLocation == 3) {
        if (m_nTokenType == TOKEN_COMMA) {
            PushSRStack(GRAMMAR_CONSTANT, 6, 4, pNode);
            return 0;
        }
        if (m_nTokenType != TOKEN_RIGHT_SQUARE) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 6 && nLocation == 4) {
        if (m_nTokenType != TOKEN_FLOAT) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        pNode->m_fVectorY = ParseFloatFromTokenString();
        PushSRStack(GRAMMAR_CONSTANT, 6, 5, pNode);
        return 0;
    }
    if (nSubType == 6 && nLocation == 5) {
        if (m_nTokenType == TOKEN_COMMA) {
            PushSRStack(GRAMMAR_CONSTANT, 6, 6, pNode);
            return 0;
        }
        if (m_nTokenType != TOKEN_RIGHT_SQUARE) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    if (nSubType == 6 && nLocation == 6) {
        if (m_nTokenType != TOKEN_FLOAT) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        pNode->m_fVectorZ = ParseFloatFromTokenString();
        PushSRStack(GRAMMAR_CONSTANT, 6, 7, pNode);
        return 0;
    }
    if (nSubType == 6 && nLocation == 7) {
        if (m_nTokenType != TOKEN_RIGHT_SQUARE) return -SCRIPT_ERROR_PARSING_CONSTANT_VECTOR;
        ModifySRStackReturnTree(pNode);
        return 0;
    }
    break;
        }
    }
    return -SCRIPT_ERROR_FATAL_COMPILER_ERROR;
}

// First leg of the WalkParseTree visitor trio, run before either subtree: pushes the node's type
// onto the type stack, resolves identifier references, and materialises the label
// pseudops whose operands are already known.
int CScriptCompilerInternal::PreVisitGenerateCode(CScriptParseTreeNode* pNode)
{
    switch (pNode->m_nType)
    {
    case NODE_COMPOUND_STATEMENT:
        m_nVarStackRecursionLevel++;
    // fallthrough
    case NODE_STATEMENT:
    case NODE_INTEGER_EXPRESSION:
    case NODE_NON_VOID_EXPRESSION:
        pNode->m_nSavedStackDepth = m_nStackCurrentDepth;
        break;

    case NODE_FUNCTION:
    {
        m_bInFunctionImplementation = 0;
        CScriptParseTreeNode* pRight = pNode->m_pRight;
        if (pRight != 0)
            m_bInFunctionImplementation = 1;
        if (pRight == 0)
            break;

        m_nVarStackRecursionLevel++;
        m_sFunctionImpName = pNode->m_pLeft->m_pLeft->m_sName;
#if KOTOR1
        uint64_t nLen = m_sFunctionImpName.GetLength();
#else
        uint64_t nLen = 0;
        if (m_sFunctionImpName.m_sString != 0)
            nLen = strlen(m_sFunctionImpName.m_sString);
#endif

        int i;
        CScriptCompilerIdListEntry* pEntry = m_pIdentifierList;
        for (i = 0; i <= m_nIdentifierCount; i++)
        {
            if (pEntry->m_nNameLength == static_cast<int>(nLen) &&
                pEntry->m_sName == m_sFunctionImpName)
                break;
            pEntry++;
        }
        if (i < 0 || i > m_nIdentifierCount)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

        pEntry->m_nCodeOffset = m_nBinaryCodeOffset;
        pEntry->m_nCodeEnd = -1;
        pEntry->m_nOutputStart = -1;
        pEntry->m_nOutputEnd = -1;

        if (m_bCompileSymbolicOutput != 0)
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FE_%s:\n",
                    m_sFunctionImpName.CStr());
#if KOTOR1
            uint64_t nNameLen = m_sFunctionImpName.GetLength();
#else
            uint64_t nNameLen = m_sFunctionImpName.m_sString != 0
                                   ? strlen(m_sFunctionImpName.m_sString) : 0;
#endif
            m_nCompiledCodeLocation += static_cast<int>(nNameLen) + 5;
        }
        else
        {
            CExoString sLabel;
            sLabel.Format(const_cast<char*>("FE_%s"), m_sFunctionImpName.CStr());
            AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
        }
        break;
    }
    }

    if (pNode->m_nType == NODE_GLOBAL_VARIABLES)
    {
        m_bInFunctionImplementation = 0;
        CScriptParseTreeNode* pRight = pNode->m_pRight;
        if (pRight != 0)
            m_bInFunctionImplementation = 1;
        if (pRight != 0)
        {
            m_nVarStackRecursionLevel++;
            if (m_bGlobalVariableDefinition != 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            m_bGlobalVariableDefinition = 1;
            m_pIdentifierList[m_nIdentifierCount].m_sName = "#globals";
            m_pIdentifierList[m_nIdentifierCount].m_nNameLength = 8;
            m_pIdentifierList[m_nIdentifierCount].m_nCodeOffset = m_nBinaryCodeOffset;
            m_pIdentifierList[m_nIdentifierCount].m_nOutputStart = -1;
            m_pIdentifierList[m_nIdentifierCount].m_nOutputEnd = -1;

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FE_#globals:\n");
                m_nCompiledCodeLocation += SYMBOLIC_ADDRESS_WIDTH;
            }
            else
            {
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("FE_#globals"));
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
            }
        }
    }

    switch (pNode->m_nType)
    {
    case NODE_ACTION:
    {
        if (pNode->m_pRight == 0)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

        int i;
        for (i = 0; i < m_nIdentifierCount; i++)
        {
            if (m_pIdentifierList[i].m_sName == pNode->m_pRight->m_sName)
                break;
        }
        if (i >= m_nIdentifierCount)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

        pNode->m_pRight->m_nValue = i;
        if (m_nIdentifierBase <= i)
        {
            pNode->m_nValue = 1;
            CExoString sName("");
            int nType = KEYWORD_INT;
            switch (m_pIdentifierList[i].m_nReturnType)
            {
            case TOKEN_INTEGER_IDENTIFIER:
                break;
            case TOKEN_FLOAT_IDENTIFIER:
                nType = KEYWORD_FLOAT;
                break;
            case TOKEN_STRING_IDENTIFIER:
                nType = KEYWORD_STRING;
                break;
            case TOKEN_OBJECT_IDENTIFIER:
                nType = KEYWORD_OBJECT;
                break;
            case TOKEN_VOID_IDENTIFIER:
                break;
            case TOKEN_STRUCTURE_IDENTIFIER:
                sName = m_pIdentifierList[i].m_sReturnStructureName;
                nType = KEYWORD_STRUCT;
                break;
            default:
                if (static_cast<unsigned int>(m_pIdentifierList[i].m_nReturnType - TOKEN_ENGINE_STRUCTURE0_IDENTIFIER) > 9)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
                nType = m_pIdentifierList[i].m_nReturnType - (TOKEN_ENGINE_STRUCTURE0_IDENTIFIER - KEYWORD_ENGINE_STRUCTURE0);
                break;
            }
            AddVariableToStack(nType, sName, 1);
        }
        else
        {
            pNode->m_nValue = 0;
        }

        // In the member-access chain each node's left child is the next `.field`, so the chain's
        // length is the operand's nesting depth.
        int nDepth = -1;
        for (CScriptParseTreeNode* pWalk = pNode; pWalk != 0; pWalk = pWalk->m_pLeft)
            nDepth++;

        K2_ASSUME(pNode != 0);   // the switch above already read pNode->m_nType
        int nIndex = pNode->m_pRight->m_nValue;
        int nHigh = m_pIdentifierList[nIndex].m_nParameterCount;
        int nLow = m_pIdentifierList[nIndex].m_nNonOptionalParameterCount;
        if (nDepth > nHigh || nDepth < nLow)
            return OutputWalkTreeError(-SCRIPT_ERROR_DECLARATION_DOES_NOT_MATCH_PARAMETERS, pNode);
        if (nHigh <= nDepth)
            return 0;

        int j = nHigh - 1;
        if (m_pIdentifierList[nIndex].m_pOptionalParameterFlags[j] != 1)
            return OutputWalkTreeError(-SCRIPT_ERROR_DECLARATION_DOES_NOT_MATCH_PARAMETERS, pNode);
        for (;;)
        {
            int nType = m_pIdentifierList[nIndex].m_pParameterTypes[j];
            if (static_cast<unsigned int>(nType - KEYWORD_INT) >= 4)
            {
                if (nType != KEYWORD_STRUCT || m_pIdentifierList[nIndex].m_pParameterStructureNames[j] != "vector")
                    return OutputWalkTreeError(-SCRIPT_ERROR_TYPE_DOES_NOT_HAVE_AN_OPTIONAL_PARAMETER, pNode);
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_INT)
            {
                int nValue = m_pIdentifierList[nIndex].m_pOptionalParameterIntegerData[j];
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTI %08x\n",
                            nValue);
                    nLen = 29;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nValue >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nValue >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(nValue >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nValue);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_INTEGER;
                m_nStackCurrentDepth++;
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_FLOAT)
            {
                float fValue = m_pIdentifierList[nIndex].m_pOptionalParameterFloatData[j];
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTF %19.9f\n",
                            fValue);
                    nLen = 40;
                }
                else
                {
                    int nBits;
                    memcpy(&nBits, &fValue, sizeof(nBits));
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nBits >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nBits >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(nBits >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nBits);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_FLOAT;
                m_nStackCurrentDepth++;
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_STRUCT &&
                m_pIdentifierList[nIndex].m_pParameterStructureNames[j] == "vector")
            {
                // A vector field is three floats: the index runs from nHigh * 3 - 3 to nHigh * 3 - 1.
                for (int k = -3; k < 0; k++)
                {
                    float fValue = m_pIdentifierList[nIndex].m_pOptionalParameterVectorData[nHigh * 3 + k];
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        PrintBinaryAddress();
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                "CONSTF %19.9f\n", fValue);
                        m_nCompiledCodeLocation += 40;
                    }
                    else
                    {
                        int nBits;
                        memcpy(&nBits, &fValue, sizeof(nBits));
                        m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT;
                        m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                            static_cast<char>(nBits >> 24);
                        m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                            static_cast<char>(nBits >> 16);
                        m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                            static_cast<char>(nBits >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nBits);
                        m_nCompiledCodeLocation += 6;
                    }
                    m_nBinaryCodeOffset += 6;
                    m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_FLOAT;
                    m_nStackCurrentDepth++;
                }
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_STRING)
            {
                CExoString sValue(m_pIdentifierList[nIndex].m_pOptionalParameterStringData[j]);
#if KOTOR1
                uint64_t nStrLen = sValue.GetLength();
#else
                uint64_t nStrLen = sValue.m_sString != 0 ? strlen(sValue.m_sString) : 0;
#endif
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
#ifdef K2_FIX_ENGINE_BUGS
                            "CONSTS %04x %s\n", static_cast<unsigned int>(nStrLen), sValue.CStr());
#else
                            "CONSTS %04x %s\n", nStrLen, sValue.CStr());
#endif
                    m_nCompiledCodeLocation += static_cast<int>(nStrLen) + 26;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STRING;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nStrLen >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nStrLen);
                    for (uint64_t k = 0; k < static_cast<uint64_t>(nStrLen); k++)
                        m_pCompiledCode[m_nCompiledCodeLocation + 4 + k] = sValue.CStr()[k];
                    m_nCompiledCodeLocation += static_cast<int>(nStrLen) + 4;
                }
                m_nBinaryCodeOffset += static_cast<int>(nStrLen) + 4;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_STRING;
                m_nStackCurrentDepth++;
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_OBJECT)
            {
                long long nObject = m_pIdentifierList[nIndex].m_pOptionalParameterObjectData[j];
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
#if KOTOR1
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTO %081x\n",
#else
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTO %08x\n",
#endif
                            static_cast<int>(nObject));
                    nLen = 29;
                }
                else
                {
                    int nBits = static_cast<int>(nObject);
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_OBJECT;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nBits >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nBits >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(nBits >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nBits);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_OBJECT;
                m_nStackCurrentDepth++;
            }

            if (j <= nDepth)
                return 0;
            j--;
            if (m_pIdentifierList[nIndex].m_pOptionalParameterFlags[j] != 1)
                return OutputWalkTreeError(-SCRIPT_ERROR_DECLARATION_DOES_NOT_MATCH_PARAMETERS, pNode);
        }
    }

    case NODE_ACTION_PARAMETER:
    {
        int nLen;
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                    "STORE_STATE %08x %08x %08x\n", NCS_STORE_STATE_RESUME_OFFSET, m_nGlobalVariableSize,
                    m_nStackCurrentDepth * 4 - m_nGlobalVariableSize);
            nLen = 52;
        }
        else
        {
            int nState = m_nGlobalVariableSize;
            int nDelta = m_nStackCurrentDepth * 4 - m_nGlobalVariableSize;
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_STORE_STATE;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_STORE_STATE_RESUME_OFFSET;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nState >> 24);
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nState >> 16);
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nState >> 8);
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nState);
            m_pCompiledCode[m_nCompiledCodeLocation + 6] = static_cast<char>(nDelta >> 24);
            m_pCompiledCode[m_nCompiledCodeLocation + 7] = static_cast<char>(nDelta >> 16);
            m_pCompiledCode[m_nCompiledCodeLocation + 8] = static_cast<char>(nDelta >> 8);
            m_pCompiledCode[m_nCompiledCodeLocation + 9] = static_cast<char>(nDelta);
            nLen = 10;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 10;

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _X%08x\n",
                    m_nCompiledCodeLocation);
            nLen = 28;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            nLen = 6;
        }
        pNode->m_nValue = m_nCompiledCodeLocation;
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 6;
        return 0;
    }

    case NODE_WHILE_BLOCK:
    {
        pNode->m_nSavedEnclosingState = m_nLoopIdentifier;
        m_nLoopIdentifier = m_nCompiledCodeLocation;
        pNode->m_nValue = m_nCompiledCodeLocation;
        if (m_bCompileSymbolicOutput == 0)
            return 0;
        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_W1_%08x:\n",
                m_nCompiledCodeLocation);
        m_nCompiledCodeLocation += 14;
        break;
    }

    case NODE_STRUCTURE_DEFINITION:
        if (m_nStructureDefinition != 0)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
        m_nStructureDefinition = 1;
        return 0;
    }

    if (pNode->m_nType == NODE_CASE)
    {
        int nValue;
        if (pNode->m_pLeft == 0)
            return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
        if (pNode->m_pLeft->m_nType == NODE_CONSTANT_INTEGER)
        {
            nValue = pNode->m_pLeft->m_nValue;
        }
        else
        {
            if (pNode->m_pLeft->m_nType != NODE_NEGATE)
                return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
            if (pNode->m_pLeft->m_pLeft == 0 ||
                pNode->m_pLeft->m_pLeft->m_nType != NODE_CONSTANT_INTEGER)
                return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
            nValue = -pNode->m_pLeft->m_pLeft->m_nValue;
        }

        if (m_bCompileSymbolicOutput != 0)
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_SC_%08x_%08x:\n",
                    nValue, m_nSwitchIdentifier);
            m_nCompiledCodeLocation += 23;
        }
        else
        {
            CExoString sLabel;
            sLabel.Format(const_cast<char*>("_SC_%08x_%08x"), nValue, m_nSwitchIdentifier);
            AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
        }

        if (m_nSwitchStackDepth + 1 != m_nStackCurrentDepth)
            return OutputWalkTreeError(-SCRIPT_ERROR_SKIPPING_DECLARATION_VIA_CASE_STATEMENT_DISALLOWED, pNode);
        return 1;
    }

    if (pNode->m_nType == NODE_DOWHILE_BLOCK)
    {
        pNode->m_nSavedEnclosingState = m_nLoopIdentifier;
        m_nLoopIdentifier = m_nCompiledCodeLocation;
        pNode->m_nValue = m_nCompiledCodeLocation;
        if (m_bCompileSymbolicOutput == 0)
            return 0;
        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_DW1_%08x:\n",
                m_nCompiledCodeLocation);
        m_nCompiledCodeLocation += 15;
    }

    if (pNode->m_nType == NODE_SWITCH_BLOCK)
    {
        m_nSwitchLevel++;
        pNode->m_nCodeStart = m_nSwitchIdentifier;
        pNode->m_nSavedEnclosingState = m_nSwitchStackDepth;
        m_nSwitchIdentifier = m_nCompiledCodeLocation;
        m_nSwitchStackDepth = m_nStackCurrentDepth;
        return 0;
    }

    if (pNode->m_nType == NODE_IF_CHOICE)
    {
        pNode->m_nValue = m_nCompiledCodeLocation;
        if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
            return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
        m_nStackCurrentDepth--;
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _I1_%08x\n",
                        pNode->m_nValue);
                nLen = 29;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
        }
    }

    if (pNode->m_nType == NODE_CONDITIONAL_CHOICE)
    {
        pNode->m_nValue = m_nCompiledCodeLocation;
        if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
            return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
        m_nStackCurrentDepth--;
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _CH1_%08x\n",
                        pNode->m_nValue);
                nLen = 30;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
        }
    }

    if ((pNode->m_nType & ~1) == NODE_PRE_INCREMENT)
    {
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                    pNode->m_nType == NODE_PRE_INCREMENT ? "INCISP %08x\n" : "DECISP %08x\n", 0);
            pNode->m_nCodeStart = m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH;
            m_nCompiledCodeLocation += 29;
        }
        else
        {
            int nLoc = m_nCompiledCodeLocation;
            m_pCompiledCode[m_nCompiledCodeLocation] =
                static_cast<char>(NCS_OP_DECISP + (pNode->m_nType == NODE_PRE_INCREMENT ? 1 : 0));
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = 0;
            pNode->m_nCodeStart = nLoc;
            m_nCompiledCodeLocation += 6;
        }
        m_nBinaryCodeOffset += 6;
    }

    return 0;
}

// Middle leg of the WalkParseTree visitor trio, run after the left subtree: emits the NWScript
// instruction for an operator node, either as real bytes or as its printable listing when
// m_bCompileSymbolicOutput is set.
int CScriptCompilerInternal::InVisitGenerateCode(CScriptParseTreeNode* pNode)
{
    switch (pNode->m_nType)
    {
    case NODE_STRUCTURE_DEFINITION:
        if (m_nStructureDefinition != 1)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
        m_nStructureDefinition = 2;
        if (pNode->m_pLeft == 0 || pNode->m_pLeft->m_nType != NODE_TYPE_STRUCT)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
        for (int i = 0; i < m_nStructureCount; i++)
        {
            if (pNode->m_pLeft->m_sName == m_pStructureList[i].m_sName)
                return OutputWalkTreeError(-SCRIPT_ERROR_STRUCTURE_REDEFINED, pNode);
        }
        m_pStructureList[m_nStructureCount].m_sName = pNode->m_pLeft->m_sName;
        m_pStructureList[m_nStructureCount].m_nFirstField = m_nFieldCount;
        m_pStructureList[m_nStructureCount].m_nLastField = -1;
        m_pStructureList[m_nStructureCount].m_nSize = 0;
        m_nStructureDefinitionFieldStart = m_nFieldCount;
        return 0;

    case NODE_ASSIGNMENT:
        if (pNode->m_pRight == 0 || pNode->m_pLeft == 0 ||
            (pNode->m_pRight->m_nType != NODE_STRUCTURE_PART && pNode->m_pRight->m_nType != NODE_VARIABLE))
            return OutputWalkTreeError(-SCRIPT_ERROR_INVALID_PARAMETERS_FOR_ASSIGNMENT, pNode);
        m_bAssignmentToVariable = 1;
        return 0;

    case NODE_WHILE_BLOCK:
        pNode->m_nCodeStart = m_nCompiledCodeLocation;
        if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
            return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
        m_nStackCurrentDepth--;
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _W2_%08x\n",
                        pNode->m_nValue);
                nLen = 29;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
        }
        break;

    case NODE_SWITCH_BLOCK:
    {
        m_bSwitchHasDefault = 0;
        m_nSwitchLabelCount = 0;
        m_nSwitchLabelCapacity = SWITCH_LABEL_COUNT;
        m_pSwitchLabels = new int[SWITCH_LABEL_COUNT];
        int nResult = TraverseTreeForSwitchLabels(pNode->m_pRight);
        if (nResult < 0)
            return nResult;
        ClearSwitchLabelList();
        return 0;
    }

    case NODE_STATEMENT:
        if (pNode->m_pLeft != 0)
        {
            if (pNode->m_pLeft->m_nType == NODE_DECLARATION)
                return 0;
            if (pNode->m_pLeft->m_nType == NODE_STATEMENT_LIST)
            {
                CScriptParseTreeNode* pMid = pNode->m_pLeft->m_pLeft;
                if (pMid != 0 && pMid->m_nType == NODE_STATEMENT)
                {
                    CScriptParseTreeNode* pLeaf = pMid->m_pLeft;
                    if (pLeaf != 0 && pLeaf->m_nType == NODE_DECLARATION)
                        return 0;
                }
            }
        }
        {
            int nDelta = pNode->m_nSavedStackDepth - m_nStackCurrentDepth;
            pNode->m_nValue = nDelta * 4;
            m_nStackCurrentDepth = pNode->m_nSavedStackDepth;
            if (nDelta == 0)
                return 0;

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n",
                        pNode->m_nValue);
                nLen = 28;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                    static_cast<char>(pNode->m_nValue >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                    static_cast<char>(pNode->m_nValue >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                    static_cast<char>(pNode->m_nValue >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                    static_cast<char>(pNode->m_nValue);
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
        }
        return 0;
    }

    if (pNode->m_nType == NODE_IF_CHOICE)
    {
        pNode->m_nCodeStart = m_nCompiledCodeLocation;
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _I2_%08x\n",
                    pNode->m_nValue);
            m_nCompiledCodeLocation += 30;
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_I1_%08x:\n",
                    pNode->m_nValue);
            m_nCompiledCodeLocation += 14;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            m_nCompiledCodeLocation += 6;
            // The relative branch operand is patched straight into the 6-byte instruction.
            int nDelta = m_nCompiledCodeLocation - pNode->m_nValue;
            m_pCompiledCode[pNode->m_nValue + 2] = static_cast<char>(nDelta >> 24);
            m_pCompiledCode[pNode->m_nValue + 3] = static_cast<char>(nDelta >> 16);
            m_pCompiledCode[pNode->m_nValue + 4] = static_cast<char>(nDelta >> 8);
            m_pCompiledCode[pNode->m_nValue + 5] = static_cast<char>(nDelta);
        }
        m_nBinaryCodeOffset += 6;
    }

    if (pNode->m_nType == NODE_CONDITIONAL_CHOICE)
    {
        pNode->m_nCodeStart = m_nCompiledCodeLocation;
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _CH2_%08x\n",
                    pNode->m_nValue);
            m_nCompiledCodeLocation += 31;
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_CH1_%08x:\n",
                    pNode->m_nValue);
            m_nCompiledCodeLocation += 15;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            m_nCompiledCodeLocation += 6;
            int nDelta = m_nCompiledCodeLocation - pNode->m_nValue;
            m_pCompiledCode[pNode->m_nValue + 2] = static_cast<char>(nDelta >> 24);
            m_pCompiledCode[pNode->m_nValue + 3] = static_cast<char>(nDelta >> 16);
            m_pCompiledCode[pNode->m_nValue + 4] = static_cast<char>(nDelta >> 8);
            m_pCompiledCode[pNode->m_nValue + 5] = static_cast<char>(nDelta);
        }
        m_nBinaryCodeOffset += 6;

        // A structure-typed operand discards its whole expanded field run off the type stack.
        int nStructSize;
        if (pNode->m_nResultType == KEYWORD_STRUCT)
        {
            nStructSize = 0;
            for (int i = 0; i < m_nStructureCount; i++)
            {
                if (pNode->m_sResultStructureName == m_pStructureList[i].m_sName)
                {
                    nStructSize = m_pStructureList[i].m_nSize;
                    break;
                }
            }
        }
        else
        {
            nStructSize = 4;
        }
        m_nStackCurrentDepth -= nStructSize >> 2;
    }

    if (pNode->m_nType == NODE_LOGICAL_AND)
    {
        if (pNode->m_pLeft == 0)
            return 0;

        int nLen;
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CPTOPSP %08x,%04x\n",
                    -4, 4);
            nLen = 35;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPTOPSP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(-4 >> 24);
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(-4 >> 16);
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(-4 >> 8);
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(-4);
            m_pCompiledCode[m_nCompiledCodeLocation + 6] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 7] = 4;
            nLen = 8;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 8;

        {
            CExoString sUnused("");
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_INTEGER;
            m_nStackCurrentDepth++;
        }

        pNode->m_nValue = m_nCompiledCodeLocation;
        if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
            return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
        m_nStackCurrentDepth--;

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _ILA_%08x\n",
                    pNode->m_nValue);
            nLen = 30;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            nLen = 6;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 6;
    }
    else if (pNode->m_nType == NODE_FUNCTION)
    {
        if (m_bInFunctionImplementation == 1)
        {
            pNode->m_nSavedStackDepth = m_nStackCurrentDepth;
            m_nFunctionImpAbortStackPointer = m_nStackCurrentDepth;
        }
        return 0;
    }

    if (pNode->m_nType == NODE_LOGICAL_OR)
    {
        if (pNode->m_pLeft == 0)
            return 0;

        int nLen;
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CPTOPSP %08x,%04x\n",
                    -4, 4);
            nLen = 35;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPTOPSP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(-4 >> 24);
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(-4 >> 16);
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(-4 >> 8);
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(-4);
            m_pCompiledCode[m_nCompiledCodeLocation + 6] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 7] = 4;
            nLen = 8;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 8;

        {
            CExoString sUnused("");
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_INTEGER;
            m_nStackCurrentDepth++;
        }

        if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
            return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
        m_nStackCurrentDepth--;

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _ILO1_%08x\n",
                    m_nCompiledCodeLocation + 66);
            nLen = 31;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = 20;
            nLen = 6;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 6;

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CPTOPSP %08x,%04x\n",
                    -4, 4);
            nLen = 35;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPTOPSP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
            m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(-4 >> 24);
            m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(-4 >> 16);
            m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(-4 >> 8);
            m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(-4);
            m_pCompiledCode[m_nCompiledCodeLocation + 6] = 0;
            m_pCompiledCode[m_nCompiledCodeLocation + 7] = 4;
            nLen = 8;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 8;

        pNode->m_nValue = m_nCompiledCodeLocation;

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _ILO2_%08x\n",
                    pNode->m_nValue);
            m_nCompiledCodeLocation += 32;
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_ILO1_%08x:\n",
                    pNode->m_nValue);
            nLen = 16;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            nLen = 6;
        }
        m_nCompiledCodeLocation += nLen;
        m_nBinaryCodeOffset += 6;
    }

    if (pNode->m_nType == NODE_DOWHILE_BLOCK)
    {
        if (m_bCompileSymbolicOutput != 0)
        {
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_CN_%08x:\n",
                    static_cast<int>(m_nLoopIdentifier));
            m_nCompiledCodeLocation += 14;
            return 0;
        }
        CExoString sLabel;
        sLabel.Format(const_cast<char*>("_CN_%08x"), static_cast<int>(m_nLoopIdentifier));
        AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
        return 0;
    }

    if (pNode->m_nType == NODE_STRUCTURE_PART)
    {
        m_bInStructurePart = 1;
        return 0;
    }

    return 0;
}

// Last leg of the WalkParseTree visitor trio, run after both subtrees: the operators that need
// their operands' types -- arithmetic, comparison, equality, shifts, conversions -- and the
// dispatch into the shared finish and error tails.
int CScriptCompilerInternal::PostVisitGenerateCode(CScriptParseTreeNode* pNode)
{
    int nResult = 0;

    switch (pNode->m_nType)
    {
    case NODE_TYPE_INT: m_nVarStackVariableType = KEYWORD_INT; break;
    case NODE_TYPE_FLOAT: m_nVarStackVariableType = KEYWORD_FLOAT; break;
    case NODE_TYPE_STRING: m_nVarStackVariableType = KEYWORD_STRING; break;
    case NODE_TYPE_OBJECT: m_nVarStackVariableType = KEYWORD_OBJECT; break;

    case NODE_TYPE_VOID: break;

    case NODE_ASSIGNMENT:
    {
        CScriptParseTreeNode* pRight = pNode->m_pRight;
        CScriptParseTreeNode* pLeft = pNode->m_pLeft;
        if (pRight == 0 || pLeft == 0 || pRight->m_nResultType != pLeft->m_nResultType)
            return OutputWalkTreeError(-SCRIPT_ERROR_MISMATCHED_TYPES, pNode);

        if (pRight->m_nResultType == KEYWORD_STRUCT)
        {
            pNode->m_nResultType = KEYWORD_STRUCT;
        }
        else
        {
            if (pRight->m_sResultStructureName != pLeft->m_sResultStructureName)
                return OutputWalkTreeError(-SCRIPT_ERROR_MISMATCHED_TYPES, pNode);
            pNode->m_nResultType = pNode->m_pRight->m_nResultType;
        }
        if (pNode->m_nResultType == KEYWORD_STRUCT)
            pNode->m_sResultStructureName = pLeft->m_sResultStructureName;

        int nValue0 = pNode->m_pRight->m_nValue;
        int nStack = m_nStackCurrentDepth;
        int nSize;
        if (pNode->m_nResultType == KEYWORD_STRUCT)
        {
            nSize = 0;
            for (int i = 0; i < m_nStructureCount; i++)
            {
                if (m_pStructureList[i].m_sName == pNode->m_sResultStructureName)
                {
                    nSize = m_pStructureList[i].m_nSize;
                    goto LAB_7f08e0;
                }
            }
        }
        else
        {
            nSize = 4;
        }

    LAB_7f08e0:
        {
            int nValue = nValue0 - nStack * 4;
            int nDiff = pNode->m_pRight->m_nValue - m_nGlobalVariableSize;
            int bBP;
            if (pNode->m_pRight->m_nValue < m_nGlobalVariableSize)
            {
                bBP = m_bGlobalVariableDefinition != 0;
                if (!bBP)
                    nValue = nDiff;
            }
            else
            {
                bBP = 1;
            }

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                        m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                        bBP ? "CPDOWNSP %08x,%04x\n" : "CPDOWNBP %08x,%04x\n",
                        nValue, nSize);
                nLen = 36;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] =
                    static_cast<char>(bBP ? NCS_OP_CPDOWNSP : NCS_OP_CPDOWNBP);
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nValue >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nValue >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nValue >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nValue);
                m_pCompiledCode[m_nCompiledCodeLocation + 6] = static_cast<char>(nSize >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 7] = static_cast<char>(nSize);
                nLen = 8;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 8;
            m_bAssignmentToVariable = 0;
        }
        break;
    }

    default:
        if (pNode->m_nType >= NODE_TYPE_ENGINE_STRUCTURE0 && pNode->m_nType <= NODE_TYPE_ENGINE_STRUCTURE9)
        {
            m_nVarStackVariableType = pNode->m_nType + 3;
            break;
        }

        switch (pNode->m_nType)
        {

        case NODE_IF_CONDITION: case NODE_WHILE_CONDITION: case NODE_DOWHILE_CONDITION: case NODE_CONDITIONAL_CONDITION: case NODE_SWITCH_CONDITION:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft != 0)
            {
                pNode->m_nResultType = pLeft->m_nResultType;
                if (pLeft->m_nResultType == KEYWORD_STRUCT)
                    pNode->m_sResultStructureName = pLeft->m_sResultStructureName;
            }
            if (pNode->m_nType == NODE_SWITCH_CONDITION)
            {
                if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
                    return OutputWalkTreeError(-SCRIPT_ERROR_SWITCH_MUST_EVALUATE_TO_AN_INTEGER, pNode);
            }
            return 0;
        }

        case NODE_VARIABLE_LIST: case NODE_STATEMENT_LIST:
            pNode->m_nResultType = 0;
            break;

        case NODE_CONDITIONAL_BLOCK:
            pNode->m_nResultType = pNode->m_pRight->m_nResultType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = pNode->m_pRight->m_sResultStructureName;
            break;

        case NODE_ACTION_ARG_LIST:
        {
            // Same body as the right-operand type case above, plus a null guard; the two share one
            // block.
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return 0;
            pNode->m_nResultType = pRight->m_nResultType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = pRight->m_sResultStructureName;
            break;
        }


        case NODE_GLOBAL_VARIABLES:
        {
            m_bGlobalVariableDefinition = 0;

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "SAVEBP\n");
                nLen = 20;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_SAVEBP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;

            if (m_bCompileConditionalFile == 1)
            {
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDI\n");
                    nLen = 20;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RSADD;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                    nLen = 2;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                if (m_bCompileConditionalFile != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                            "JSR FE_StartingConditional\n");
                    m_nCompiledCodeLocation += 40;
                }
                else
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_main\n");
                    m_nCompiledCodeLocation += 25;
                }
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JSR;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;

                CExoString sEntry;
                if (m_bCompileConditionalFile != 0)
                    sEntry.Format(const_cast<char*>("FE_StartingConditional"));
                else
                    sEntry.Format(const_cast<char*>("FE_main"));

                AddSymbolToQueryList(sEntry, m_nCompiledCodeLocation + 2);
                m_nCompiledCodeLocation += 6;
            }
            m_nBinaryCodeOffset += 6;

            if (m_bCompileConditionalFile == 1)
            {
                int nOffset = -12 - m_nGlobalVariableSize;
                int nSize = 4;
                if (m_nFunctionImpReturnType == KEYWORD_STRUCT)
                    nSize = GetStructureSize(m_sFunctionImpReturnStructureName);

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                            "CPDOWNSP %08x,%04x\n", nOffset, nSize);
                    nLen = 36;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPDOWNSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nOffset >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nOffset >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nOffset >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nOffset);
                    m_pCompiledCode[m_nCompiledCodeLocation + 6] = static_cast<char>(nSize >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 7] = static_cast<char>(nSize);
                    nLen = 8;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 8;

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n", -4);
                    nLen = 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(-4 >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(-4 >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(-4 >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(-4);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RESTOREBP\n");
                nLen = 23;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RESTOREBP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;

            if (m_nGlobalVariableSize != 0)
            {
                int nFrame = -m_nGlobalVariableSize;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n", nFrame);
                    nLen = 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nFrame >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nFrame >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nFrame >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nFrame);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RET\n");
                nLen = 17;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RETN;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;

            m_pIdentifierList[m_nIdentifierCount].m_nCodeEnd = m_nBinaryCodeOffset;
            if (m_nIdentifierCount++ >= IDENTIFIER_LIST_COUNT - 1)
                return OutputWalkTreeError(-SCRIPT_ERROR_IDENTIFIER_LIST_FULL, pNode);
            break;
        }


        case NODE_CONDITIONAL_CHOICE:
        {
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_CH2_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 15;
            }
            else
            {
                int nDelta = m_nCompiledCodeLocation - pNode->m_nCodeStart;
                m_pCompiledCode[pNode->m_nCodeStart + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nCodeStart + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nCodeStart + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nCodeStart + 5] = static_cast<char>(nDelta);
            }

            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            int nLeftTag = pLeft->m_nResultType;
            if (pRight->m_nResultType != nLeftTag)
                return OutputWalkTreeError(-SCRIPT_ERROR_CONDITIONAL_MUST_HAVE_MATCHING_RETURN_TYPES, pNode);
            if (nLeftTag != KEYWORD_STRUCT)
            {
                pNode->m_nResultType = nLeftTag;
                return 0;
            }
            if (pLeft->m_sResultStructureName != pRight->m_sResultStructureName)
                return OutputWalkTreeError(-SCRIPT_ERROR_CONDITIONAL_MUST_HAVE_MATCHING_RETURN_TYPES, pNode);

            pNode->m_nResultType = pLeft->m_nResultType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = pLeft->m_sResultStructureName;
            break;
        }

        case NODE_TYPE_STRUCT:
            if (m_nStructureDefinition == 1)
                return 0;
            m_nVarStackVariableType = KEYWORD_STRUCT;
            m_sVarStackVariableTypeName = pNode->m_sName;
            break;


        case NODE_STRUCTURE_DEFINITION:
        {
            if (m_nStructureDefinition != 2)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            m_pStructureList[m_nStructureCount].m_nLastField = m_nFieldCount - 1;
            if (m_pStructureList[m_nStructureCount].m_nFirstField >= m_nFieldCount)
            {
                m_pStructureList[m_nStructureCount].m_nSize = 0;
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            }

            int nSize = 0;
            int i = m_pStructureList[m_nStructureCount].m_nFirstField;
            for (;;)
            {
                m_pStructureFieldList[i].m_nOffset = nSize;
                if (m_pStructureFieldList[i].m_nType == KEYWORD_STRUCT)
                {
                    for (int j = 0; j < m_nStructureCount; j++)
                    {
                        if (m_pStructureFieldList[i].m_sStructureName == m_pStructureList[j].m_sName)
                            nSize += m_pStructureList[j].m_nSize;
                    }
                }
                else
                {
                    nSize += 4;
                }
                if (i >= m_pStructureList[m_nStructureCount].m_nLastField)
                    break;
                i++;
            }
            m_pStructureList[m_nStructureCount].m_nSize = nSize;
            if (nSize == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            m_nStructureDefinition = 0;
            m_nStructureCount++;
            return 0;
        }


        case NODE_FUNCTION_PARAMETER_NAME:
        {
            m_nVarStackVariableType = DECLARATION_TYPE_NONE;
            if (m_bInFunctionImplementation == 0)
                return 0;

            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            for (int i = m_nVarStackTop; i >= 0; i--)
            {
                if (m_pVarStackList[i].m_sName == pNode->m_sName
                    && m_pVarStackList[i].m_nRecursionLevel == m_nVarStackRecursionLevel)
                    return OutputWalkTreeError(-SCRIPT_ERROR_VARIABLE_ALREADY_USED_WITHIN_SCOPE, pNode);
            }

            CExoString s("");
            int nType = KEYWORD_INT;
            switch (pRight->m_nType)
            {
            case NODE_TYPE_INT: break;
            case NODE_TYPE_FLOAT: nType = KEYWORD_FLOAT; break;
            case NODE_TYPE_STRING: nType = KEYWORD_STRING; break;
            case NODE_TYPE_OBJECT: nType = KEYWORD_OBJECT; break;
            case NODE_TYPE_STRUCT: s = pRight->m_sName; nType = KEYWORD_STRUCT; break;
            case NODE_TYPE_VOID: return 0;
            default:
                if (static_cast<unsigned>(pRight->m_nType - NODE_TYPE_ENGINE_STRUCTURE0) > 9)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
                nType = pRight->m_nType + (KEYWORD_ENGINE_STRUCTURE0 - NODE_TYPE_ENGINE_STRUCTURE0);
                break;
            }

            m_nVarStackTop++;
            if (m_bGlobalVariableDefinition != 0)
            {
                m_nGlobalVariableCount++;
                if (nType == KEYWORD_STRUCT)
                    m_nGlobalVariableSize += GetStructureSize(s);
                else
                    m_nGlobalVariableSize += 4;
            }

            m_pVarStackList[m_nVarStackTop].m_sName = pNode->m_sName;
            m_pVarStackList[m_nVarStackTop].m_nType = nType;
            if (nType == KEYWORD_STRUCT)
                m_pVarStackList[m_nVarStackTop].m_sStructureName = s;
            m_pVarStackList[m_nVarStackTop].m_nRecursionLevel = m_nVarStackRecursionLevel;
            m_pVarStackList[m_nVarStackTop].m_nRunTimeLocation = m_nStackCurrentDepth * 4;

            AddVariableToStack(nType, s, 0);
            return 0;
        }


        case NODE_FUNCTION:
        {
            if (m_bInFunctionImplementation == 0)
                return 0;

            int nLimit = m_nGlobalVariableCount;
            if (m_nFunctionImpReturnType != KEYWORD_VOID)
                nLimit++;

            while (m_nVarStackTop >= nLimit)
            {
                if (m_pVarStackList[m_nVarStackTop].m_nType == KEYWORD_STRUCT)
                    m_nStackCurrentDepth -= GetStructureSize(m_pVarStackList[m_nVarStackTop].m_sStructureName) >> 2;
                else
                    m_nStackCurrentDepth--;
                m_nVarStackTop--;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FX_%s:\n",
                        m_sFunctionImpName.CStr());
#if KOTOR1
                uint64_t nNameLen = m_sFunctionImpName.GetLength();
#else
                uint64_t nNameLen = 0;
                if (m_sFunctionImpName.m_sString != 0)
                    nNameLen = strlen(m_sFunctionImpName.m_sString);
#endif
                m_nCompiledCodeLocation += static_cast<int>(nNameLen) + 5;
            }
            else
            {
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("FX_%s"), m_sFunctionImpName.CStr());
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
            }

            int nSlots = m_nStackCurrentDepth - pNode->m_nSavedStackDepth;
            if (nSlots != 0)
            {
                int nOperand = nSlots * 4;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n",
                            nOperand);
                    m_nCompiledCodeLocation += 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nOperand >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nOperand >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(nOperand >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                        static_cast<char>(nOperand);
                    m_nCompiledCodeLocation += 6;
                }
                m_nBinaryCodeOffset += 6;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RET\n");
                m_nCompiledCodeLocation += 17;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RETN;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                m_nCompiledCodeLocation += 2;
            }
            m_nBinaryCodeOffset += 2;

            if (m_nFunctionImpReturnType != KEYWORD_VOID)
            {
                if (!FoundReturnStatementOnAllBranches(pNode->m_pRight))
                    return OutputWalkTreeError(-SCRIPT_ERROR_NOT_ALL_CONTROL_PATHS_RETURN_A_VALUE, pNode);
            }

            if (nLimit != m_nGlobalVariableCount)
            {
                if (m_pVarStackList[m_nVarStackTop].m_nType == KEYWORD_STRUCT)
                    m_nStackCurrentDepth -= GetStructureSize(m_pVarStackList[m_nVarStackTop].m_sStructureName) >> 2;
                else
                    m_nStackCurrentDepth--;
                m_nVarStackTop--;
            }

            m_nVarStackRecursionLevel--;

            int i = GetIdentifierByName(m_sFunctionImpName);
            if (i < 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            m_pIdentifierList[i].m_nCodeEnd = m_nBinaryCodeOffset;
            m_bInFunctionImplementation = 0;
            return 0;
        }

        case NODE_DECLARATION:
            m_nVarStackVariableType = DECLARATION_TYPE_NONE;
            pNode->m_nResultType = 0;
            break;


        case NODE_ACTION_ID:
        {
            pNode->m_nValue = GetIdentifierByName(pNode->m_sName);
            if (pNode->m_nValue < 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            break;
        }


        case NODE_CONSTANT_INTEGER:
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTI %08x\n",
                        pNode->m_nValue);
                nLen = 29;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                    static_cast<char>(pNode->m_nValue >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                    static_cast<char>(pNode->m_nValue >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                    static_cast<char>(pNode->m_nValue >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                    static_cast<char>(pNode->m_nValue);
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
            pNode->m_nResultType = KEYWORD_INT;
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_INTEGER;
            m_nStackCurrentDepth++;
            break;
        }


        case NODE_CONSTANT_FLOAT:
        {
            int* pnBits = reinterpret_cast<int*>(&pNode->m_fValue);
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTF %19.9f\n",
                        pNode->m_fValue);
                nLen = 40;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                    static_cast<char>(*pnBits >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                    static_cast<char>(*pnBits >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                    static_cast<char>(*pnBits >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(*pnBits);
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
            pNode->m_nResultType = KEYWORD_FLOAT;
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_FLOAT;
            m_nStackCurrentDepth++;
            break;
        }


        case NODE_CONSTANT_STRING:
        {
#if KOTOR1
            int nStrLen = pNode->m_sName.GetLength();
#else
            int nStrLen = pNode->m_sName.m_sString
                              ? static_cast<int>(strlen(pNode->m_sName.m_sString)) : 0;
#endif
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTS %04x %s\n",
                        nStrLen, pNode->m_sName.CStr());
                m_nCompiledCodeLocation += nStrLen + 26;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STRING;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                    static_cast<char>(nStrLen >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                    static_cast<char>(nStrLen);
                if (nStrLen > 0)
                {
                    for (uint64_t k = 0; k < static_cast<uint64_t>(nStrLen); k++)
                        m_pCompiledCode[m_nCompiledCodeLocation + 4 + k] =
                            pNode->m_sName.CStr()[k];
                }
                m_nCompiledCodeLocation += nStrLen + 4;
            }
            m_nBinaryCodeOffset += nStrLen + 4;
            pNode->m_nResultType = KEYWORD_STRING;
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_STRING;
            m_nStackCurrentDepth++;
            break;
        }


        case NODE_CONSTANT_OBJECT:
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "CONSTO %08x\n",
                        pNode->m_nValue);
                nLen = 29;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_OBJECT;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                    static_cast<char>(pNode->m_nValue >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                    static_cast<char>(pNode->m_nValue >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                    static_cast<char>(pNode->m_nValue >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                    static_cast<char>(pNode->m_nValue);
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
            pNode->m_nResultType = KEYWORD_OBJECT;
            m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_OBJECT;
            m_nStackCurrentDepth++;
            break;
        }


        case NODE_CONSTANT_VECTOR:
        {
            int* pnComponent = reinterpret_cast<int*>(&pNode->m_fVectorX);
            for (int i = 0; i < 3; i++)
            {
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                            "CONSTF %19.9f\n", pNode->m_fValue);
                    nLen = 40;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(pnComponent[i] >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(pnComponent[i] >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(pnComponent[i] >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                        static_cast<char>(pnComponent[i]);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_FLOAT;
                m_nStackCurrentDepth++;
            }
            pNode->m_nResultType = KEYWORD_STRUCT;
            pNode->m_sResultStructureName = "vector";
            break;
        }


        case NODE_ACTION_PARAMETER:
        {
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RETN\n");
                m_nCompiledCodeLocation += 18;
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_X%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 12;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RETN;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                m_nCompiledCodeLocation += 2;

                int nDelta = m_nCompiledCodeLocation - pNode->m_nValue;
                m_pCompiledCode[pNode->m_nValue + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nValue + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nValue + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nValue + 5] = static_cast<char>(nDelta);
            }

            m_nBinaryCodeOffset += 2;
            if (pNode->m_pLeft == 0)
                return 0;
            pNode->m_nResultType = KEYWORD_ACTION;
            break;
        }


        case NODE_NEGATE:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            if (pLeft->m_nResultType == KEYWORD_FLOAT)
            {
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEGF\n");
                    nLen = 18;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_NEG;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT;
                    nLen = 2;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                pNode->m_nResultType = KEYWORD_FLOAT;
                break;
            }

            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEGI\n");
                nLen = 18;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_NEG;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_WHILE_BLOCK:
        {
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _W1_%08x\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 30;
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_W2_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 14;
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_BR_%08x:\n",
                        m_nLoopIdentifier);
                m_nCompiledCodeLocation += 14;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;

                int nDelta = pNode->m_nValue - m_nCompiledCodeLocation;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nDelta);

                m_nCompiledCodeLocation += 6;

                int nDelta2 = m_nCompiledCodeLocation - pNode->m_nCodeStart;
                m_pCompiledCode[pNode->m_nCodeStart + 2] = static_cast<char>(nDelta2 >> 24);
                m_pCompiledCode[pNode->m_nCodeStart + 3] = static_cast<char>(nDelta2 >> 16);
                m_pCompiledCode[pNode->m_nCodeStart + 4] = static_cast<char>(nDelta2 >> 8);
                m_pCompiledCode[pNode->m_nCodeStart + 5] = static_cast<char>(nDelta2);

                CExoString sLabel;
                sLabel.Format(const_cast<char*>("_BR_%08x"), static_cast<int>(m_nLoopIdentifier));
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
            }

            m_nBinaryCodeOffset += 6;
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            pNode->m_nResultType = pLeft->m_nResultType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = pLeft->m_sResultStructureName;
            m_nLoopIdentifier = pNode->m_nSavedEnclosingState;
            break;
        }


        case NODE_STRUCTURE_PART:
        {
            m_bInStructurePart = 0;
            if (pNode->m_pLeft == 0 || pNode->m_pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pNode->m_pLeft->m_nResultType != KEYWORD_STRUCT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LEFT_OF_STRUCTURE_PART_NOT_STRUCTURE, pNode);

            int nField = GetStructureField(pNode->m_pLeft->m_sResultStructureName,
                                           pNode->m_pRight->m_sName);
            if (nField < 0)
                return OutputWalkTreeError(nField, pNode);

            pNode->m_nResultType = m_pStructureFieldList[nField].m_nType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = m_pStructureFieldList[nField].m_sStructureName;
            pNode->m_nValue = pNode->m_pLeft->m_nValue + m_pStructureFieldList[nField].m_nOffset;
            if (m_bAssignmentToVariable != 0)
                return 0;

            int nStructSize = GetStructureSize(pNode->m_pLeft->m_sResultStructureName);
            int nFieldSize;
            if (m_pStructureFieldList[nField].m_nType != KEYWORD_STRUCT)
                nFieldSize = 4;
            else
                nFieldSize = GetStructureSize(pNode->m_pRight->m_sResultStructureName);
            int nFieldOffset = m_pStructureFieldList[nField].m_nOffset;

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                        "DESTRUCT %04x,%04x,%04x\n", nStructSize, nFieldOffset, nFieldSize);
                nLen = 37;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_DESTRUCT;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nStructSize >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nStructSize);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nFieldOffset >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nFieldOffset);
                m_pCompiledCode[m_nCompiledCodeLocation + 6] = static_cast<char>(nFieldSize >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 7] = static_cast<char>(nFieldSize);
                nLen = 8;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 8;

            int nOffset4 = nFieldOffset >> 2;
            int nStruct4 = nStructSize >> 2;
            int nField4 = nFieldSize >> 2;
            int nFieldEnd4 = nOffset4 + nField4;
            int nShrink = nFieldEnd4 - nStruct4;
            if (nShrink < 0)
                m_nStackCurrentDepth += nShrink;
            for (int i = m_nStackCurrentDepth - nFieldEnd4; i < m_nStackCurrentDepth - nOffset4; i++)
                m_aStackTypes[i] = m_aStackTypes[nOffset4 + i];
            m_nStackCurrentDepth -= nOffset4;
            return 0;
        }


        case NODE_ONES_COMPLEMENT:
        {
            if (pNode->m_pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pNode->m_pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "COMPI\n");
                nLen = 19;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_COMP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_RETURN:
        {
            if (m_bInFunctionImplementation == 0)
                return 0;

            int nTag = m_nFunctionImpReturnType;
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (nTag == KEYWORD_VOID)
            {
                if (pRight != 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
            }
            else
            {
                if (pRight == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);

                switch (nTag)
                {
                case KEYWORD_INT:
                    if (pRight->m_nResultType != KEYWORD_INT)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_FLOAT:
                    if (pRight->m_nResultType != KEYWORD_FLOAT)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_STRING:
                    if (pRight->m_nResultType != KEYWORD_STRING)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_OBJECT:
                    if (pRight->m_nResultType != KEYWORD_OBJECT)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_STRUCT:
                    if (pRight->m_nResultType != KEYWORD_STRUCT || m_sFunctionImpReturnStructureName != pRight->m_sResultStructureName)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 1:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 1)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 2:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 2)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 3:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 3)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 4:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 4)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 5:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 5)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 6:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 6)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 7:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 7)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 8:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 8)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                case KEYWORD_ENGINE_STRUCTURE0 + 9:
                    if (pRight->m_nResultType != KEYWORD_ENGINE_STRUCTURE0 + 9)
                        return OutputWalkTreeError(-SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED, pNode);
                    break;
                default:
                    if (nTag == KEYWORD_VOID)
                        goto emitMovSp;
                    break;
                }

                // Moves the frame pointer down past the declaration's slot.
                {
                    int nValue = m_nGlobalVariableSize - m_nStackCurrentDepth * 4;
                    int nSize = 4;
                    if (nTag == KEYWORD_STRUCT)
                    {
                        nSize = 0;
                        for (int i = 0; i < m_nStructureCount; i++)
                        {
                            if (m_sFunctionImpReturnStructureName == m_pStructureList[i].m_sName)
                            {
                                nSize = m_pStructureList[i].m_nSize;
                                break;
                            }
                        }
                    }

                    int nLen;
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        PrintBinaryAddress();
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                "CPDOWNSP %08x,%04x\n", nValue, nSize);
                        nLen = 36;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPDOWNSP;
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                        m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                            static_cast<char>(nValue >> 24);
                        m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                            static_cast<char>(nValue >> 16);
                        m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                            static_cast<char>(nValue >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                            static_cast<char>(nValue);
                        m_pCompiledCode[m_nCompiledCodeLocation + 6] =
                            static_cast<char>(nSize >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 7] =
                            static_cast<char>(nSize);
                        nLen = 8;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 8;
                }
            }

        emitMovSp:
            pNode->m_nValue = (m_nFunctionImpAbortStackPointer - m_nStackCurrentDepth) * 4;
            if (pNode->m_nValue != 0)
            {
                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n",
                            pNode->m_nValue);
                    m_nCompiledCodeLocation += 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(pNode->m_nValue >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(pNode->m_nValue >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(pNode->m_nValue >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                        static_cast<char>(pNode->m_nValue);
                    m_nCompiledCodeLocation += 6;
                }
                m_nBinaryCodeOffset += 6;
            }

            // The forward jump to the enclosing function's exit label.
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP FX_%s\n",
                        m_sFunctionImpName.CStr());
#if KOTOR1
                uint64_t nNameLen = m_sFunctionImpName.GetLength();
#else
                uint64_t nNameLen = 0;
                if (m_sFunctionImpName.m_sString != 0)
                    nNameLen = strlen(m_sFunctionImpName.m_sString);
#endif
                m_nCompiledCodeLocation += static_cast<int>(nNameLen) + 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("FX_%s"), m_sFunctionImpName.CStr());
                AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
                m_nCompiledCodeLocation += 6;
            }
            m_nBinaryCodeOffset += 6;
            return 0;
        }

        case NODE_BOOLEAN_NOT:
        {
            if (pNode->m_pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pNode->m_pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NOTI\n");
                nLen = 18;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_NOTI;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_GREATER_EQUAL: case NODE_GREATER_THAN: case NODE_LESS_THAN: case NODE_LESS_EQUAL:
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                m_nCompiledCodeLocation += SYMBOLIC_ADDRESS_WIDTH;

                int nMnemonic;
                if (pNode->m_nType == NODE_GREATER_EQUAL)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "GEQ");
                    nMnemonic = 3;
                }
                else if (pNode->m_nType == NODE_GREATER_THAN)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "GT");
                    nMnemonic = 2;
                }
                else if (pNode->m_nType == NODE_LESS_THAN)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "LT");
                    nMnemonic = 2;
                }
                else
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "LEQ");
                    nMnemonic = 3;
                }
                m_nCompiledCodeLocation += nMnemonic;

                if (pNode->m_pLeft == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            }
            else
            {
                if (pNode->m_nType == NODE_GREATER_EQUAL)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_GEQ;
                else if (pNode->m_nType == NODE_GREATER_THAN)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_GT;
                else if (pNode->m_nType == NODE_LESS_THAN)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_LT;
                else
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_LEQ;

                if (pNode->m_pLeft == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            }

            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            int nLeftTag = pNode->m_pLeft->m_nResultType;
            if (nLeftTag == KEYWORD_INT)
            {
                if (pRight->m_nResultType != KEYWORD_INT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_COMPARISON_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "II\n");
                    nLen = 3;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                    nLen = 2;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 1;
                pNode->m_nResultType = KEYWORD_INT;
            }
            else if (nLeftTag == KEYWORD_FLOAT)
            {
                if (pRight->m_nResultType != KEYWORD_FLOAT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_COMPARISON_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FF\n");
                    nLen = 3;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT_FLOAT;
                    nLen = 2;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 2;
                AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                pNode->m_nResultType = KEYWORD_INT;
            }
            else
            {
                return OutputWalkTreeError(-SCRIPT_ERROR_COMPARISON_TEST_HAS_INVALID_OPERANDS, pNode);
            }
            break;
        }


        case NODE_SHIFT_LEFT: case NODE_SHIFT_RIGHT: case NODE_UNSIGNED_SHIFT_RIGHT:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_SHIFT_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_SHIFT_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                m_nCompiledCodeLocation += SYMBOLIC_ADDRESS_WIDTH;
                if (pNode->m_nType == NODE_SHIFT_LEFT)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "SHLEFT\n");
                    nLen = 7;
                }
                else if (pNode->m_nType == NODE_GREATER_THAN)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "SHRIGHT\n");
                    nLen = 8;
                }
                else
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "USHRIGHTII\n");
                    nLen = 11;
                }
            }
            else
            {
                if (pNode->m_nType == NODE_SHIFT_LEFT)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_SHLEFTII;
                else if (pNode->m_nType == NODE_SHIFT_RIGHT)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_SHRIGHTII;
                else
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_USHRIGHTII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            // Shared finish, also reached by the comparison arm above.
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 2;
            AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_ADD: case NODE_SUBTRACT: case NODE_MULTIPLY: case NODE_DIVIDE:
        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                m_nCompiledCodeLocation += SYMBOLIC_ADDRESS_WIDTH;

                if (pNode->m_nType == NODE_ADD)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "ADD");
                    nLen = 3;
                }
                else if (pNode->m_nType == NODE_SUBTRACT)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "SUB");
                    nLen = 3;
                }
                else if (pNode->m_nType == NODE_MULTIPLY)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "MUL");
                    nLen = 3;
                }
                else
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "DIV");
                    nLen = 3;
                }
                m_nCompiledCodeLocation += nLen;

                if (pNode->m_pLeft == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            }
            else
            {
                if (pNode->m_nType == NODE_ADD)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_ADD;
                else if (pNode->m_nType == NODE_SUBTRACT)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_SUB;
                else if (pNode->m_nType == NODE_MULTIPLY)
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MUL;
                else
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_DIV;

                if (pNode->m_pLeft == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            }

            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            if (pNode->m_pLeft->m_nResultType == KEYWORD_INT)
            {
                // The right operand must carry the same tag.
                if (pRight->m_nResultType == KEYWORD_INT)
                {
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "II\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_INT;
                    break;
                }
                if (pRight->m_nResultType == KEYWORD_FLOAT)
                {
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "IF\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_FLOAT;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_FLOAT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_FLOAT;
                    break;
                }
            }
            // Left operand is a float.
            else if (pNode->m_pLeft->m_nResultType == KEYWORD_FLOAT)
            {
                if (pRight->m_nResultType == KEYWORD_INT)
                {
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FI\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT_INTEGER;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_FLOAT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_FLOAT;
                    break;
                }
                if (pRight->m_nResultType == KEYWORD_FLOAT)
                {
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FF\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT_FLOAT;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_FLOAT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_FLOAT;
                    break;
                }
            }
            // A string left operand is only valid for ADD.
            else if (pNode->m_pLeft->m_nResultType == KEYWORD_STRING)
            {
                if (pRight->m_nResultType == KEYWORD_STRING)
                {
                    if (pNode->m_nType != NODE_ADD)
                        return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "SS\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STRING_STRING;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_STRING, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_STRING;
                    break;
                }
            }
            // A vector left operand is only valid for the same named struct.
            else if (pNode->m_pLeft->m_nResultType == KEYWORD_STRUCT)
            {
                if (pRight->m_nResultType == KEYWORD_STRUCT &&
                    pNode->m_pLeft->m_sResultStructureName == "vector" &&
                    pNode->m_pRight->m_sResultStructureName == pNode->m_pLeft->m_sResultStructureName)
                {
                    if (pNode->m_nType == NODE_MULTIPLY || pNode->m_nType == NODE_DIVIDE)
                        return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "VV\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_VECTOR_VECTOR;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 6;
                    AddStructureToStack(CExoString("vector"), 0);
                    pNode->m_nResultType = KEYWORD_STRUCT;
                    pNode->m_sResultStructureName = pNode->m_pLeft->m_sResultStructureName;
                    break;
                }
            }

            // Cross cases, vector op scalar and scalar op vector.
            if (pNode->m_pLeft->m_nResultType == KEYWORD_STRUCT)
            {
                if (pRight->m_nResultType != KEYWORD_FLOAT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

                if (pNode->m_pLeft->m_sResultStructureName == "vector")
                {
                    if (pNode->m_nType == NODE_ADD || pNode->m_nType == NODE_SUBTRACT)
                        return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "VF\n");
                        nLen = 3;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_VECTOR_FLOAT;
                        nLen = 2;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 4;
                    AddStructureToStack(CExoString("vector"), 0);
                    pNode->m_nResultType = KEYWORD_STRUCT;
                    pNode->m_sResultStructureName = pNode->m_pLeft->m_sResultStructureName;
                    break;
                }
            }

            if (pNode->m_pLeft->m_nResultType != KEYWORD_FLOAT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_STRUCT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pNode->m_pRight->m_sResultStructureName == "vector")
            {
                if (pNode->m_nType == NODE_ADD || pNode->m_nType == NODE_SUBTRACT || pNode->m_nType == NODE_DIVIDE)
                    return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "FV\n");
                    nLen = 3;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT_VECTOR;
                    nLen = 2;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 4;
                AddStructureToStack(CExoString("vector"), 0);
                pNode->m_nResultType = KEYWORD_STRUCT;
                pNode->m_sResultStructureName = pNode->m_pRight->m_sResultStructureName;
            }
            else
            {
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);
            }
            break;
        }



        case NODE_LOGICAL_AND:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "LOGANDII\n");
                nLen = 22;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_LOGANDII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 1;
            pNode->m_nResultType = KEYWORD_INT;

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_ILA_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 15;
            }
            else
            {
                int nDelta = m_nCompiledCodeLocation - pNode->m_nValue;
                m_pCompiledCode[pNode->m_nValue + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nValue + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nValue + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nValue + 5] = static_cast<char>(nDelta);
            }
            break;
        }

        case NODE_BOOLEAN_AND:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "BOOLANDII\n");
                nLen = 23;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_BOOLANDII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 1;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_INCLUSIVE_OR:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "INCORII\n");
                nLen = 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_INCORII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 1;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }

        case NODE_EXCLUSIVE_OR:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "EXCORII\n");
                nLen = 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_EXCORII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 1;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }

        case NODE_LOGICAL_OR:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS, pNode);

            pNode->m_nResultType = pNode->m_pRight->m_nResultType;

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_ILO2_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 16;
            }
            else
            {
                int nDelta = m_nCompiledCodeLocation - pNode->m_nValue;
                m_pCompiledCode[pNode->m_nValue + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nValue + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nValue + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nValue + 5] = static_cast<char>(nDelta);
            }

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "LOGORII\n");
                nLen = 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_LOGORII;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 1;
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_EQUAL: case NODE_NOT_EQUAL:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            int nLeftTag = pLeft->m_nResultType;
            int nLen;
            switch (nLeftTag)
            {
            case KEYWORD_INT:
                if (pRight->m_nResultType != KEYWORD_INT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    if (pNode->m_nType == NODE_EQUAL)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "EQUALII\n");
                        nLen = 21;
                    }
                    else
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEQUALII\n");
                        nLen = 22;
                    }
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                    nLen = 2;
                }

                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 1;
                pNode->m_nResultType = KEYWORD_INT;
                break;

            case KEYWORD_FLOAT:
                if (pRight->m_nResultType != KEYWORD_FLOAT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    if (pNode->m_nType == NODE_EQUAL)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "EQUALFF\n");
                        nLen = 21;
                    }
                    else
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEQUALFF\n");
                        nLen = 22;
                    }
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_FLOAT_FLOAT;
                    nLen = 2;
                }

                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 2;
                AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                pNode->m_nResultType = KEYWORD_INT;
                break;

            case KEYWORD_STRING:
                if (pRight->m_nResultType != KEYWORD_STRING)
                    return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    if (pNode->m_nType == NODE_EQUAL)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "EQUALSS\n");
                        nLen = 21;
                    }
                    else
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEQUALSS\n");
                        nLen = 22;
                    }
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STRING_STRING;
                    nLen = 2;
                }

                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 2;
                AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                pNode->m_nResultType = KEYWORD_INT;
                break;

            case KEYWORD_OBJECT:
                if (pRight->m_nResultType != KEYWORD_OBJECT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                if (m_bCompileSymbolicOutput != 0)
                {
                    PrintBinaryAddress();
                    if (pNode->m_nType == NODE_EQUAL)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "EQUALOO\n");
                        nLen = 21;
                    }
                    else
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "NEQUALOO\n");
                        nLen = 22;
                    }
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_OBJECT_OBJECT;
                    nLen = 2;
                }

                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 2;
                m_nStackCurrentDepth -= 2;
                AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                pNode->m_nResultType = KEYWORD_INT;
                break;

            default:
                if (nLeftTag >= KEYWORD_ENGINE_STRUCTURE0 && nLeftTag <= KEYWORD_ENGINE_STRUCTURE9)
                {
                    if (pRight->m_nResultType != nLeftTag)
                        return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                    if (m_bCompileSymbolicOutput != 0)
                    {
                        PrintBinaryAddress();
                        if (pNode->m_nType == NODE_EQUAL)
                        {
                            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                    "EQUALP%1dP%1d\n",
                                    (nLeftTag + (256 - KEYWORD_ENGINE_STRUCTURE0)) & 0xff,
                                    (nLeftTag + (256 - KEYWORD_ENGINE_STRUCTURE0)) & 0xff);
                            nLen = 23;
                        }
                        else
                        {
                            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                    "NEQUALP%1dP%1d\n",
                                    (nLeftTag + (256 - KEYWORD_ENGINE_STRUCTURE0)) & 0xff,
                                    (nLeftTag + (256 - KEYWORD_ENGINE_STRUCTURE0)) & 0xff);
                            nLen = 24;
                        }
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = static_cast<char>(nLeftTag - (KEYWORD_ENGINE_STRUCTURE0 - NCS_TYPE_ENGINE_STRUCTURE0_PAIR));
                        nLen = 2;
                    }

                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 2;
                    m_nStackCurrentDepth -= 2;
                    AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_INT;
                }
                else if (nLeftTag == KEYWORD_STRUCT)
                {
                    if (pRight->m_nResultType != KEYWORD_STRUCT)
                        return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);
                    if (!(pLeft->m_sResultStructureName == pRight->m_sResultStructureName))
                        return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);

                    int nSize = GetStructureSize(pLeft->m_sResultStructureName);

                    if (m_bCompileSymbolicOutput != 0)
                    {
                        PrintBinaryAddress();
                        if (pNode->m_nType == NODE_EQUAL)
                        {
                            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                    "EQUALTT %04x\n", nSize);
                            nLen = 26;
                        }
                        else
                        {
                            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                    "NEQUALTT %04x\n", nSize);
                            nLen = 27;
                        }
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation] = pNode->m_nType == NODE_EQUAL ? NCS_OP_EQUAL : NCS_OP_NEQUAL;
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STRUCT_STRUCT;
                        m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nSize >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nSize);
                        nLen = 4;
                    }

                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 4;
                    m_nStackCurrentDepth -= nSize >> 1;
                    AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
                    pNode->m_nResultType = KEYWORD_INT;
                }
                else
                {
                    return OutputWalkTreeError(-SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS, pNode);
                }
                break;
            }
            break;
        }


        case NODE_MODULUS:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);
            if (pRight->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS, pNode);

            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MODII\n");
                nLen = 19;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOD;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }

            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
            m_nStackCurrentDepth -= 2;
            AddVariableToStack(KEYWORD_INT, CExoString(""), 0);
            pNode->m_nResultType = KEYWORD_INT;
            break;
        }


        case NODE_IF_BLOCK:
        {
            pNode->m_nResultType = pNode->m_pLeft->m_nResultType;
            break;
        }

        case NODE_IF_CHOICE:
        {
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_I2_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 14;
            }
            else
            {
                int nDelta = m_nCompiledCodeLocation - pNode->m_nCodeStart;
                m_pCompiledCode[pNode->m_nCodeStart + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nCodeStart + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nCodeStart + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nCodeStart + 5] = static_cast<char>(nDelta);
            }
            break;
        }

        case NODE_INTEGER_EXPRESSION: case NODE_NON_VOID_EXPRESSION:
        {
            int nDiff = m_nStackCurrentDepth - pNode->m_nSavedStackDepth;
            if (nDiff == 1)
            {
                if (pNode->m_nType == NODE_INTEGER_EXPRESSION && m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
                    return OutputWalkTreeError(-SCRIPT_ERROR_NON_INTEGER_EXPRESSION_WHERE_INTEGER_REQUIRED, pNode);

                pNode->m_nResultType = pNode->m_pLeft->m_nResultType;
                break;
            }
            if (nDiff != 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_INCORRECT_VARIABLE_STATE_LEFT_ON_STACK, pNode);
            return OutputWalkTreeError(-SCRIPT_ERROR_VOID_EXPRESSION_WHERE_NON_VOID_REQUIRED, pNode);
        }

        case NODE_SWITCH_BLOCK:
        {
            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_BR_%08x:\n", m_nSwitchIdentifier);
                m_nCompiledCodeLocation += 14;
            }
            else
            {
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("_BR_%08x"), m_nSwitchIdentifier);
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
            }

            m_nSwitchLevel -= 1;
            m_nSwitchIdentifier = pNode->m_nCodeStart;
            m_nSwitchStackDepth = pNode->m_nSavedEnclosingState;
            break;
        }


        case NODE_DOWHILE_BLOCK:
        {
            if (m_aStackTypes[m_nStackCurrentDepth - 1] != NCS_TYPE_INTEGER)
                return OutputWalkTreeError(-SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK, pNode);
            m_nStackCurrentDepth--;

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JZ _DW2_%08x\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 30;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JZ;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = 0;
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = 0;
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = 0;
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = 12;
                m_nCompiledCodeLocation += 6;
            }
            m_nBinaryCodeOffset += 6;

            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _DW1_%08x\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 31;
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_DW2_%08x:\n",
                        pNode->m_nValue);
                m_nCompiledCodeLocation += 15;
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_BR_%08x:\n",
                        m_nLoopIdentifier);
                m_nCompiledCodeLocation += 14;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;

                int nDelta = pNode->m_nValue - m_nCompiledCodeLocation;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nDelta);

                m_nCompiledCodeLocation += 6;

                CExoString sLabel;
                sLabel.Format(const_cast<char*>("_BR_%08x"), static_cast<int>(m_nLoopIdentifier));
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
            }

            m_nBinaryCodeOffset += 6;

            CScriptParseTreeNode* pRight = pNode->m_pRight;
            pNode->m_nResultType = pRight->m_nResultType;
            if (pNode->m_nResultType == KEYWORD_STRUCT)
                pNode->m_sResultStructureName = pRight->m_sResultStructureName;
            m_nLoopIdentifier = pNode->m_nSavedEnclosingState;
            break;
        }


        case NODE_FUNCTION_IDENTIFIER:
        {
            m_nVarStackVariableType = DECLARATION_TYPE_NONE;
            if (m_bInFunctionImplementation == 0)
                return 0;

            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            CExoString s("");
            int nType = KEYWORD_INT;
            int nTag = pNode->m_nType;
            switch (pLeft->m_nType)
            {
            case NODE_TYPE_INT: break;
            case NODE_TYPE_FLOAT: nType = KEYWORD_FLOAT; break;
            case NODE_TYPE_STRING: nType = KEYWORD_STRING; break;
            case NODE_TYPE_OBJECT: nType = KEYWORD_OBJECT; break;
            case NODE_TYPE_STRUCT: s = pLeft->m_sName; nType = KEYWORD_STRUCT; break;
            case NODE_TYPE_VOID: goto emitTag;
            default:
                if (static_cast<unsigned>(pLeft->m_nType - NODE_TYPE_ENGINE_STRUCTURE0) > 9)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
                nType = pLeft->m_nType + (KEYWORD_ENGINE_STRUCTURE0 - NODE_TYPE_ENGINE_STRUCTURE0);
                break;
            }

            m_nVarStackTop++;
            if (m_bGlobalVariableDefinition != 0)
            {
                m_nGlobalVariableCount++;
                if (nType == KEYWORD_STRUCT)
                    m_nGlobalVariableSize += GetStructureSize(s);
                else
                    m_nGlobalVariableSize += 4;
            }

            m_pVarStackList[m_nVarStackTop].m_sName = "#retval";
            m_pVarStackList[m_nVarStackTop].m_nType = nType;
            if (nType == KEYWORD_STRUCT)
                m_pVarStackList[m_nVarStackTop].m_sStructureName = s;
            m_pVarStackList[m_nVarStackTop].m_nRecursionLevel = m_nVarStackRecursionLevel;
            m_pVarStackList[m_nVarStackTop].m_nRunTimeLocation = m_nStackCurrentDepth * 4;

            AddVariableToStack(nType, s, 0);
            nTag = nType;

        emitTag:
            m_nFunctionImpReturnType = nTag;
            m_sFunctionImpReturnStructureName = s;
            return 0;
        }


        case NODE_PRE_INCREMENT: case NODE_PRE_DECREMENT:
        {
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            if (pLeft == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
            if (pLeft->m_nResultType != KEYWORD_INT)
                return OutputWalkTreeError(-SCRIPT_ERROR_OPERAND_MUST_BE_AN_INTEGER_LVALUE, pNode);
            if (pLeft->m_nType != NODE_VARIABLE && pLeft->m_nType != NODE_STRUCTURE_PART)
                return OutputWalkTreeError(-SCRIPT_ERROR_OPERAND_MUST_BE_AN_INTEGER_LVALUE, pLeft);

            pNode->m_nResultType = KEYWORD_INT;
            int nDelta = pLeft->m_nValue - m_nGlobalVariableSize;
            int nKeep = 1;
            if (pLeft->m_nValue < m_nGlobalVariableSize && m_bGlobalVariableDefinition == 0)
                nKeep = 0;
            else
                nDelta = pLeft->m_nValue - m_nStackCurrentDepth * 4 + 4;

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[pNode->m_nCodeStart],
                        pNode->m_nType == NODE_PRE_INCREMENT ? "INCI" : "DECI");
                sprintf(&m_pCompiledCode[pNode->m_nCodeStart + 4],
                        nKeep != 0 ? "SP " : "BP ");
                sprintf(&m_pCompiledCode[pNode->m_nCodeStart + 7], "%08x\n", nDelta);
                m_pCompiledCode[pNode->m_nCodeStart + 16] = ' ';
            }
            else
            {
                if (nKeep == 0)
                    m_pCompiledCode[pNode->m_nCodeStart] =
                        static_cast<char>(pNode->m_nType == NODE_PRE_INCREMENT ? NCS_OP_INCIBP : NCS_OP_DECIBP);
                m_pCompiledCode[pNode->m_nCodeStart + 2] = static_cast<char>(nDelta >> 24);
                m_pCompiledCode[pNode->m_nCodeStart + 3] = static_cast<char>(nDelta >> 16);
                m_pCompiledCode[pNode->m_nCodeStart + 4] = static_cast<char>(nDelta >> 8);
                m_pCompiledCode[pNode->m_nCodeStart + 5] = static_cast<char>(nDelta);
            }
            break;
        }


        case NODE_COMPOUND_STATEMENT:
        {
            int nDepth0 = m_nStackCurrentDepth;
            int nDepth = nDepth0;
            while (m_nVarStackTop >= 0)
            {
                if (m_pVarStackList[m_nVarStackTop].m_nRecursionLevel != m_nVarStackRecursionLevel)
                    break;
                if (m_pVarStackList[m_nVarStackTop].m_nType != KEYWORD_STRUCT)
                {
                    nDepth--;
                }
                else
                {
                    int nSize = 0;
                    for (int j = 0; j < m_nStructureCount; j++)
                    {
                        if (m_pVarStackList[m_nVarStackTop].m_sStructureName ==
                            m_pStructureList[j].m_sName)
                        {
                            nSize = m_pStructureList[j].m_nSize;
                            break;
                        }
                    }
                    nDepth = m_nStackCurrentDepth - (nSize >> 2);
                }
                m_nStackCurrentDepth = nDepth;
                m_nVarStackTop--;
            }

            m_nVarStackRecursionLevel--;
            int nDelta = m_nStackCurrentDepth - nDepth0;
            if (nDelta != 0)
            {
                int nOperand = nDelta * 4;
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                            m_nBinaryCodeOffset);
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "MOVSP %08x\n",
                            nOperand);
                    m_nCompiledCodeLocation += 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_MOVSP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nOperand >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nOperand >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(nOperand >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                        static_cast<char>(nOperand);
                    m_nCompiledCodeLocation += 6;
                }
                m_nBinaryCodeOffset += 6;
            }

            if (pNode->m_nSavedStackDepth != m_nStackCurrentDepth)
                return OutputWalkTreeError(-SCRIPT_ERROR_INCORRECT_VARIABLE_STATE_LEFT_ON_STACK, pNode);
            break;
        }


        case NODE_VARIABLE:
        {
            if (m_nVarStackVariableType == DECLARATION_TYPE_NONE)
            {
                if (m_bInStructurePart == 1)
                    return 0;

                int i;
                for (i = m_nVarStackTop; i >= 0; i--)
                {
                    if (m_pVarStackList[i].m_sName == pNode->m_sName)
                        goto lIdentifierReference;
                }
                return OutputWalkTreeError(-SCRIPT_ERROR_VARIABLE_DEFINED_WITHOUT_TYPE, pNode);

            lIdentifierReference:
                pNode->m_sName = "";
                pNode->m_nResultType = m_pVarStackList[i].m_nType;
                if (pNode->m_nResultType == KEYWORD_STRUCT)
                    pNode->m_sResultStructureName = m_pVarStackList[i].m_sStructureName;
                else
                    pNode->m_sResultStructureName = "";

                int nValue0 = m_pVarStackList[i].m_nRunTimeLocation;
                pNode->m_nValue = nValue0;
                if (m_bAssignmentToVariable != 0)
                    return 0;

                {
                    int nStack = m_nStackCurrentDepth;
                    int nSize;
                    if (pNode->m_nResultType == KEYWORD_STRUCT)
                    {
                        nSize = 0;
                        for (int j = 0; j < m_nStructureCount; j++)
                        {
                            if (m_pStructureList[j].m_sName == pNode->m_sResultStructureName)
                            {
                                nSize = m_pStructureList[j].m_nSize;
                                break;
                            }
                        }
                    }
                    else
                    {
                        nSize = 4;
                    }

                    int nValue = nValue0 - nStack * 4;
                    int nDiff = pNode->m_nValue - m_nGlobalVariableSize;
                    int bBP;
                    if (pNode->m_nValue < m_nGlobalVariableSize)
                    {
                        bBP = m_bGlobalVariableDefinition != 0;
                        if (!bBP)
                            nValue = nDiff;
                    }
                    else
                    {
                        bBP = 1;
                    }

                    int nLen;
                    if (m_bCompileSymbolicOutput != 0)
                    {
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                                m_nBinaryCodeOffset);
                        sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                bBP ? "CPTOPSP %08x,%04x\n" : "CPTOPBP %08x,%04x\n",
                                nValue, nSize);
                        nLen = 35;
                    }
                    else
                    {
                        m_pCompiledCode[m_nCompiledCodeLocation] =
                            static_cast<char>(bBP ? NCS_OP_CPTOPSP : NCS_OP_CPTOPBP);
                        m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                        m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                            static_cast<char>(nValue >> 24);
                        m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                            static_cast<char>(nValue >> 16);
                        m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                            static_cast<char>(nValue >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 5] =
                            static_cast<char>(nValue);
                        m_pCompiledCode[m_nCompiledCodeLocation + 6] =
                            static_cast<char>(nSize >> 8);
                        m_pCompiledCode[m_nCompiledCodeLocation + 7] =
                            static_cast<char>(nSize);
                        nLen = 8;
                    }
                    m_nCompiledCodeLocation += nLen;
                    m_nBinaryCodeOffset += 8;

                    int nTag = pNode->m_nResultType;
                    int nTagDiff = nTag - KEYWORD_INT;
                    int nParamDiff = nTag - KEYWORD_ENGINE_STRUCTURE0;
                    if (static_cast<unsigned>(nTagDiff) < 4
                        || static_cast<unsigned>(nParamDiff) <= 9)
                    {
                        int nTypeCode = (nTag == KEYWORD_INT) ? NCS_TYPE_INTEGER : NCS_TYPE_FLOAT;
                        nTypeCode = (static_cast<unsigned int>(nTagDiff) < 2) ? nTypeCode : NCS_TYPE_STRING;
                        nTypeCode = (static_cast<unsigned int>(nTagDiff) < 3) ? nTypeCode : NCS_TYPE_OBJECT;
                        nTypeCode = (static_cast<unsigned int>(nTagDiff) < 4
                                     || static_cast<unsigned int>(nParamDiff) > 9)
                                        ? nTypeCode
                                        : nTag - (KEYWORD_ENGINE_STRUCTURE0 - NCS_TYPE_ENGINE_STRUCTURE0);
                        m_aStackTypes[m_nStackCurrentDepth] = static_cast<char>(nTypeCode);
                        m_nStackCurrentDepth++;
                    }
                    else if (nTag == KEYWORD_STRUCT)
                    {
                        AddStructureToStack(pNode->m_sResultStructureName, 0);
                    }
                }
                return 0;
            }

            if (m_nStructureDefinition == 0)
            {
                for (int i = m_nVarStackTop; i >= 0; i--)
                {
                    if (m_pVarStackList[i].m_sName == pNode->m_sName
                        && m_pVarStackList[i].m_nRecursionLevel == m_nVarStackRecursionLevel)
                        return OutputWalkTreeError(-SCRIPT_ERROR_VARIABLE_ALREADY_USED_WITHIN_SCOPE, pNode);
                }

                m_nVarStackTop++;
                if (m_bGlobalVariableDefinition != 0)
                {
                    m_nGlobalVariableCount++;
                    if (m_nVarStackVariableType == KEYWORD_STRUCT)
                    {
                        int nSize = 0;
                        for (int j = 0; j < m_nStructureCount; j++)
                        {
                            if (m_pStructureList[j].m_sName == m_sVarStackVariableTypeName)
                            {
                                nSize = m_pStructureList[j].m_nSize;
                                break;
                            }
                        }
                        m_nGlobalVariableSize += nSize;
                    }
                    else
                    {
                        m_nGlobalVariableSize += 4;
                    }
                }

                m_pVarStackList[m_nVarStackTop].m_sName = pNode->m_sName;
                m_pVarStackList[m_nVarStackTop].m_nType = m_nVarStackVariableType;
                if (m_nVarStackVariableType == KEYWORD_STRUCT)
                {
                    m_pVarStackList[m_nVarStackTop].m_sStructureName = m_sVarStackVariableTypeName;
                    int bTypeFound = 0;
                    for (int j = 0; j < m_nStructureCount; j++)
                    {
                        if (m_pStructureList[j].m_sName == m_sVarStackVariableTypeName)
                            bTypeFound = 1;
                    }
                    if (bTypeFound == 0)
                        return OutputWalkTreeError(-SCRIPT_ERROR_UNDEFINED_STRUCTURE, pNode);
                }
                else
                {
                    m_sVarStackVariableTypeName = "";
                }

                m_pVarStackList[m_nVarStackTop].m_nRecursionLevel = m_nVarStackRecursionLevel;
                m_pVarStackList[m_nVarStackTop].m_nRunTimeLocation = m_nStackCurrentDepth * 4;
                AddVariableToStack(m_nVarStackVariableType, m_sVarStackVariableTypeName, 1);
                return 0;
            }

            for (int i = m_nStructureDefinitionFieldStart; i < m_nFieldCount; i++)
            {
                if (m_pStructureFieldList[i].m_sName == pNode->m_sName)
                    return OutputWalkTreeError(-SCRIPT_ERROR_VARIABLE_USED_TWICE_IN_SAME_STRUCTURE, pNode);
            }

            m_pStructureFieldList[m_nFieldCount].m_nType = static_cast<unsigned char>(m_nVarStackVariableType);
            if (m_nVarStackVariableType == KEYWORD_STRUCT)
                m_pStructureFieldList[m_nFieldCount].m_sStructureName = m_sVarStackVariableTypeName;
            m_pStructureFieldList[m_nFieldCount].m_sName = pNode->m_sName;
            m_pStructureFieldList[m_nFieldCount].m_nOffset = 0;
            m_nFieldCount++;
            return 0;
        }

        case NODE_ACTION:
        {
            CScriptParseTreeNode* pRight = pNode->m_pRight;
            if (pRight == 0)
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

            int nDepth = 0;
            CScriptParseTreeNode* pLeft = pNode->m_pLeft;
            while (pLeft != 0)
            {
                if (static_cast<unsigned int>(pLeft->m_nResultType - KEYWORD_INT) > KEYWORD_ENGINE_STRUCTURE9 - KEYWORD_INT
                    || (ACTION_ARGUMENT_TYPE_MASK
                        & (1ULL << static_cast<unsigned int>(pLeft->m_nResultType - KEYWORD_INT))) == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

                m_aArgumentTypes[nDepth] = static_cast<unsigned char>(pLeft->m_nResultType);
                if (pLeft->m_nResultType != KEYWORD_ACTION)
                {
                    if (pLeft->m_nResultType == KEYWORD_STRUCT)
                    {
                        m_aArgumentStructureNames[nDepth] = pLeft->m_sResultStructureName;
                        int nSize = 0;
                        for (int i = 0; i < m_nStructureCount; i++)
                        {
                            if (m_pStructureList[i].m_sName == pLeft->m_sResultStructureName)
                            {
                                nSize = m_pStructureList[i].m_nSize;
                                break;
                            }
                        }
                        m_nStackCurrentDepth -= nSize >> 2;
                    }
                    else
                    {
                        m_nStackCurrentDepth--;
                    }
                }

                nDepth++;
                pLeft = pLeft->m_pLeft;
            }

            int nIndex = pNode->m_pRight->m_nValue;
            int nInRange = (nDepth <= m_pIdentifierList[nIndex].m_nParameterCount)
                           & (nDepth >= m_pIdentifierList[nIndex].m_nNonOptionalParameterCount);
            for (int j = 0; j < nDepth; j++)
            {
                if (m_aArgumentTypes[j] != m_pIdentifierList[nIndex].m_pParameterTypes[j]
                    || (m_aArgumentTypes[j] == KEYWORD_STRUCT
                        && m_aArgumentStructureNames[j] != m_pIdentifierList[nIndex].m_pParameterStructureNames[j]))
                    nInRange = 0;
            }
            if (nInRange != 1)
                return OutputWalkTreeError(-SCRIPT_ERROR_DECLARATION_DOES_NOT_MATCH_PARAMETERS, pNode);

            for (int j = nDepth; j < m_pIdentifierList[nIndex].m_nParameterCount; j++)
            {
                if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_ACTION)
                {
                }
                else if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_STRUCT)
                {
                    m_aArgumentStructureNames[nDepth] = m_pIdentifierList[nIndex].m_pParameterStructureNames[j];
                    int nSize = 0;
                    for (int i = 0; i < m_nStructureCount; i++)
                    {
                        if (m_pStructureList[i].m_sName == m_pIdentifierList[nIndex].m_pParameterStructureNames[j])
                        {
                            nSize = m_pStructureList[i].m_nSize;
                            break;
                        }
                    }
                    m_nStackCurrentDepth -= nSize >> 2;
                }
                else
                {
                    m_nStackCurrentDepth--;
                }
            }

            int nType = m_pIdentifierList[nIndex].m_nReturnType;
            if (nType != TOKEN_VOID_IDENTIFIER)
            {
                pNode->m_sResultStructureName = "";
                switch (nType)
                {
                case TOKEN_INTEGER_IDENTIFIER:
                    pNode->m_nResultType = KEYWORD_INT;
                    break;
                case TOKEN_FLOAT_IDENTIFIER:
                    pNode->m_nResultType = KEYWORD_FLOAT;
                    break;
                case TOKEN_STRING_IDENTIFIER:
                    pNode->m_nResultType = KEYWORD_STRING;
                    break;
                case TOKEN_OBJECT_IDENTIFIER:
                    pNode->m_nResultType = KEYWORD_OBJECT;
                    break;
                }

                if (static_cast<unsigned int>(nType - TOKEN_ENGINE_STRUCTURE0_IDENTIFIER) <= 9)
                    pNode->m_nResultType = nType - (TOKEN_ENGINE_STRUCTURE0_IDENTIFIER - KEYWORD_ENGINE_STRUCTURE0);
                else if (nType == TOKEN_STRUCTURE_IDENTIFIER)
                {
                    pNode->m_nResultType = KEYWORD_STRUCT;
                    pNode->m_sResultStructureName = m_pIdentifierList[nIndex].m_sReturnStructureName;
                }

                if (pNode->m_nValue != 0)
                    goto lIdentifierCall;

                int nTag = pNode->m_nResultType;
                int nTagDiff = nTag - KEYWORD_INT;
                int nParamDiff = nTag - KEYWORD_ENGINE_STRUCTURE0;
                if (static_cast<unsigned int>(nTagDiff) < 4
                    || static_cast<unsigned int>(nParamDiff) <= 9)
                {
                    int nTypeCode = (nTag == KEYWORD_INT) ? NCS_TYPE_INTEGER : NCS_TYPE_FLOAT;
                    nTypeCode = (static_cast<unsigned int>(nTagDiff) < 2) ? nTypeCode : NCS_TYPE_STRING;
                    nTypeCode = (static_cast<unsigned int>(nTagDiff) < 3) ? nTypeCode : NCS_TYPE_OBJECT;
                    nTypeCode = (static_cast<unsigned int>(nTagDiff) < 4
                                 || static_cast<unsigned int>(nParamDiff) > 9)
                                    ? nTypeCode
                                    : nTag - (KEYWORD_ENGINE_STRUCTURE0 - NCS_TYPE_ENGINE_STRUCTURE0);
                    m_aStackTypes[m_nStackCurrentDepth] = static_cast<char>(nTypeCode);
                    m_nStackCurrentDepth++;
                }
                else if (nTag == KEYWORD_STRUCT)
                {
                    AddStructureToStack(pNode->m_sResultStructureName, 0);
                }
            }

        lIdentifierCall:
            if (pNode->m_nValue == 0)
            {
                int nAction = m_pIdentifierList[nIndex].m_nActionId;
                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                            "ACTION %04x %02x\n", nAction & 0xffff,
                            static_cast<unsigned short>(
                                m_pIdentifierList[nIndex].m_nParameterCount));
                    nLen = 28;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_ACTION;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nAction >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nAction);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] =
                        static_cast<char>(m_pIdentifierList[nIndex].m_nParameterCount);
                    nLen = 5;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 5;
                return 0;
            }

            if (m_bCompileSymbolicOutput != 0)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_%s\n",
                        m_pIdentifierList[nIndex].m_sName.CStr());
#if KOTOR1
                uint64_t nNameLen = m_pIdentifierList[nIndex].m_sName.GetLength();
#else
                uint64_t nNameLen = 0;
                if (m_pIdentifierList[nIndex].m_sName.m_sString != 0)
                    nNameLen = strlen(m_pIdentifierList[nIndex].m_sName.m_sString);
#endif
                m_nCompiledCodeLocation += static_cast<int>(nNameLen) + 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JSR;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("FE_%s"), m_pIdentifierList[nIndex].m_sName.CStr());
                AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
                m_nCompiledCodeLocation += 6;
            }
            m_nBinaryCodeOffset += 6;
            return 0;
        }



        case NODE_STATEMENT: case NODE_WHILE_CHOICE: case NODE_FUNCTIONAL_UNIT: case NODE_FUNCTION_DECLARATION:
            break;

        default:
        {
            switch (pNode->m_nType)
            {
            case NODE_POST_INCREMENT: case NODE_POST_DECREMENT:
            {
                CScriptParseTreeNode* pLeft = pNode->m_pLeft;
                if (pLeft == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
                if (pLeft->m_nResultType != KEYWORD_INT)
                    return OutputWalkTreeError(-SCRIPT_ERROR_OPERAND_MUST_BE_AN_INTEGER_LVALUE, pNode);
                if (pLeft->m_nType != NODE_VARIABLE && pLeft->m_nType != NODE_STRUCTURE_PART)
                    return OutputWalkTreeError(-SCRIPT_ERROR_OPERAND_MUST_BE_AN_INTEGER_LVALUE, pLeft);

                pNode->m_nResultType = KEYWORD_INT;
                int nValue0 = pLeft->m_nValue;
                int nValue = nValue0 - m_nStackCurrentDepth * 4;
                int nDiff = nValue0 - m_nGlobalVariableSize;
                int bBP;
                if (nValue0 < m_nGlobalVariableSize)
                {
                    bBP = m_bGlobalVariableDefinition != 0;
                    if (!bBP)
                        nValue = nDiff;
                }
                else
                {
                    bBP = 1;
                }

                int nLen;
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                            m_nBinaryCodeOffset);
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                            pNode->m_nType == NODE_POST_INCREMENT ? "INCI" : "DECI");
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + (SYMBOLIC_ADDRESS_WIDTH + 4)],
                            bBP != 0 ? "SP " : "BP ");
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + (SYMBOLIC_ADDRESS_WIDTH + 7)], "%08x\n",
                            nValue);
                    nLen = 29;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = static_cast<char>(
                        pNode->m_nType == NODE_POST_INCREMENT ? (bBP != 0 ? NCS_OP_INCISP : NCS_OP_INCIBP)
                                               : (bBP != 0 ? NCS_OP_DECISP : NCS_OP_DECIBP));
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                    m_pCompiledCode[m_nCompiledCodeLocation + 2] =
                        static_cast<char>(nValue >> 24);
                    m_pCompiledCode[m_nCompiledCodeLocation + 3] =
                        static_cast<char>(nValue >> 16);
                    m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nValue >> 8);
                    m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nValue);
                    nLen = 6;
                }
                m_nCompiledCodeLocation += nLen;
                m_nBinaryCodeOffset += 6;
                break;
            }

            case NODE_DEFAULT:
            {
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_SC_DEFAULT_%08x:\n",
                            static_cast<int>(m_nSwitchIdentifier));
                    m_nCompiledCodeLocation += 22;
                }
                else
                {
                    CExoString sLabel;
                    sLabel.Format(const_cast<char*>("_SC_DEFAULT_%08x"), static_cast<int>(m_nSwitchIdentifier));
                    AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
                }
                if (m_nSwitchStackDepth + 1 != m_nStackCurrentDepth)
                    return OutputWalkTreeError(-SCRIPT_ERROR_SKIPPING_DECLARATION_VIA_DEFAULT_STATEMENT_DISALLOWED, pNode);
                break;
            }

            case NODE_CASE:
                break;

            case NODE_BREAK:
            {
                int nValue = m_nSwitchIdentifier > m_nLoopIdentifier ? m_nSwitchIdentifier : m_nLoopIdentifier;
                if (nValue == 0)
                    return OutputWalkTreeError(-SCRIPT_ERROR_BREAK_OUTSIDE_OF_LOOP_OR_CASE_STATEMENT, pNode);
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                            m_nBinaryCodeOffset);
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _BR_%08x\n",
                            nValue);
                    m_nCompiledCodeLocation += 30;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    CExoString sLabel;
                    sLabel.Format(const_cast<char*>("_BR_%08x"), nValue);
                    AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
                    m_nCompiledCodeLocation += 6;
                }
                m_nBinaryCodeOffset += 6;
                break;
            }

            case NODE_CONTINUE:
            {
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ",
                            m_nBinaryCodeOffset);
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JMP _CN_%08x\n",
                            static_cast<int>(m_nLoopIdentifier));
                    m_nCompiledCodeLocation += 30;
                }
                else
                {
                    m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
                    m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
                    CExoString sLabel;
                    sLabel.Format(const_cast<char*>("_CN_%08x"), static_cast<int>(m_nLoopIdentifier));
                    AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
                    m_nCompiledCodeLocation += 6;
                }
                m_nBinaryCodeOffset += 6;
                break;
            }

            case NODE_CONTINUE_TARGET:
            {
                if (m_bCompileSymbolicOutput != 0)
                {
                    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "_CN_%08x:\n",
                            static_cast<int>(m_nLoopIdentifier));
                    m_nCompiledCodeLocation += 14;
                    break;
                }
                CExoString sLabel;
                sLabel.Format(const_cast<char*>("_CN_%08x"), static_cast<int>(m_nLoopIdentifier));
                AddSymbolToLabelList(sLabel, m_nCompiledCodeLocation);
                break;
            }

            default:
                return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_OPERATION_IN_SEMANTIC_CHECK, pNode);
            }
        }
        }
        break;
    }

    return nResult;
}

// Releases both symbol-list arrays.
void CScriptCompilerInternal::ClearAllSymbolLists()
{
    if (m_pSymbolQueryList != 0) {
        delete[] m_pSymbolQueryList;
        m_pSymbolQueryList = 0;
    }
    m_nSymbolQueryListCapacity = 0;
    m_nSymbolQueryListCount = 0;
    if (m_pSymbolLabelList != 0) {
        delete[] m_pSymbolLabelList;
        m_pSymbolLabelList = 0;
    }
    m_nSymbolLabelListCapacity = 0;
    m_nSymbolLabelListCount = 0;
}

// Releases every identifier-table slot above the saved watermark.
void CScriptCompilerInternal::ClearUserDefinedIdentifiers()
{
    for (int i = m_nIdentifierBase; i < m_nIdentifierCount; i++) {
        m_pIdentifierList[i].m_sName = "";
        m_pIdentifierList[i].m_nNameLength = 0;
        m_pIdentifierList[i].m_nIdentifierType = IDENTIFIER_TYPE_CONSTANT;
        m_pIdentifierList[i].m_nReturnType = 0;
        m_pIdentifierList[i].m_bImplementationInPlace = 0;
        m_pIdentifierList[i].m_sReturnStructureName = "";
        m_pIdentifierList[i].m_sStringData = "";
        m_pIdentifierList[i].m_nIntegerData = 0;
        m_pIdentifierList[i].m_fFloatData = 0.0f;
        m_pIdentifierList[i].m_nActionId = -1;
        m_pIdentifierList[i].m_nParameterCount = 0;
        m_pIdentifierList[i].m_nNonOptionalParameterCount = 0;
        m_pIdentifierList[i].m_nParameterSpace = 0;
        if (m_pIdentifierList[i].m_pParameterTypes != 0) {
            delete[] m_pIdentifierList[i].m_pParameterTypes;
            m_pIdentifierList[i].m_pParameterTypes = 0;
        }
        if (m_pIdentifierList[i].m_pParameterStructureNames != 0) {
            delete[] m_pIdentifierList[i].m_pParameterStructureNames;
            m_pIdentifierList[i].m_pParameterStructureNames = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterFlags != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterFlags;
            m_pIdentifierList[i].m_pOptionalParameterFlags = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterIntegerData != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterIntegerData;
            m_pIdentifierList[i].m_pOptionalParameterIntegerData = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterFloatData != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterFloatData;
            m_pIdentifierList[i].m_pOptionalParameterFloatData = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterStringData != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterStringData;
            m_pIdentifierList[i].m_pOptionalParameterStringData = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterObjectData != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterObjectData;
            m_pIdentifierList[i].m_pOptionalParameterObjectData = 0;
        }
        if (m_pIdentifierList[i].m_pOptionalParameterVectorData != 0) {
            delete[] m_pIdentifierList[i].m_pOptionalParameterVectorData;
            m_pIdentifierList[i].m_pOptionalParameterVectorData = 0;
        }
        m_pIdentifierList[i].m_nCodeOffset = -1;
        m_pIdentifierList[i].m_nCodeEnd = -1;
        m_pIdentifierList[i].m_nOutputStart = -1;
        m_pIdentifierList[i].m_nOutputEnd = -1;
    }
    m_nIdentifierCount = m_nIdentifierBase;
}

// Deep copy of one parse tree.
CScriptParseTreeNode* CScriptCompilerInternal::DuplicateScriptParseTree(CScriptParseTreeNode* pNode)
{
    if (pNode == 0)
        return 0;
    CScriptParseTreeNode* pNew = new CScriptParseTreeNode();
    pNew->m_nType = pNode->m_nType;
    pNew->m_sName = pNode->m_sName;
    pNew->m_nValue = pNode->m_nValue;
    pNew->m_nCodeStart = pNode->m_nCodeStart;
    pNew->m_fValue = pNode->m_fValue;
    pNew->m_fVectorX = pNode->m_fVectorX;
    pNew->m_fVectorY = pNode->m_fVectorY;
    pNew->m_fVectorZ = pNode->m_fVectorZ;
    pNew->m_nLine = pNode->m_nLine;
    pNew->m_nColumn = pNode->m_nColumn;
    pNew->m_nResultType = pNode->m_nResultType;
    pNew->m_sResultStructureName = pNode->m_sResultStructureName;
    pNew->m_nSavedStackDepth = pNode->m_nSavedStackDepth;
    pNew->m_pLeft = DuplicateScriptParseTree(pNode->m_pLeft);
    pNew->m_pRight = DuplicateScriptParseTree(pNode->m_pRight);
    return pNew;
}

// Emits code in a recursive walk: pre, left, in, right, post.
int CScriptCompilerInternal::WalkParseTree(CScriptParseTreeNode* pNode)
{
    if (pNode == 0)
        return 0;

    int nRet = PreVisitGenerateCode(pNode);
    if (nRet == 0) {
        nRet = WalkParseTree(pNode->m_pLeft);
        if (nRet == 0) {
            nRet = InVisitGenerateCode(pNode);
            if (nRet == 0) {
                nRet = WalkParseTree(pNode->m_pRight);
                if (nRet == 0)
                    nRet = PostVisitGenerateCode(pNode);
            }
        }
    }
    if (nRet < 0)
        return nRet & (nRet >> 31);
    int nCodeLen = m_nCompiledCodeLocation;
    if (nCodeLen >= m_nCompiledCodeSize - CODE_BUFFER_SLACK) {
        m_nCompiledCodeSize += CODE_BUFFER_SIZE;
        char* pNewCode = new char[m_nCompiledCodeSize];
        __builtin_memcpy(pNewCode, m_pCompiledCode, nCodeLen);  // idiom-exempt: runtime length (compiled-code length)
        delete[] m_pCompiledCode;
        m_pCompiledCode = pNewCode;
        return OutputWalkTreeError(-SCRIPT_ERROR_SCRIPT_TOO_LARGE, pNode);
    }
    return 0;
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

int CScriptCompilerInternal::AddUserDefinedIdentifier(CScriptParseTreeNode* pNode, int nFlags)
{
    m_pIdentifierList[m_nIdentifierCount].m_sName = pNode->m_pLeft->m_pLeft->m_sName;
#if KOTOR1
    m_pIdentifierList[m_nIdentifierCount].m_nNameLength = pNode->m_pLeft->m_pLeft->m_sName.GetLength();
#else
    m_pIdentifierList[m_nIdentifierCount].m_nNameLength =
        pNode->m_pLeft->m_pLeft->m_sName.m_sString
            ? static_cast<int>(strlen(pNode->m_pLeft->m_pLeft->m_sName.m_sString))
            : 0;
#endif
    m_pIdentifierList[m_nIdentifierCount].m_nIdentifierType = IDENTIFIER_TYPE_FUNCTION;

    int nDeclType = pNode->m_pLeft->m_pLeft->m_pLeft->m_nType;
    switch (nDeclType) {
    case NODE_TYPE_INT:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_INTEGER_IDENTIFIER;
        break;
    case NODE_TYPE_FLOAT:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_FLOAT_IDENTIFIER;
        break;
    case NODE_TYPE_STRING:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_STRING_IDENTIFIER;
        break;
    case NODE_TYPE_OBJECT:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_OBJECT_IDENTIFIER;
        break;
    case NODE_TYPE_STRUCT:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_STRUCTURE_IDENTIFIER;
        m_pIdentifierList[m_nIdentifierCount].m_sReturnStructureName = pNode->m_pLeft->m_pLeft->m_pLeft->m_sName;
        break;
    case NODE_TYPE_VOID:
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_VOID_IDENTIFIER;
        break;
    default:
        if (static_cast<unsigned int>(nDeclType - NODE_TYPE_ENGINE_STRUCTURE0) > 9)
            return -SCRIPT_ERROR_PARSING_FUNCTION_DECLARATION;
        m_pIdentifierList[m_nIdentifierCount].m_nReturnType = nDeclType + (TOKEN_ENGINE_STRUCTURE0_IDENTIFIER - NODE_TYPE_ENGINE_STRUCTURE0);
        break;
    }
    m_pIdentifierList[m_nIdentifierCount].m_nActionId = -1;

    int nParamCount = 0;
    for (CScriptParseTreeNode* p = pNode->m_pLeft->m_pRight; p != 0; p = p->m_pLeft)
        nParamCount++;
    while (nParamCount > m_pIdentifierList[m_nIdentifierCount].m_nParameterSpace) {
        int nRet = m_pIdentifierList[m_nIdentifierCount].ExpandParameterSpace();
        if (nRet < 0)
            return nRet;
    }

    CScriptParseTreeNode* pParam = pNode->m_pLeft->m_pRight;
    if (pParam == 0) {
        m_pIdentifierList[m_nIdentifierCount].m_nParameterCount = 0;
        m_pIdentifierList[m_nIdentifierCount].m_nNonOptionalParameterCount = 0;
    } else {
        int nIndex = 0;
        int nState = 0;
        do {
            int nParamType = pParam->m_pRight->m_nType;
            char cType;
            switch (nParamType) {
            case NODE_TYPE_INT:
                cType = KEYWORD_INT;
                break;
            case NODE_TYPE_FLOAT:
                cType = KEYWORD_FLOAT;
                break;
            case NODE_TYPE_STRING:
                cType = KEYWORD_STRING;
                break;
            case NODE_TYPE_OBJECT:
                cType = KEYWORD_OBJECT;
                break;
            case NODE_TYPE_STRUCT:
                cType = KEYWORD_STRUCT;
                break;
            case NODE_TYPE_VOID:
                cType = KEYWORD_VOID;
                break;
            default:
                if (static_cast<unsigned int>(nParamType - NODE_TYPE_ENGINE_STRUCTURE0) > 9)
                    return -SCRIPT_ERROR_PARSING_FUNCTION_DECLARATION;
                cType = static_cast<char>(nParamType + (KEYWORD_ENGINE_STRUCTURE0 - NODE_TYPE_ENGINE_STRUCTURE0));
                break;
            }
            m_pIdentifierList[m_nIdentifierCount].m_pParameterTypes[nIndex] = cType;
            m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 0;
            if (nState == 1 && pParam->m_pRight->m_pRight == 0)
                return -SCRIPT_ERROR_NON_OPTIONAL_PARAMETER_CANNOT_FOLLOW_OPTIONAL_PARAMETER;
            switch (static_cast<unsigned char>(cType)) {
            case KEYWORD_INT: {
                CScriptParseTreeNode* pSub = pParam->m_pRight->m_pRight;
                if (pSub != 0) {
                    if (pSub->m_nType != NODE_CONSTANT_INTEGER) {
                        pParam->m_pRight->m_pRight = 0;
                        delete pSub;
                        return -SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION;
                    }
                    nState = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterIntegerData[nIndex] = pSub->m_nValue;
                    pParam->m_pRight->m_pRight = 0;
                    delete pSub;
                }
                break;
            }
            case KEYWORD_FLOAT: {
                CScriptParseTreeNode* pSub = pParam->m_pRight->m_pRight;
                if (pSub != 0) {
                    if (pSub->m_nType != NODE_CONSTANT_FLOAT) {
                        pParam->m_pRight->m_pRight = 0;
                        delete pSub;
                        return -SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION;
                    }
                    nState = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFloatData[nIndex] = pSub->m_fValue;
                    pParam->m_pRight->m_pRight = 0;
                    delete pSub;
                }
                break;
            }
            case KEYWORD_STRING: {
                CScriptParseTreeNode* pSub = pParam->m_pRight->m_pRight;
                if (pSub != 0) {
                    if (pSub->m_nType != NODE_CONSTANT_STRING) {
                        pParam->m_pRight->m_pRight = 0;
                        delete pSub;
                        return -SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION;
                    }
                    nState = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterStringData[nIndex] = pSub->m_sName;
                    pParam->m_pRight->m_pRight = 0;
                    delete pSub;
                }
                break;
            }
            case KEYWORD_OBJECT: {
                CScriptParseTreeNode* pSub = pParam->m_pRight->m_pRight;
                if (pSub != 0) {
                    if (pSub->m_nType != NODE_CONSTANT_OBJECT) {
                        pParam->m_pRight->m_pRight = 0;
                        delete pSub;
                        return -SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION;
                    }
                    nState = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 1;
                    m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterObjectData[nIndex] = pSub->m_nValue;
                    pParam->m_pRight->m_pRight = 0;
                    delete pSub;
                }
                break;
            }
            case KEYWORD_VOID:
                if (pParam->m_pRight->m_pRight != 0)
                    return -SCRIPT_ERROR_TYPE_DOES_NOT_HAVE_AN_OPTIONAL_PARAMETER;
                break;
            case KEYWORD_STRUCT: {
                m_pIdentifierList[m_nIdentifierCount].m_pParameterStructureNames[nIndex] = pParam->m_pRight->m_sName;
                if (pParam->m_pRight->m_pRight != 0) {
                    if (pParam->m_pRight->m_sName != "vector")
                        return -SCRIPT_ERROR_TYPE_DOES_NOT_HAVE_AN_OPTIONAL_PARAMETER;
                    if (pParam->m_pRight->m_pRight != 0 &&
                        pParam->m_pRight->m_sName == "vector") {
                        CScriptParseTreeNode* pSub = pParam->m_pRight->m_pRight;
                        if (pSub->m_nType != NODE_CONSTANT_VECTOR) {
                            pParam->m_pRight->m_pRight = 0;
                            delete pSub;
                            return -SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION;
                        }
                        nState = 1;
                        m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterFlags[nIndex] = 1;
                        float* pnComponents = m_pIdentifierList[m_nIdentifierCount].m_pOptionalParameterVectorData;
                        pnComponents[nIndex * 3] = pSub->m_fVectorX;
                        pnComponents[nIndex * 3 + 1] = pSub->m_fVectorY;
                        pnComponents[nIndex * 3 + 2] = pSub->m_fVectorZ;
                        pParam->m_pRight->m_pRight = 0;
                        delete pSub;
                    }
                }
                break;
            }
            default:
                if (cType >= KEYWORD_ENGINE_STRUCTURE0) {
                    if (pParam->m_pRight->m_pRight != 0)
                        return -SCRIPT_ERROR_TYPE_DOES_NOT_HAVE_AN_OPTIONAL_PARAMETER;
                }
                break;
            }
            nIndex++;
            m_pIdentifierList[m_nIdentifierCount].m_nParameterCount = nIndex;
            if (nState == 0)
                m_pIdentifierList[m_nIdentifierCount].m_nNonOptionalParameterCount = nIndex;
            pParam = pParam->m_pLeft;
        } while (pParam != 0);
    }

    int nDuplicate = 0;
    if (m_nIdentifierCount > 0) {
        for (int i = 0; i < m_nIdentifierCount; i++) {
            if (m_pIdentifierList[i].m_nNameLength != m_pIdentifierList[m_nIdentifierCount].m_nNameLength)
                continue;
            if (!(m_pIdentifierList[i].m_sName == m_pIdentifierList[m_nIdentifierCount].m_sName))
                continue;
            if (m_pIdentifierList[i].m_nParameterCount == m_pIdentifierList[m_nIdentifierCount].m_nParameterCount) {
                if (m_pIdentifierList[i].m_nParameterCount >= 1) {
                    int nMismatch = 0;
                    for (int k = 0; k < m_pIdentifierList[i].m_nParameterCount; k++) {
                        unsigned char cOld = m_pIdentifierList[i].m_pParameterTypes[k];
                        if (cOld != m_pIdentifierList[m_nIdentifierCount].m_pParameterTypes[k] ||
                            (cOld == TOKEN_STRUCTURE_IDENTIFIER &&
                             m_pIdentifierList[i].m_pParameterStructureNames[k] !=
                                 m_pIdentifierList[m_nIdentifierCount].m_pParameterStructureNames[k]))
                            nMismatch = 1;
                    }
                    if (nMismatch) {
                        if (nDuplicate)
                            continue;
                        return -SCRIPT_ERROR_FUNCTION_IMPLEMENTATION_AND_DEFINITION_DIFFER;
                    }
                }
            } else {
                if (nDuplicate)
                    continue;
                return -SCRIPT_ERROR_FUNCTION_IMPLEMENTATION_AND_DEFINITION_DIFFER;
            }
            if (nFlags == 1 && m_pIdentifierList[i].m_bImplementationInPlace == 1)
                return -SCRIPT_ERROR_DUPLICATE_FUNCTION_IMPLEMENTATION;
            nDuplicate = 1;
        }
    }

    if (!nDuplicate) {
        int nSlot = m_nIdentifierCount;
        m_pIdentifierList[nSlot].m_bImplementationInPlace = nFlags;
        __builtin_memset(&m_pIdentifierList[nSlot].m_nCodeOffset, -1,
                          sizeof(m_pIdentifierList[nSlot].m_nCodeOffset) +
                          sizeof(m_pIdentifierList[nSlot].m_nCodeEnd) +
                          sizeof(m_pIdentifierList[nSlot].m_nOutputStart) +
                          sizeof(m_pIdentifierList[nSlot].m_nOutputEnd));
        m_nIdentifierCount = nSlot + 1;
        if (nSlot > IDENTIFIER_LIST_COUNT - 2)
            return -SCRIPT_ERROR_IDENTIFIER_LIST_FULL;
    }
    return 0;
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

// Appends a decimal digit to the token being built, while the 512-byte token buffer has room.
int CScriptCompilerInternal::ParseCharacterNumeric(int nChar)
{
    switch (m_nTokenType) {
    case TOKEN_NUMBER:
    case TOKEN_FLOAT:
    case TOKEN_IDENTIFIER:
    case TOKEN_STRING:
    case TOKEN_HEX_NUMBER:
        break;
    case TOKEN_NONE:
        m_nTokenType = TOKEN_NUMBER;
        m_nTokenLength = 0;
        break;
    default:
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    }
    m_szTokenText[m_nTokenLength] = static_cast<char>(nChar);
    int nLength = m_nTokenLength;
    m_nTokenLength = nLength + 1;
    return nLength > MAX_TOKEN_LENGTH - 1 ? -SCRIPT_ERROR_TOKEN_TOO_LONG : 0;
}

int CScriptCompilerInternal::ParseCharacterPeriod()
{
    if (m_nTokenType == TOKEN_NUMBER) {
        m_nTokenType = TOKEN_FLOAT;
        m_szTokenText[m_nTokenLength] = '.';
        int nLength = m_nTokenLength;
        m_nTokenLength = nLength + 1;
        return nLength > MAX_TOKEN_LENGTH - 2 ? -SCRIPT_ERROR_TOKEN_TOO_LONG : 0;
    }
    if (m_nTokenType == TOKEN_IDENTIFIER) {
        int nResult = HandleIdentifierToken();
        if (nResult < 0)
            return nResult;
    } else if (m_nTokenType != TOKEN_NONE) {
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    }
    m_nTokenType = TOKEN_PERIOD;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterAlphabet(int nChar)
{
    if (m_nTokenType == TOKEN_NONE) {
        m_nTokenType = TOKEN_IDENTIFIER;
        m_nTokenLength = 0;
        m_szTokenText[m_nTokenLength] = static_cast<char>(nChar);
    } else if ((nChar | ('a' - 'A')) == 'x' && m_nTokenType == TOKEN_NUMBER &&
               m_nTokenLength == 1 && m_szTokenText[0] == '0') {
        m_szTokenText[1] = static_cast<char>(nChar);
        m_nTokenType = TOKEN_HEX_NUMBER;
        m_nTokenLength = 2;
        return 0;
    } else if (m_nTokenType == TOKEN_HEX_NUMBER) {
        unsigned int nIndex = static_cast<unsigned int>(nChar - 'A');
        if (nIndex > 'f' - 'A')
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        if (((1LL << nIndex) & HEX_LETTER_MASK) == 0)
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        m_szTokenText[m_nTokenLength] = static_cast<char>(nChar < 'G' ? nChar + ('a' - 'A') : nChar);
    } else {
        if ((m_nTokenType & ~1) != TOKEN_IDENTIFIER)
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        m_szTokenText[m_nTokenLength] = static_cast<char>(nChar);
    }
    int nLength = m_nTokenLength;
    m_nTokenLength = nLength + 1;
    return nLength > MAX_TOKEN_LENGTH - 2 ? -SCRIPT_ERROR_TOKEN_TOO_LONG : 0;
}
int CScriptCompilerInternal::ParseCharacterEllipsis()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_szTokenText[0] = '#';
    m_nTokenType = TOKEN_IDENTIFIER;
    m_nTokenLength = 1;
    return 0;
}

// Walks the switch body's statement chain collecting its `case` labels into the label table.
int CScriptCompilerInternal::TraverseTreeForSwitchLabels(CScriptParseTreeNode* pNode)
{
    if (pNode == 0 || pNode->m_nType == NODE_SWITCH_BLOCK)
        return 0;

    int nRet = TraverseTreeForSwitchLabels(pNode->m_pLeft);
    if (nRet < 0)
        return nRet;

    if (pNode->m_nType == NODE_DEFAULT)
    {
        if (m_bSwitchHasDefault == 1)
            return OutputWalkTreeError(-SCRIPT_ERROR_MULTIPLE_DEFAULT_STATEMENTS_WITHIN_SWITCH, pNode);
        m_bSwitchHasDefault = 1;
    }
    else if (pNode->m_nType == NODE_CASE)
    {
        int nLabel;
        if (pNode->m_pLeft == 0)
            return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
        if (pNode->m_pLeft->m_nType == NODE_NEGATE)
        {
            if (pNode->m_pLeft->m_pLeft == 0 || pNode->m_pLeft->m_pLeft->m_nType != NODE_CONSTANT_INTEGER)
                return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
            nLabel = -pNode->m_pLeft->m_pLeft->m_nValue;
        }
        else if (pNode->m_pLeft->m_nType == NODE_CONSTANT_INTEGER)
        {
            nLabel = pNode->m_pLeft->m_nValue;
        }
        else
        {
            return OutputWalkTreeError(-SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER, pNode);
        }

        for (int i = 0; i < m_nSwitchLabelCount; i++)
        {
            if (m_pSwitchLabels[i] == nLabel)
                return OutputWalkTreeError(-SCRIPT_ERROR_MULTIPLE_CASE_CONSTANT_STATEMENTS_WITHIN_SWITCH, pNode);
        }

        if (m_nSwitchLabelCount >= m_nSwitchLabelCapacity)
        {
            int nNewCapacity = m_nSwitchLabelCapacity * 2;
            int* pNewLabels = new int[nNewCapacity];
            int i = 0;
            while (i < m_nSwitchLabelCount)
            {
                pNewLabels[i] = m_pSwitchLabels[i];
                i++;
            }
            m_nSwitchLabelCapacity = nNewCapacity;
            if (m_pSwitchLabels != 0)
                delete[] m_pSwitchLabels;
            m_pSwitchLabels = pNewLabels;
        }
        m_pSwitchLabels[m_nSwitchLabelCount] = nLabel;
        m_nSwitchLabelCount = m_nSwitchLabelCount + 1;

        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(m_pCompiledCode + m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH,
                        "CPTOPSP %08x,%04x\n", -4, 4);
                nLen = 35;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CPTOPSP;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_STACK_OPERATION;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(-4 >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(-4 >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(-4 >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(-4);
                m_pCompiledCode[m_nCompiledCodeLocation + 6] = 0;
                m_pCompiledCode[m_nCompiledCodeLocation + 7] = 4;
                nLen = 8;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 8;
        }

        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(m_pCompiledCode + m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH,
                        "CONSTI %08x\n", nLabel);
                nLen = 29;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_CONST;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER;
                m_pCompiledCode[m_nCompiledCodeLocation + 2] = static_cast<char>(nLabel >> 24);
                m_pCompiledCode[m_nCompiledCodeLocation + 3] = static_cast<char>(nLabel >> 16);
                m_pCompiledCode[m_nCompiledCodeLocation + 4] = static_cast<char>(nLabel >> 8);
                m_pCompiledCode[m_nCompiledCodeLocation + 5] = static_cast<char>(nLabel);
                nLen = 6;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 6;
        }

        {
            int nLen;
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
                sprintf(m_pCompiledCode + m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH, "EQUALII\n");
                nLen = 21;
            }
            else
            {
                m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_EQUAL;
                m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_INTEGER_INTEGER;
                nLen = 2;
            }
            m_nCompiledCodeLocation += nLen;
            m_nBinaryCodeOffset += 2;
        }

        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(m_pCompiledCode + m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH, "JNZ _SC_%08x_%08x\n",
                    nLabel, m_nSwitchIdentifier);
            m_nCompiledCodeLocation += 39;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JNZ;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            CExoString sLabel;
            sLabel.Format(const_cast<char*>("_SC_%08x_%08x"), nLabel, static_cast<int>(m_nSwitchIdentifier));
            AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
            m_nCompiledCodeLocation += 6;
        }
        m_nBinaryCodeOffset += 6;
    }

    nRet = TraverseTreeForSwitchLabels(pNode->m_pRight);
    return nRet < 0 ? nRet : 0;
}

// Fresh label table, then walks the switch body to collect and emit its case labels.
int CScriptCompilerInternal::GenerateCodeForSwitchLabels(CScriptParseTreeNode* pNode)
{
    InitializeSwitchLabelList();
    if (pNode != 0) {
        int nRet = TraverseTreeForSwitchLabels(pNode->m_pRight);
        if (nRet < 0)
            return nRet;
    }
    ClearSwitchLabelList();
    return 0;
}

// Emits the tail of a switch: the jump to the default case, then discards the collected
// labels.
void CScriptCompilerInternal::ClearSwitchLabelList()
{
    if (m_bSwitchHasDefault == 1)
    {
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation] + SYMBOLIC_ADDRESS_WIDTH, "JMP _SC_DEFAULT_%08x\n",
                    static_cast<int>(m_nSwitchIdentifier));
            m_nCompiledCodeLocation += 38;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            CExoString sLabel;
            sLabel.Format(const_cast<char*>("_SC_DEFAULT_%08x"), static_cast<int>(m_nSwitchIdentifier));
            AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
            m_nCompiledCodeLocation += 6;
        }
    }
    else
    {
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();
            sprintf(&m_pCompiledCode[m_nCompiledCodeLocation] + SYMBOLIC_ADDRESS_WIDTH, "JMP _BR_%08x\n",
                    static_cast<int>(m_nSwitchIdentifier));
            m_nCompiledCodeLocation += 30;
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_JMP;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = NCS_TYPE_NONE;
            CExoString sLabel;
            sLabel.Format(const_cast<char*>("_BR_%08x"), static_cast<int>(m_nSwitchIdentifier));
            AddSymbolToQueryList(sLabel, m_nCompiledCodeLocation + 2);
            m_nCompiledCodeLocation += 6;
        }
    }

    m_bSwitchHasDefault = 0;
    m_nSwitchLabelCount = 0;
    m_nBinaryCodeOffset += 6;
    m_nSwitchLabelCapacity = SWITCH_LABEL_COUNT;
    if (m_pSwitchLabels != 0)
        delete[] m_pSwitchLabels;
    m_pSwitchLabels = 0;
}

// Pushes one entry on the subroutine stack, doubling the table when it is full.
void CScriptCompilerInternal::PushSRStack(int nType, int nSubType, int nLocation, CScriptParseTreeNode* pNode)
{
    int nCapacity = m_nSubroutineStackCapacity;
    if (m_nSubroutineStackSize + 1 >= nCapacity) {
        int nNewCapacity = nCapacity * 2;
        CScriptSRStackEntry* pNewEntries = new CScriptSRStackEntry[nNewCapacity];
        for (int i = 0; i < nCapacity; i++) {
            pNewEntries[i].m_nType = m_pSubroutineStack[i].m_nType;
            pNewEntries[i].m_nSubType = m_pSubroutineStack[i].m_nSubType;
            pNewEntries[i].m_nLocation = m_pSubroutineStack[i].m_nLocation;
            pNewEntries[i].m_pNode = m_pSubroutineStack[i].m_pNode;
            pNewEntries[i].m_pReturnNode = m_pSubroutineStack[i].m_pReturnNode;
        }
        m_nSubroutineStackCapacity = nNewCapacity;
        delete[] m_pSubroutineStack;
        m_pSubroutineStack = pNewEntries;
    }
    m_nSubroutineStackSize = m_nSubroutineStackSize + 1;
    CScriptSRStackEntry& entry = m_pSubroutineStack[m_nSubroutineStackSize];
    entry.m_nType = nType;
    entry.m_nSubType = nSubType;
    entry.m_nLocation = nLocation;
    entry.m_pNode = pNode;
    entry.m_pReturnNode = 0;
}
// Allocates and initialises one parse-tree node.
CScriptParseTreeNode* CScriptCompilerInternal::CreateScriptParseTreeNode(int nType,
                                                                       CScriptParseTreeNode* pLeft,
                                                                       CScriptParseTreeNode* pRight)
{
    CScriptParseTreeNode* pNode = new CScriptParseTreeNode;
    pNode->m_nType = nType;
    pNode->m_pLeft = pLeft;
    pNode->m_pRight = pRight;
    pNode->m_nLine = m_nLine;
    pNode->m_nColumn = m_nColumn;
    return pNode;
}


int CScriptCompilerInternal::ParseCharacterLeftBrace()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_LEFT_BRACE;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterRightBrace()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_RIGHT_BRACE;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterLeftBracket()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_LEFT_BRACKET;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterRightBracket()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_RIGHT_BRACKET;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterLeftSquareBracket()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_LEFT_SQUARE;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterRightSquareBracket()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_RIGHT_SQUARE;
    return HandleToken();
}

// Handles '>', the one ParseCharacter* that reads as a small state machine, for '>=', '>>' and
// '>>>'.
int CScriptCompilerInternal::ParseCharacterRightAngle(int nChar)
{
    int nResult;
    if (m_nTokenType == TOKEN_NONE) {
        if (nChar == '=') {
            m_nTokenType = TOKEN_GREATER_EQUAL;
            nResult = HandleToken();
            if (nResult < 0)
                return nResult;
            return 1;
        }
        if (nChar == '>') {
            m_nTokenType = TOKEN_SHIFT_RIGHT;
            return 0;
        }
        m_nTokenType = TOKEN_GREATER_THAN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 0;
    }
    if (m_nTokenType == TOKEN_SHIFT_RIGHT) {
        if (nChar == '>') {
            m_nTokenType = TOKEN_TRIPLE_SHIFT_RIGHT;
            return 0;
        }
        if (nChar == '=') {
            m_nTokenType = TOKEN_SHIFT_RIGHT_ASSIGN;
            nResult = HandleToken();
            if (nResult < 0)
                return nResult;
            return 1;
        }
        m_nTokenType = TOKEN_SHIFT_RIGHT;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 0;
    }
    if (m_nTokenType == TOKEN_TRIPLE_SHIFT_RIGHT) {
        if (nChar == '=') {
            m_nTokenType = TOKEN_TRIPLE_SHIFT_RIGHT_ASSIGN;
            nResult = HandleToken();
            if (nResult < 0)
                return nResult;
            return 1;
        }
        m_nTokenType = TOKEN_TRIPLE_SHIFT_RIGHT;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 0;
    }
    return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
}
int CScriptCompilerInternal::ParseCharacterLeftAngle(int nChar)
{
    int nResult;
    if (m_nTokenType == TOKEN_NONE) {
        if (nChar == '=') {
            m_nTokenType = TOKEN_LESS_EQUAL;
            nResult = HandleToken();
            if (nResult < 0)
                return nResult;
            return 1;
        }
        if (nChar == '<') {
            m_nTokenType = TOKEN_SHIFT_LEFT;
            return 0;
        }
        m_nTokenType = TOKEN_LESS_THAN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 0;
    }
    if (m_nTokenType == TOKEN_SHIFT_LEFT) {
        if (nChar == '=') {
            m_nTokenType = TOKEN_SHIFT_LEFT_ASSIGN;
            nResult = HandleToken();
            if (nResult < 0)
                return nResult;
            return 1;
        }
        m_nTokenType = TOKEN_SHIFT_LEFT;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 0;
    }
    return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
}

int CScriptCompilerInternal::ParseCharacterSemicolon()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_SEMICOLON;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterComma()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_COMMA;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterTilde()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_TILDE;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterQuestionMark()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_QUESTION_MARK;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterColon()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_COLON;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterCarat(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_CARAT_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_CARAT;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterEqualSign(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_EQUAL;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_ASSIGN;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterHyphen(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_MINUS_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    if (nChar == '-') {
        m_nTokenType = TOKEN_MINUS_MINUS;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_MINUS;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterPlusSign(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_PLUS_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    if (nChar == '+') {
        m_nTokenType = TOKEN_PLUS_PLUS;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_PLUS;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

// Handles '/', the only family member with an arm that does not run through the shared token
// tail.
int CScriptCompilerInternal::ParseCharacterSlash(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '*') {
        m_nTokenType = TOKEN_BLOCK_COMMENT;
        return 1;
    }
    if (nChar == '/') {
        m_nTokenType = TOKEN_LINE_COMMENT;
        return 1;
    }
    if (nChar == '=') {
        m_nTokenType = TOKEN_SLASH_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_DIVIDE;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterAmpersand(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '&') {
        m_nTokenType = TOKEN_LOGICAL_AND;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    if (nChar == '=') {
        m_nTokenType = TOKEN_AMPERSAND_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_AMPERSAND;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterVerticalBar(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '|') {
        m_nTokenType = TOKEN_LOGICAL_OR;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    if (nChar == '=') {
        m_nTokenType = TOKEN_VERTICALBAR_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_VERTICAL_BAR;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}


int CScriptCompilerInternal::ParseCharacterAsterisk(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_ASTERISK_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_ASTERISK;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterExclamationPoint(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_NOT_EQUAL;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_EXCLAMATION;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

int CScriptCompilerInternal::ParseCharacterPercentSign(int nChar)
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    int nResult;
    if (nChar == '=') {
        m_nTokenType = TOKEN_PERCENT_ASSIGN;
        nResult = HandleToken();
        if (nResult < 0)
            return nResult;
        return 1;
    }
    m_nTokenType = TOKEN_PERCENT;
    nResult = HandleToken();
    if (nResult < 0)
        return nResult;
    return 0;
}

// Runs while a string token is live; nChar is the character being consumed.
int CScriptCompilerInternal::ParseStringCharacter(int nChar, int nEscaped)
{
    if (m_nTokenType != TOKEN_STRING)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    if (nChar == '"')
        return HandleToken();
    if (nChar == '\\') {
        if (nEscaped == 'n') {
            m_szTokenText[m_nTokenLength] = '\n';
            int nLength = m_nTokenLength;
            m_nTokenLength = nLength + 1;
            return nLength > MAX_TOKEN_LENGTH - 2 ? -SCRIPT_ERROR_TOKEN_TOO_LONG : 1;
        }
        return 0;
    }
    if (nChar == '\n')
        return -SCRIPT_ERROR_UNTERMINATED_STRING_CONSTANT;
    m_szTokenText[m_nTokenLength] = static_cast<char>(nChar);
    int nLength = m_nTokenLength;
    m_nTokenLength = nLength + 1;
    return nLength > MAX_TOKEN_LENGTH - 2 ? -SCRIPT_ERROR_TOKEN_TOO_LONG : 0;
}

// Unlike its siblings this one returns 0 in its own arm instead of running the shared tail.
int CScriptCompilerInternal::ParseCharacterQuotationMark()
{
    if (m_nTokenType != TOKEN_STRING) {
        if (m_nTokenType != TOKEN_NONE)
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        m_nTokenType = TOKEN_STRING;
        m_nTokenLength = 0;
        return 0;
    }
    return HandleToken();
}

// Runs while a comment token is live. m_nTokenLength doubles as a "just saw the opening
// delimiter" flag.
int CScriptCompilerInternal::ParseCommentedOutCharacter(int nChar)
{
    if (m_nTokenType == TOKEN_LINE_COMMENT && nChar == '\n')
        TokenInitialize();

    if (m_nTokenType != TOKEN_BLOCK_COMMENT)
        return 0;

    if (nChar == '*') {
        if (m_nTokenLength != 0)
            return 0;
        m_nTokenLength = 1;
        return 0;
    }
    if (nChar == '/') {
        if (m_nTokenLength != 1)
            return 0;
        TokenInitialize();
        return 0;
    }
    m_nTokenLength = 0;
    return 0;
}

// Empties the subroutine stack without freeing it.
void CScriptCompilerInternal::DeleteCompileStack()
{
    for (int64_t i = 0; i <= m_nSubroutineStackSize; i++) {
        if (m_pSubroutineStack[i].m_pNode != 0)
            DeleteParseTree(1, m_pSubroutineStack[i].m_pNode);
        if (m_pSubroutineStack[i].m_pReturnNode != 0)
            DeleteParseTree(1, m_pSubroutineStack[i].m_pReturnNode);
    }
}

// Walks the source buffer one character at a time, dispatching each to the ParseCharacter*
// handlers.
int CScriptCompilerInternal::ParseSource(char* pSource, int nLength)
{
    if (m_nIdentifierCount == 0)
    {
        int nResult = ParseIdentifierFile();
        if (nResult < 0)
            return nResult;
    }

    if (nLength < 1)
    {
    }
    else
    {
        int nChar = pSource[0];
        int nNext;
        if (nLength == 1)
            nNext = -1;
        else
            nNext = pSource[1];

        int nIndex = 2;
        while (nChar != -1)
        {
            int nResult = ParseNextCharacter(nChar, nNext);
            if (nResult < 0)
                return PrintParseSourceError(nChar, nResult);

            for (int nLoop = nResult + 1; nLoop > 0; nLoop--)
            {
                if (nChar == '\n')
                {
                    m_nLine++;
                    m_nColumn = 1;
                }
                else
                {
                    m_nColumn++;
                }

                nChar = nNext;
                if (nIndex < nLength)
                    nNext = pSource[nIndex];
                else
                    nNext = -1;
                nIndex++;
            }
        }
    }

    int nResult = ParseNextCharacter(-1, -1);
    if (nResult < 0)
        return PrintParseSourceError(-1, nResult);
    return 0;
}


__attribute__((noinline))
void CScriptCompilerInternal::InitializeFinalCode()
{
    if (m_pCompiledCode != 0)
        delete[] m_pCompiledCode;

    char* pCode = new char[CODE_BUFFER_SIZE];
    m_pCompiledCode = pCode;
    m_nCompiledCodeSize = CODE_BUFFER_SIZE;
    sprintf(pCode, "NCS V1.0");

    m_nBinaryCodeOffset = NCS_HEADER_SIZE;
    m_nCompiledCodeLocation = m_bCompileSymbolicOutput ? SYMBOLIC_HEADER_SIZE : NCS_HEADER_SIZE;
}

__attribute__((noinline))
void CScriptCompilerInternal::FinalizeFinalCode()
{
    if (m_bCompileSymbolicOutput)
    {
        sprintf(&m_pCompiledCode[8], "T %08x\n", m_nCompiledCodeLocation);
        m_pCompiledCode[SYMBOLIC_HEADER_SIZE - 1] = '\n';
    }
    else
    {
        m_pCompiledCode[8] = 'B';
        m_pCompiledCode[9]  = static_cast<char>(m_nCompiledCodeLocation >> 24);
        m_pCompiledCode[10] = static_cast<char>(m_nCompiledCodeLocation >> 16);
        m_pCompiledCode[11] = static_cast<char>(m_nCompiledCodeLocation >> 8);
        m_pCompiledCode[12] = static_cast<char>(m_nCompiledCodeLocation);
    }
}

// Appends the width-8 hex form of the current instruction's byte offset.
void CScriptCompilerInternal::PrintBinaryAddress()
{
    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
}

// Pushes a variable's stack type code onto the type stack and, in symbolic mode, emits its
// declaration line.
void CScriptCompilerInternal::AddVariableToStack(int nType, const CExoString& sName, int nFlags)
{
    if (static_cast<unsigned>(nType - KEYWORD_INT) >= 4 && static_cast<unsigned>(nType - KEYWORD_ENGINE_STRUCTURE0) > 9)
    {
        if (nType == KEYWORD_STRUCT)
            AddStructureToStack(sName, nFlags);
        return;
    }

    int nStackType = NCS_TYPE_FLOAT;
    if (nType == KEYWORD_INT)
        nStackType = NCS_TYPE_INTEGER;
    if (static_cast<unsigned>(nType - KEYWORD_INT) >= 2)
        nStackType = NCS_TYPE_STRING;
    if (static_cast<unsigned>(nType - KEYWORD_INT) >= 3)
        nStackType = NCS_TYPE_OBJECT;
    if (static_cast<unsigned>(nType - KEYWORD_ENGINE_STRUCTURE0) <= 9)
        nStackType = nType - (KEYWORD_ENGINE_STRUCTURE0 - NCS_TYPE_ENGINE_STRUCTURE0);

    m_aStackTypes[m_nStackCurrentDepth] = static_cast<char>(nStackType);
    m_nStackCurrentDepth++;

    if (nFlags == 1)
    {
        if (m_bCompileSymbolicOutput != 0)
        {
            PrintBinaryAddress();

            if (nType == KEYWORD_INT)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDI\n");
                m_nCompiledCodeLocation += 20;
            }
            else if (nType == KEYWORD_OBJECT)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDO\n");
                m_nCompiledCodeLocation += 20;
            }
            else if (nType == KEYWORD_STRING)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDS\n");
                m_nCompiledCodeLocation += 20;
            }
            else if (nType == KEYWORD_FLOAT)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDF\n");
                m_nCompiledCodeLocation += 20;
            }
            else if (static_cast<unsigned>(nType - KEYWORD_ENGINE_STRUCTURE0) <= 9)
            {
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDP%1d\n",
                        nType - KEYWORD_ENGINE_STRUCTURE0);
                m_nCompiledCodeLocation += 21;
            }
        }
        else
        {
            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RSADD;
            m_pCompiledCode[m_nCompiledCodeLocation + 1] = static_cast<char>(nStackType);
            m_nCompiledCodeLocation += 2;
        }
        m_nBinaryCodeOffset += 2;
    }
}

// Pushes every field of the named structure onto the type stack.
void CScriptCompilerInternal::AddStructureToStack(const CExoString& sName, int nFlags)
{
    for (int i = 0; i < m_nStructureCount; i++)
    {
        if (sName == m_pStructureList[i].m_sName)
        {
            for (int j = m_pStructureList[i].m_nFirstField; j <= m_pStructureList[i].m_nLastField; j++)
            {
                unsigned char nType = static_cast<unsigned char>(m_pStructureFieldList[j].m_nType);

                if (static_cast<unsigned char>(nType - KEYWORD_INT) < 4 || static_cast<unsigned char>(nType - KEYWORD_ENGINE_STRUCTURE0) <= 9)
                {
                    int nStackType = NCS_TYPE_FLOAT;
                    if (nType == KEYWORD_INT)
                        nStackType = NCS_TYPE_INTEGER;
                    if (static_cast<unsigned char>(nType - KEYWORD_INT) >= 2)
                        nStackType = NCS_TYPE_STRING;
                    if (static_cast<unsigned char>(nType - KEYWORD_INT) >= 3)
                        nStackType = NCS_TYPE_OBJECT;
                    if (static_cast<unsigned char>(nType - KEYWORD_ENGINE_STRUCTURE0) <= 9)
                        nStackType = nType - (KEYWORD_ENGINE_STRUCTURE0 - NCS_TYPE_ENGINE_STRUCTURE0);

                    m_aStackTypes[m_nStackCurrentDepth] = static_cast<char>(nStackType);
                    m_nStackCurrentDepth++;

                    if (nFlags == 1)
                    {
                        if (m_bCompileSymbolicOutput != 0)
                        {
                            PrintBinaryAddress();

                            if (nType == KEYWORD_INT)
                            {
                                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDI\n");
                                m_nCompiledCodeLocation += 20;
                            }
                            else if (nType == KEYWORD_OBJECT)
                            {
                                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDO\n");
                                m_nCompiledCodeLocation += 20;
                            }
                            else if (nType == KEYWORD_STRING)
                            {
                                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDS\n");
                                m_nCompiledCodeLocation += 20;
                            }
                            else if (nType == KEYWORD_FLOAT)
                            {
                                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "RSADDF\n");
                                m_nCompiledCodeLocation += 20;
                            }
                            else if (static_cast<unsigned char>(nType - KEYWORD_ENGINE_STRUCTURE0) <= 9)
                            {
                                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH],
                                        "RSADDP%1d\n", nType - KEYWORD_ENGINE_STRUCTURE0);
                                m_nCompiledCodeLocation += 21;
                            }
                        }
                        else
                        {
                            m_pCompiledCode[m_nCompiledCodeLocation] = NCS_OP_RSADD;
                            m_pCompiledCode[m_nCompiledCodeLocation + 1] = static_cast<char>(nStackType);
                            m_nCompiledCodeLocation += 2;
                        }
                        m_nBinaryCodeOffset += 2;
                    }
                }
                else if (nType == KEYWORD_STRUCT)
                {
                    AddStructureToStack(m_pStructureFieldList[j].m_sStructureName, nFlags);
                }
            }
        }
    }
}

int CScriptCompilerInternal::CleanUpAfterCompile(int nMode, CScriptParseTreeNode* pNode)
{
    DeleteParseTree(0, pNode);

    if (nMode < 0) {
        if (m_pCompiledCode != 0) {
            delete[] m_pCompiledCode;
            m_pCompiledCode = 0;
        }
        m_nCompiledCodeLocation = 0;
    }

    DeleteParseTree(0, m_pGlobalVariableParseTree);
    m_pGlobalVariableParseTree = 0;
    ClearUserDefinedIdentifiers();
#if KOTOR1
    if (m_pSymbolQueryList != 0) {
        delete[] m_pSymbolQueryList;
        m_pSymbolQueryList = 0;
    }
    m_nSymbolQueryListCapacity = 0;
    m_nSymbolQueryListCount = 0;
    if (m_pSymbolLabelList != 0) {
        delete[] m_pSymbolLabelList;
        m_pSymbolLabelList = 0;
    }
    m_nSymbolLabelListCapacity = 0;
    m_nSymbolLabelListCount = 0;
#else
    ClearAllSymbolLists();
#endif
    return nMode;
}

int CScriptCompilerInternal::AddToGlobalVariableList(CScriptParseTreeNode* pNode)
{
    if (m_pGlobalVariableParseTree == 0)
        m_pGlobalVariableParseTree = CreateScriptParseTreeNode(NODE_GLOBAL_VARIABLES, 0, 0);

    CScriptParseTreeNode* pNew = CreateScriptParseTreeNode(NODE_STATEMENT, pNode, 0);

    CScriptParseTreeNode* pLast = m_pGlobalVariableParseTree;
    CScriptParseTreeNode* pIter = m_pGlobalVariableParseTree;
    do {
        pLast = pIter;
        pIter = pIter->m_pRight;
    } while (pIter != 0);
    pLast->m_pRight = pNew;
    return 0;
}

// Linear search of the built-in type table by name.
int CScriptCompilerInternal::GetStructureSize(const CExoString& sName)
{
    for (int64_t i = 0; i < m_nStructureCount; i++) {
        if (sName == m_pStructureList[i].m_sName)
            return m_pStructureList[i].m_nSize;
    }
    return 0;
}

// Looks a field up in the built-in type table: finds the type by name, then the field inside it.
int CScriptCompilerInternal::GetStructureField(const CExoString& sStructureName,
                                               const CExoString& sFieldName)
{
    if (m_nStructureCount < 1)
        return -SCRIPT_ERROR_UNDEFINED_STRUCTURE;

    for (int64_t i = 0; i < m_nStructureCount; i++) {
        if (sStructureName == m_pStructureList[i].m_sName) {
            int nFirstField = m_pStructureList[i].m_nFirstField;
            if (nFirstField > m_pStructureList[i].m_nLastField)
                return -SCRIPT_ERROR_UNDEFINED_FIELD_IN_STRUCTURE;

            for (int64_t j = nFirstField; j <= m_pStructureList[i].m_nLastField; j++) {
                if (sFieldName == m_pStructureFieldList[j].m_sName)
                    return static_cast<int>(j);
            }
            return -SCRIPT_ERROR_UNDEFINED_FIELD_IN_STRUCTURE;
        }
    }
    return -SCRIPT_ERROR_UNDEFINED_STRUCTURE;
}

// Identifier-table lookup by name; compares the cached name length before the text.
int CScriptCompilerInternal::GetIdentifierByName(const CExoString& sName)
{
#if KOTOR1
    int nNameLength = sName.GetLength();
#else
    int nNameLength = sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0;
#endif

    for (int64_t i = 0; i <= m_nIdentifierCount; i++) {
        if (m_pIdentifierList[i].m_nNameLength != nNameLength)
            continue;
        if (m_pIdentifierList[i].m_sName == sName)
            return static_cast<int>(i);
    }
    return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
}

// Tears down the scratch state for every path that abandons a compile: releases the parse tree and
// the type/label scratch state.
int CScriptCompilerInternal::CleanUpDuringCompile(int nMode)
{
    DeleteCompileStack();

    --m_nIncludeFileDepth;
    if (m_nIncludeFileDepth >= 1)
        ShutdownIncludeFile(m_nIncludeFileDepth);

    DeleteParseTree(0, m_pGlobalVariableParseTree);
    m_pGlobalVariableParseTree = 0;
    ClearUserDefinedIdentifiers();
    ClearAllSymbolLists();
    return nMode;
}

// Does this if/else subtree return on every path? Recursive over the parse tree.
int CScriptCompilerInternal::FoundReturnStatementOnAllBranches(CScriptParseTreeNode* pNode)
{
    if (pNode == 0)
        return 0;

    switch (pNode->m_nType) {
    case NODE_COMPOUND_STATEMENT:
    case NODE_STATEMENT:
    case NODE_STATEMENT_LIST:
    case NODE_IF_BLOCK:
        if (FoundReturnStatementOnAllBranches(pNode->m_pLeft) == 1)
            return 1;
        return FoundReturnStatementOnAllBranches(pNode->m_pRight) == 1;
    case NODE_IF_CHOICE:
        if (FoundReturnStatementOnAllBranches(pNode->m_pLeft) != 1)
            break;
        if (FoundReturnStatementOnAllBranches(pNode->m_pRight) != 1)
            break;
        return 1;
    case NODE_RETURN:
        return 1;
    }
    return 0;
}

// Reports a parse failure of the identifier-specification file, with the failing line.
int CScriptCompilerInternal::PrintParseIdentifierFileError(int nLine, int nError)
{
    K2_UNUSED(nLine);
    STR_RES strRes;
    g_pTlkTable->Fetch(static_cast<unsigned int>(-nError), strRes, CTlkTable::GENDER_MALE);

    CExoString sFile(m_includeFiles[0].m_sFileName);
    OutputError(nError, sFile, m_nLine, strRes.text);
    m_includeFiles[0].m_script.UnloadScript();
    return nError;
}

// Reports a codegen failure, formatted with the source position of the failing node.
int CScriptCompilerInternal::OutputWalkTreeError(int nError, CScriptParseTreeNode* pNode)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(static_cast<unsigned int>(-nError), strRes, CTlkTable::GENDER_MALE);

    CExoString sFile(m_includeFiles[m_nIncludeFileDepth].m_sFileName);
    int nLine = 0;
    if (pNode != 0)
        nLine = pNode->m_nLine;

    return OutputError(nError, m_includeFiles[m_nIncludeFileDepth].m_sFileName, nLine, strRes.text);
}

// Reports an identifier error; the talk-table text is re-formatted as "<text> (<name>)".
int CScriptCompilerInternal::OutputIdentifierError(const CExoString& sName, int nError)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(static_cast<unsigned int>(-nError), strRes, CTlkTable::GENDER_MALE);

    CExoString sFile(m_includeFiles[m_nIncludeFileDepth].m_sFileName);
    CExoString sMessage;
    sMessage.Format(const_cast<char*>("%s (%s)"), strRes.text.CStr(), sName.CStr());

    return OutputError(nError, m_includeFiles[m_nIncludeFileDepth].m_sFileName, 0, sMessage);
}

int CScriptCompilerInternal::AddSymbolToQueryList(const CExoString& sLabel, int nLocation)
{
    if (m_nSymbolQueryListCapacity == m_nSymbolQueryListCount)
    {
        m_nSymbolQueryListCapacity = m_nSymbolQueryListCapacity + SYMBOL_LIST_GROWTH;
        K2_ASSUME(m_nSymbolQueryListCount < m_nSymbolQueryListCapacity);   // the old capacity was the count
        CScriptCompilerSymbolListEntry* pNewEntries =
            new CScriptCompilerSymbolListEntry[m_nSymbolQueryListCapacity];

        if (m_pSymbolQueryList != 0)
        {
            for (int i = 0; i < m_nSymbolQueryListCount; i++)
            {
                pNewEntries[i].m_sLabel = m_pSymbolQueryList[i].m_sLabel;
                pNewEntries[i].m_nCodeLocation = m_pSymbolQueryList[i].m_nCodeLocation;
            }
            delete[] m_pSymbolQueryList;
        }
        m_pSymbolQueryList = pNewEntries;
    }

    K2_ASSUME(m_nSymbolQueryListCount < m_nSymbolQueryListCapacity);   // the count only grows by one, after the capacity passes it
    m_pSymbolQueryList[m_nSymbolQueryListCount].m_sLabel = sLabel;
    m_pSymbolQueryList[m_nSymbolQueryListCount].m_nCodeLocation = nLocation;
    m_nSymbolQueryListCount++;

    return 0;
}

// Files the same entry type as AddSymbolToQueryList, its label-list twin, in the list ResolveLabels
// reads.
int CScriptCompilerInternal::AddSymbolToLabelList(const CExoString& sLabel, int nLocation)
{
    if (m_nSymbolLabelListCapacity == m_nSymbolLabelListCount)
    {
        m_nSymbolLabelListCapacity = m_nSymbolLabelListCapacity + SYMBOL_LIST_GROWTH;
        K2_ASSUME(m_nSymbolLabelListCount < m_nSymbolLabelListCapacity);   // the old capacity was the count
        CScriptCompilerSymbolListEntry* pNewEntries =
            new CScriptCompilerSymbolListEntry[m_nSymbolLabelListCapacity];

        if (m_pSymbolLabelList != 0)
        {
            for (int i = 0; i < m_nSymbolLabelListCount; i++)
            {
                pNewEntries[i].m_sLabel = m_pSymbolLabelList[i].m_sLabel;
                pNewEntries[i].m_nCodeLocation = m_pSymbolLabelList[i].m_nCodeLocation;
            }
            delete[] m_pSymbolLabelList;
        }
        m_pSymbolLabelList = pNewEntries;
    }

    K2_ASSUME(m_nSymbolLabelListCount < m_nSymbolLabelListCapacity);   // the count only grows by one, after the capacity passes it
    m_pSymbolLabelList[m_nSymbolLabelListCount].m_sLabel = sLabel;
    m_pSymbolLabelList[m_nSymbolLabelListCount].m_nCodeLocation = nLocation;
    m_nSymbolLabelListCount++;

    return 0;
}

// Reserves this identifier's slice of the resolved image: looks the name up and assigns it a
// location.
int CScriptCompilerInternal::ValidateLocationOfIdentifier(const CExoString& sName)
{
    int nIndex = GetIdentifierByName(sName);
    if (nIndex < 0)
        return OutputIdentifierError(sName, nIndex);

    if (m_pIdentifierList[nIndex].m_nOutputStart != -1)
        return 0;

    int nCodeLength = m_pIdentifierList[nIndex].m_nCodeEnd - m_pIdentifierList[nIndex].m_nCodeOffset;
    m_pIdentifierList[nIndex].m_nOutputStart = m_nResolvedCodeSize;
    m_pIdentifierList[nIndex].m_nOutputEnd = m_nResolvedCodeSize + nCodeLength;
    m_nResolvedCodeSize = m_pIdentifierList[nIndex].m_nOutputEnd;

    CExoString sTarget;
    int nResult = 0;
    for (int64_t j = 0; j < m_nSymbolQueryListCount; j++) {
        if (m_pSymbolQueryList[j].m_nCodeLocation < m_pIdentifierList[nIndex].m_nCodeOffset)
            continue;
        if (m_pSymbolQueryList[j].m_nCodeLocation >= m_pIdentifierList[nIndex].m_nCodeEnd)
            continue;
        if (!(m_pSymbolQueryList[j].m_sLabel.Left(2) == "FE"))
            continue;

        CExoString& sSymbol = m_pSymbolQueryList[j].m_sLabel;
#if KOTOR1
        int nSymbolLength = sSymbol.GetLength();
#else
        int nSymbolLength = sSymbol.m_sString ? static_cast<int>(strlen(sSymbol.m_sString)) : 0;
#endif
        sTarget = sSymbol.Right(nSymbolLength - 3);
        nResult = ValidateLocationOfIdentifier(sTarget);
        if (nResult < 0)
            break;
        nResult = 0;
    }

    return nResult;
}

// Runs the second half of label binding: patches every unresolved reference that
// AddSymbolToQueryList recorded.
int CScriptCompilerInternal::ResolveLabels()
{
    CExoString sLabel;

    if (m_nSymbolQueryListCount < 1)
        return 0;

    for (int nQuery = 0; nQuery < m_nSymbolQueryListCount; nQuery++)
    {
        int nOffset = m_pSymbolQueryList[nQuery].m_nCodeLocation;

        if (m_nIdentifierBase > m_nIdentifierCount)
            return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;

        int bFound = 0;
        int nIndex K2_UNSET;
        for (int64_t nId = m_nIdentifierBase; nId <= m_nIdentifierCount; nId++)
        {
            if (nOffset < m_pIdentifierList[nId].m_nCodeOffset)
                continue;
            if (nOffset < m_pIdentifierList[nId].m_nCodeEnd)
            {
                bFound = 1;
                nIndex = static_cast<int>(nId);
            }
        }

        if (bFound == 0)
            return -SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER;

        if (m_pIdentifierList[nIndex].m_nOutputStart == -1)
            continue;

        int nLabelIndex = 0;
        int nValue = 0;

        if (m_nSymbolLabelListCount < 1)
            goto lMissingLabel;

        nValue = m_pIdentifierList[nIndex].m_nCodeOffset
               - (nOffset + m_pIdentifierList[nIndex].m_nOutputStart) + 2;

        for (nLabelIndex = 0; nLabelIndex < m_nSymbolLabelListCount; nLabelIndex++)
        {
            if (m_pSymbolQueryList[nQuery].m_sLabel == m_pSymbolLabelList[nLabelIndex].m_sLabel)
                goto lFoundLabel;
        }
        goto lMissingLabel;

lFoundLabel:

        if (m_pSymbolQueryList[nQuery].m_sLabel.Left(2) == "FE"
            || m_pSymbolQueryList[nQuery].m_sLabel.Left(2) == "FX")
        {
#if KOTOR1
            int nSymbolLength = m_pSymbolQueryList[nQuery].m_sLabel.GetLength();
#else
            int nSymbolLength = m_pSymbolQueryList[nQuery].m_sLabel.m_sString
                    ? static_cast<int>(strlen(m_pSymbolQueryList[nQuery].m_sLabel.m_sString)) : 0;
#endif
            sLabel = m_pSymbolQueryList[nQuery].m_sLabel.Right(nSymbolLength - 3);

            int nIdentifier = GetIdentifierByName(sLabel);
            if (nIdentifier < 0)
                return OutputIdentifierError(sLabel, nIdentifier);
            nIndex = nIdentifier;
        }

        nValue += m_pSymbolLabelList[nLabelIndex].m_nCodeLocation
                + m_pIdentifierList[nIndex].m_nOutputStart
                - m_pIdentifierList[nIndex].m_nCodeOffset;

        m_pCompiledCode[m_pSymbolQueryList[nQuery].m_nCodeLocation]     = static_cast<char>(nValue >> 24);
        m_pCompiledCode[m_pSymbolQueryList[nQuery].m_nCodeLocation + 1] = static_cast<char>(nValue >> 16);
        m_pCompiledCode[m_pSymbolQueryList[nQuery].m_nCodeLocation + 2] = static_cast<char>(nValue >> 8);
        m_pCompiledCode[m_pSymbolQueryList[nQuery].m_nCodeLocation + 3] = static_cast<char>(nValue);

        continue;

lMissingLabel:
        {
#if KOTOR1
            int nSymbolLength = m_pSymbolQueryList[nQuery].m_sLabel.GetLength();
#else
            int nSymbolLength = m_pSymbolQueryList[nQuery].m_sLabel.m_sString
                    ? static_cast<int>(strlen(m_pSymbolQueryList[nQuery].m_sLabel.m_sString)) : 0;
#endif
            sLabel = m_pSymbolQueryList[nQuery].m_sLabel.Right(nSymbolLength - 3);
        }
        return OutputIdentifierError(sLabel, -SCRIPT_ERROR_UNDEFINED_IDENTIFIER);
    }

    return 0;
}

// Walks the identifier table and lays out each live code object in the resolved image.
int CScriptCompilerInternal::DetermineLocationOfCode()
{
    if (m_bOptimizeBinaryCodeLength == 1) {
        m_nResolvedCodeSize = NCS_HEADER_SIZE;
        return ValidateLocationOfIdentifier("#loader");
    }

    m_nResolvedCodeSize = m_nCompiledCodeLocation;

    for (int i = m_nIdentifierBase; i <= m_nIdentifierCount; i++) {
        if (m_pIdentifierList[i].m_nCodeOffset != -1) {
            m_pIdentifierList[i].m_nOutputStart = m_pIdentifierList[i].m_nCodeOffset;
            m_pIdentifierList[i].m_nOutputEnd = m_pIdentifierList[i].m_nCodeEnd;
        }
    }

    return 0;
}

// Rebuilds m_pCompiledCode as the final, relocated image.
__attribute__((noinline))
int CScriptCompilerInternal::WriteResolvedOutput()
{
    int64_t nSize = m_nResolvedCodeSize;
    K2_ASSUME(nSize >= 13);   // the image starts at the 13-byte header (DetermineLocationOfCode)
    char* pNew = new char[nSize];
    __builtin_memset(pNew, 0, nSize);  // idiom-exempt: runtime length (a sized image, no array type)

    __builtin_memcpy(pNew, m_pCompiledCode, 13);  // idiom-exempt: the fixed 13-byte "NCS V1.0"+length prologue has no named type

    for (int64_t i = m_nIdentifierBase; i < m_nIdentifierCount; i++) {
        if (m_pIdentifierList[i].m_nOutputStart == -1)
            continue;
        __builtin_memcpy(&pNew[m_pIdentifierList[i].m_nOutputStart],
               &m_pCompiledCode[m_pIdentifierList[i].m_nCodeOffset],
               m_pIdentifierList[i].m_nOutputEnd - m_pIdentifierList[i].m_nOutputStart);
    }

    if (m_pCompiledCode != 0)
        delete[] m_pCompiledCode;

    m_pCompiledCode = pNew;
    m_nCompiledCodeLocation = m_nResolvedCodeSize;
    m_nCompiledCodeSize = m_nResolvedCodeSize;
    return 0;
}

// Flushes the compiled buffer to the output resource and re-scans the resource manager.
int CScriptCompilerInternal::WriteFinalCodeToFile(const CExoString& sFile)
{
    CExoString sName;
    CExoFile* pFile;
    sName.Format(const_cast<char*>("%s:%s"), m_sOutputAlias.CStr(), sFile.CStr());

    if (m_bCompileSymbolicOutput != 0)
        pFile = new CExoFile(sName, RESTYPE_NCS, CExoString("w"));
    else
        pFile = new CExoFile(sName, RESTYPE_NCS, CExoString("wb"));

    if (pFile->FileOpened())
    {
        pFile->Write(m_pCompiledCode, 1, m_nCompiledCodeLocation);
        delete pFile;

        CExoString sDirectory;
        sDirectory.Format(const_cast<char*>("%s:"), m_sOutputAlias.CStr());
        g_pExoResMan->UpdateResourceDirectory(CExoString(sDirectory.CStr()));

        if (m_pCompiledCode != 0)
            delete[] m_pCompiledCode;
        m_pCompiledCode = 0;
        m_nCompiledCodeLocation = 0;
        return 0;
    }

    delete pFile;
    return -SCRIPT_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING;
}

// Reports a failing ParseSource, then tears the compile down.
int CScriptCompilerInternal::PrintParseSourceError(int nLine, int nError)
{
    K2_UNUSED(nLine);
    STR_RES strRes;
    g_pTlkTable->Fetch(static_cast<unsigned int>(-nError), strRes, CTlkTable::GENDER_MALE);

    CExoString sFile(m_includeFiles[m_nIncludeFileDepth - 1].m_sFileName);
    CExoString sMessage;
    if (nError == -SCRIPT_ERROR_UNDEFINED_IDENTIFIER)
        sMessage.Format(const_cast<char*>("%s (%s)"), strRes.text.CStr(), m_sUndefinedIdentifier.CStr());
    else
        sMessage.Format(const_cast<char*>("%s"), strRes.text.CStr());

    OutputError(nError, sFile, m_nLine, sMessage);
    return CleanUpDuringCompile(nError);
}

CSourceScript::CSourceScript()
{
}

__attribute__((noinline))
void CSourceScript::UnloadScript()
{
    m_pRes->Release();
}

CSourceScript::~CSourceScript() {}

int CScriptCompilerInternal::GenerateIdentifierList()
{
    int nIndex;
    int nValue;
    int i;
    float fSign;
    float fValue;
    int nState;
    int nSign;
    int nSkip;

    switch (m_nTokenType) {
    case KEYWORD_INT:
    case KEYWORD_FLOAT:
    case KEYWORD_STRING:
    case KEYWORD_OBJECT:
    case KEYWORD_ACTION:
    case KEYWORD_VOID:
    case KEYWORD_VECTOR:
    case KEYWORD_ENGINE_STRUCTURE0 + 0:
    case KEYWORD_ENGINE_STRUCTURE0 + 1:
    case KEYWORD_ENGINE_STRUCTURE0 + 2:
    case KEYWORD_ENGINE_STRUCTURE0 + 3:
    case KEYWORD_ENGINE_STRUCTURE0 + 4:
    case KEYWORD_ENGINE_STRUCTURE0 + 5:
    case KEYWORD_ENGINE_STRUCTURE0 + 6:
    case KEYWORD_ENGINE_STRUCTURE0 + 7:
    case KEYWORD_ENGINE_STRUCTURE0 + 8:
    case KEYWORD_ENGINE_STRUCTURE0 + 9:
        if (m_nIdentifierListState == IDLIST_PARAMETERS) {
            nIndex = m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterCount;
            if (nIndex == m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterSpace) {
                nValue = m_pIdentifierList[m_nIdentifierCount - 1].ExpandParameterSpace();
                if (nValue < 0)
                    return nValue;
            }
            m_pIdentifierList[m_nIdentifierCount - 1].m_pParameterTypes[nIndex] = static_cast<char>(m_nTokenType);
            if (m_nTokenType == KEYWORD_VECTOR) {
                m_pIdentifierList[m_nIdentifierCount - 1].m_pParameterTypes[nIndex] = KEYWORD_STRUCT;
                m_pIdentifierList[m_nIdentifierCount - 1].m_pParameterStructureNames[nIndex] = "vector";
            }
            m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterFlags[nIndex] = 0;
            m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterCount = nIndex + 1;
            return 0;
        }
        if (m_nIdentifierListState == IDLIST_IDLE) {
            m_nIdentifierListState = IDLIST_RETURN_TYPE;
            m_nIdentifierListReturnType = m_nTokenType;
            return 0;
        }
        break;

    case TOKEN_NUMBER:
        if (m_nIdentifierListState == IDLIST_ENGINE_NUM_STRUCTURES) {
            if (m_nTokenLength < 1) {
                nValue = 0;
            } else {
                nValue = 0;
                i = 0;
                do {
                    nValue = nValue * 10 + m_szTokenText[i] - '0';
                    i++;
                } while (i < m_nTokenLength);
                if (nValue > 9)
                    nValue = 10;
                else if (nValue < 0)
                    nValue = 1;
            }
            m_nEngineStructureCount = nValue;
            delete[] m_pEngineStructureValid;
            m_pEngineStructureValid = new int[nValue];
            if (nValue > 0)
                __builtin_memset(m_pEngineStructureValid, 0, nValue * sizeof(int));
            delete[] m_pEngineStructureNames;
            m_pEngineStructureNames = new CExoString[nValue];
            m_nIdentifierListState = IDLIST_IDLE;
            return 0;
        }
        if (m_nIdentifierListState == IDLIST_CONSTANT_VALUE && m_bCompileIdentifierConstants == 0)
            return 0;
        nIndex = m_nIdentifierCount - 1;
        if (m_nIdentifierListState == IDLIST_CONSTANT_VALUE) {
            m_nIdentifierListState = IDLIST_CONSTANT_DONE;
            if (m_pIdentifierList[nIndex].m_nReturnType != TOKEN_INTEGER_IDENTIFIER ||
                m_pIdentifierList[nIndex].m_nIdentifierType != IDENTIFIER_TYPE_CONSTANT)
                return -SCRIPT_ERROR_NON_INTEGER_ID_FOR_INTEGER_CONSTANT;
            nState = IDLIST_CONSTANT_DONE;
        } else if (m_nIdentifierListState == IDLIST_PARAMETER_DEFAULT) {
            nState = IDLIST_PARAMETER_DEFAULT;
        } else {
            break;
        }
        nSign = m_pIdentifierList[nIndex].m_nIntegerData;
        if (nSign == -1)
            m_pIdentifierList[nIndex].m_nIntegerData = 0;
        else
            nSign = 1;
        nSkip = (m_szTokenText[0] == '-') ? 1 : 0;
        if (nSkip != 0)
            nSign = -nSign;
        if (m_nTokenLength > nSkip) {
            nValue = 0;
            for (i = nSkip; i < m_nTokenLength; i++)
                nValue = nValue * 10 + m_szTokenText[i] - '0';
        } else {
            nValue = 0;
        }
        if (nState == IDLIST_PARAMETER_DEFAULT) {
            m_nIdentifierListState = IDLIST_PARAMETERS;
            i = m_pIdentifierList[nIndex].m_nParameterCount - 1;
            m_pIdentifierList[nIndex].m_pOptionalParameterFlags[i] = 1;
            m_pIdentifierList[nIndex].m_pOptionalParameterIntegerData[i] = nValue * nSign;
            return 0;
        }
        if (nState != IDLIST_CONSTANT_DONE)
            return 0;
        m_pIdentifierList[nIndex].m_nIntegerData = nValue * nSign;
        m_szTokenText[m_nTokenLength] = '\0';
        m_pIdentifierList[nIndex].m_sStringData.Format(nSign != -1 ? const_cast<char*>("%s") : const_cast<char*>("-%s"), m_szTokenText);
        return 0;

    case TOKEN_FLOAT:
        if (m_bCompileIdentifierConstants == 0 && m_nIdentifierListState == IDLIST_CONSTANT_VALUE)
            return 0;
        nIndex = m_nIdentifierCount - 1;
        if (static_cast<unsigned int>(m_nIdentifierListState - IDLIST_PARAMETER_VECTOR) >= 3) {
            if (m_nIdentifierListState != IDLIST_CONSTANT_VALUE)
                break;
            m_nIdentifierListState = IDLIST_CONSTANT_DONE;
            if (m_pIdentifierList[nIndex].m_nReturnType != TOKEN_FLOAT_IDENTIFIER ||
                m_pIdentifierList[nIndex].m_nIdentifierType != IDENTIFIER_TYPE_CONSTANT)
                return -SCRIPT_ERROR_NON_FLOAT_ID_FOR_FLOAT_CONSTANT;
        }
        if (m_pIdentifierList[nIndex].m_nIntegerData == -1) {
            m_pIdentifierList[nIndex].m_nIntegerData = 0;
            fSign = -1.0f;
        } else {
            fSign = 1.0f;
        }
        fValue = ParseFloatFromTokenString();
        switch (m_nIdentifierListState) {
        case IDLIST_CONSTANT_DONE:
            m_pIdentifierList[nIndex].m_fFloatData = fSign * fValue;
            m_szTokenText[m_nTokenLength] = '\0';
            m_pIdentifierList[nIndex].m_sStringData.Format(fSign < 0.0f ? const_cast<char*>("-%s") : const_cast<char*>("%s"), m_szTokenText);
            return 0;
        case IDLIST_PARAMETER_VECTOR:
            nValue = m_pIdentifierList[nIndex].m_nParameterCount - 1;
            m_pIdentifierList[nIndex].m_pOptionalParameterFlags[nValue] = 1;
            static_cast<float*>(m_pIdentifierList[nIndex].m_pOptionalParameterVectorData)[m_nIdentifierListVectorComponent + nValue * 3] = fSign * fValue;
            return 0;
        case IDLIST_CONSTANT_VECTOR:
            m_pIdentifierList[nIndex].m_fVectorData[m_nIdentifierListVectorComponent] = fSign * fValue;
            return 0;
        case IDLIST_PARAMETER_DEFAULT:
            m_nIdentifierListState = IDLIST_PARAMETERS;
            nValue = m_pIdentifierList[nIndex].m_nParameterCount - 1;
            m_pIdentifierList[nIndex].m_pOptionalParameterFlags[nValue] = 1;
            static_cast<float*>(m_pIdentifierList[nIndex].m_pOptionalParameterFloatData)[nValue] = fSign * fValue;
            return 0;
        default:
            return 0;
        }

    case TOKEN_STRING:
        if (m_bCompileIdentifierConstants == 0 && m_nIdentifierListState == IDLIST_CONSTANT_VALUE)
            return 0;
        nIndex = m_nIdentifierCount - 1;
        if (m_nIdentifierListState == IDLIST_PARAMETER_DEFAULT) {
            m_nIdentifierListState = IDLIST_PARAMETERS;
                nValue = m_pIdentifierList[nIndex].m_nParameterCount - 1;
            m_pIdentifierList[nIndex].m_pOptionalParameterFlags[nValue] = 1;
            m_szTokenText[m_nTokenLength] = '\0';
            m_pIdentifierList[nIndex].m_pOptionalParameterStringData[nValue].Format(const_cast<char*>("%s"), m_szTokenText);
            return 0;
        }
        if (m_nIdentifierListState != IDLIST_CONSTANT_VALUE)
            break;
        m_nIdentifierListState = IDLIST_CONSTANT_DONE;
        if (m_pIdentifierList[nIndex].m_nReturnType != TOKEN_STRING_IDENTIFIER ||
            m_pIdentifierList[nIndex].m_nIdentifierType != IDENTIFIER_TYPE_CONSTANT)
            return -SCRIPT_ERROR_NON_STRING_ID_FOR_STRING_CONSTANT;
        m_szTokenText[m_nTokenLength] = '\0';
        m_pIdentifierList[nIndex].m_sStringData.Format(const_cast<char*>("%s"), m_szTokenText);
        return 0;

    case TOKEN_MINUS:
        if (m_nIdentifierListState != IDLIST_PARAMETER_DEFAULT && m_nIdentifierListState != IDLIST_CONSTANT_VALUE)
            return 0;
        m_pIdentifierList[m_nIdentifierCount - 1].m_nIntegerData = -1;
        return 0;

    case TOKEN_LEFT_BRACKET:
        if (m_nIdentifierListState == IDLIST_NAME) {
            m_nIdentifierListState = IDLIST_PARAMETERS;
            return 0;
        }
        break;

    case TOKEN_SEMICOLON:
        if (m_nIdentifierListState == IDLIST_PARAMETERS) {
            nIndex = m_nIdentifierCount - 1;
                m_pIdentifierList[nIndex].m_nActionId = m_nNextActionId;
            m_nNextActionId = m_nNextActionId + 1;
            m_pIdentifierList[nIndex].m_nNonOptionalParameterCount = 0;
            m_pIdentifierList[nIndex].m_bImplementationInPlace = 1;
            nValue = m_pIdentifierList[nIndex].m_nParameterCount;
            for (i = nValue; i > 0; i--) {
                if (m_pIdentifierList[nIndex].m_pOptionalParameterFlags[i - 1] == 0) {
                    m_pIdentifierList[nIndex].m_nNonOptionalParameterCount = i;
                    break;
                }
            }
        }
        m_nIdentifierListState = IDLIST_IDLE;
        m_nIdentifierBase = m_nIdentifierBase + 1;
        return 0;

    case TOKEN_ASSIGN:
        if (m_nIdentifierListState == IDLIST_PARAMETERS) {
            m_nIdentifierListState = IDLIST_PARAMETER_DEFAULT;
            return 0;
        }
        if (m_nIdentifierListState == IDLIST_NAME) {
            m_nIdentifierListState = IDLIST_CONSTANT_VALUE;
            m_pIdentifierList[m_nIdentifierCount - 1].m_nIdentifierType = IDENTIFIER_TYPE_CONSTANT;
            return 0;
        }
        break;

    case TOKEN_VARIABLE:
        if (m_nIdentifierListState == IDLIST_RETURN_TYPE) {
            nValue = 0;
            m_nIdentifierListState = IDLIST_NAME;
            m_pIdentifierList[m_nIdentifierCount].m_nIdentifierType = IDENTIFIER_TYPE_FUNCTION;
            switch (m_nIdentifierListReturnType) {
            case KEYWORD_INT:
                nValue = TOKEN_INTEGER_IDENTIFIER;
                break;
            case KEYWORD_FLOAT:
                nValue = TOKEN_FLOAT_IDENTIFIER;
                break;
            case KEYWORD_STRING:
                nValue = TOKEN_STRING_IDENTIFIER;
                break;
            case KEYWORD_OBJECT:
                nValue = TOKEN_OBJECT_IDENTIFIER;
                break;
            case KEYWORD_VOID:
                nValue = TOKEN_VOID_IDENTIFIER;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 0:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 1:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 1;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 2:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 2;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 3:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 3;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 4:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 4;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 5:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 5;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 6:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 6;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 7:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 7;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 8:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 8;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 9:
                nValue = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 9;
                break;
            case KEYWORD_VECTOR:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_STRUCTURE_IDENTIFIER;
                m_pIdentifierList[m_nIdentifierCount].m_sReturnStructureName = "vector";
                break;
            default:
                return -SCRIPT_ERROR_PARSING_IDENTIFIER_LIST;
            }
            m_pIdentifierList[m_nIdentifierCount].m_nReturnType = nValue;
            m_szTokenText[m_nTokenLength] = '\0';
            m_pIdentifierList[m_nIdentifierCount].m_sName.Format(const_cast<char*>("%s"), m_szTokenText);
            m_pIdentifierList[m_nIdentifierCount].m_nNameLength = m_nTokenLength;
            m_pIdentifierList[m_nIdentifierCount].m_nCodeOffset = -1;
            m_pIdentifierList[m_nIdentifierCount].m_nCodeEnd = -1;
            m_pIdentifierList[m_nIdentifierCount].m_nOutputStart = -1;
            m_pIdentifierList[m_nIdentifierCount].m_nOutputEnd = -1;
            nValue = m_nIdentifierCount;
            m_nIdentifierCount = nValue + 1;
            return nValue > IDENTIFIER_LIST_COUNT - 2 ? -SCRIPT_ERROR_IDENTIFIER_LIST_FULL : 0;
        }
        if (m_nIdentifierListState == IDLIST_PARAMETERS)
            return 0;
        if (m_nIdentifierListState != IDLIST_ENGINE_STRUCTURE)
            break;
        m_pEngineStructureValid[m_nIdentifierListEngineStructure] = 1;
        m_szTokenText[m_nTokenLength] = '\0';
        m_pEngineStructureNames[m_nIdentifierListEngineStructure].Format(const_cast<char*>("%s"), m_szTokenText);
        m_nIdentifierListState = IDLIST_IDLE;
        return 0;

    case KEYWORD_DEFINE:
        if (m_nIdentifierListState == IDLIST_IDLE) {
            m_nIdentifierListState = IDLIST_DEFINE;
            return 0;
        }
        break;

    case KEYWORD_ENGINE_NUM_STRUCTURES:
        if (m_nIdentifierListState == IDLIST_DEFINE) {
            m_nIdentifierListState = IDLIST_ENGINE_NUM_STRUCTURES;
            return 0;
        }
        break;

    case KEYWORD_ENGINE_STRUCTURE:
        if (m_nIdentifierListState != IDLIST_DEFINE)
            return -SCRIPT_ERROR_PARSING_IDENTIFIER_LIST;
        m_nIdentifierListState = IDLIST_ENGINE_STRUCTURE;
        nValue = m_szTokenText[17] - '0';
        m_nIdentifierListEngineStructure = nValue;
        return static_cast<unsigned int>(nValue) > 9 ? -SCRIPT_ERROR_PARSING_IDENTIFIER_LIST : 0;

    case KEYWORD_OBJECT_SELF:
        if (m_nIdentifierListState != IDLIST_PARAMETER_DEFAULT)
            break;
        m_nIdentifierListState = IDLIST_PARAMETERS;
        nValue = m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterCount - 1;
        m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterFlags[nValue] = 1;
        m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterObjectData[nValue] = 0;
        return 0;

    case KEYWORD_OBJECT_INVALID:
        if (m_nIdentifierListState != IDLIST_PARAMETER_DEFAULT)
            break;
        m_nIdentifierListState = IDLIST_PARAMETERS;
        nValue = m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterCount - 1;
        m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterFlags[nValue] = 1;
        m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterObjectData[nValue] = 1;
        return 0;

    case TOKEN_LEFT_SQUARE:
        if (m_nIdentifierListState != IDLIST_PARAMETER_DEFAULT && m_nIdentifierListState != IDLIST_CONSTANT_VALUE)
            break;
        m_nIdentifierListState = m_nIdentifierListState == IDLIST_CONSTANT_VALUE ? IDLIST_CONSTANT_VECTOR : IDLIST_PARAMETER_VECTOR;
        m_nIdentifierListVectorComponent = 0;
        nValue = m_pIdentifierList[m_nIdentifierCount - 1].m_nParameterCount - 1;
        m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterFlags[nValue] = 1;
        reinterpret_cast<int*>(m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterVectorData)[nValue * 3] = 0;
        reinterpret_cast<int*>(m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterVectorData)[nValue * 3 + 1] = 0;
        reinterpret_cast<int*>(m_pIdentifierList[m_nIdentifierCount - 1].m_pOptionalParameterVectorData)[nValue * 3 + 2] = 0;
        return 0;

    case TOKEN_RIGHT_SQUARE:
        if (static_cast<unsigned int>(m_nIdentifierListState - IDLIST_PARAMETER_VECTOR) > 1)
            break;
        m_nIdentifierListState = m_nIdentifierListState == IDLIST_PARAMETER_VECTOR ? IDLIST_PARAMETERS : IDLIST_CONSTANT_DONE;
        m_nIdentifierListVectorComponent = 0;
        return 0;

    case TOKEN_RIGHT_BRACKET:
    case TOKEN_COMMA:
    case TOKEN_END_OF_SOURCE:
        return 0;

    default:
        break;
    }
    return -SCRIPT_ERROR_PARSING_IDENTIFIER_LIST;
}

// Loads the identifier-specification script into include level 0 and pumps it through the
// tokenizer.
int CScriptCompilerInternal::ParseIdentifierFile()
{
    m_nNextActionId = 0;

    char* pSourceText;
    unsigned int nSize;
    int nResult = m_includeFiles[0].m_script.LoadScript(m_sSourceFile.CStr(), &pSourceText, &nSize);
    if (nResult < 0)
        return PrintParseIdentifierFileError(m_nLine, nResult);

    if (nSize < 1)
    {
    }
    else
    {
        int nChar = pSourceText[0];
        int nNext;
        if (nSize == 1)
            nNext = -1;
        else
            nNext = pSourceText[1];

        unsigned int nIndex = 2;
        while (nChar != -1)
        {
            nResult = ParseNextCharacter(nChar, nNext);
            if (nResult < 0)
            {
                m_includeFiles[0].m_script.UnloadScript();
                return PrintParseIdentifierFileError(m_nLine, nResult);
            }

            for (int nLoop = nResult + 1; nLoop > 0; nLoop--)
            {
                if (nChar == '\n')
                {
                    m_nLine++;
                    m_nColumn = 1;
                }
                else
                {
                    m_nColumn++;
                }

                nChar = nNext;
                if (nIndex < nSize)
                    nNext = pSourceText[nIndex];
                else
                    nNext = -1;
                nIndex++;
            }
        }
    }

    nResult = ParseNextCharacter(-1, -1);
    m_includeFiles[0].m_script.UnloadScript();
    if (nResult < 0)
        return PrintParseIdentifierFileError(m_nLine, nResult);
    return 0;
}

// Parses the token buffer as a float; hand-rolled because the token is not NUL-terminated.
__attribute__((noinline))
float CScriptCompilerInternal::ParseFloatFromTokenString()
{
    float fValue = 0.0f;
    float fSign = 1.0f;

    if (m_nTokenLength >= 1) {
        float fScale = 1.0f;
        int bAfterDot = 0;
        char* pText = m_szTokenText;

        for (int64_t i = 0; i < m_nTokenLength; i++) {
            char c = pText[i];
            if (c == '-' && i == 0)
                fSign = -1.0f;

            int nDigit = c - '0';
            if (static_cast<unsigned int>(nDigit) <= 9) {
                if (bAfterDot == 0) {
                    fValue = fValue * 10.0f + static_cast<float>(nDigit);
                } else if (bAfterDot == 1) {
                    fScale /= 10.0f;
                    fValue += fScale * static_cast<float>(nDigit);
                }
            }

            if (c == '.')
                bAfterDot = 1;
        }
    }

    return fSign < 0.0f ? fValue * fSign : fValue;
}

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
    long long* paNew98 = new long long[nNewCapacity];
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
