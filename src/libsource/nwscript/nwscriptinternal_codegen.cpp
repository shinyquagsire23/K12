// CScriptCompilerInternal's code generator: the parse-tree walk that emits NCS, label resolution,
// the final code buffer, and the symbol tables the walk keeps.

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

enum { SYMBOL_LIST_GROWTH = 32 };

// The code buffer starts at CODE_BUFFER_SIZE bytes and grows by as much whenever a statement leaves
// less than CODE_BUFFER_SLACK bytes free.
enum { CODE_BUFFER_SIZE = 256 * 1024 };
enum { CODE_BUFFER_SLACK = 200 };

// The symbolic listing (m_bCompileSymbolicOutput): each instruction line opens with its byte
// offset, "  %08x   ", and the header line is "NCS V1.0T %08x\n".
enum { SYMBOLIC_ADDRESS_WIDTH = 13 };
enum { SYMBOLIC_HEADER_SIZE = 20 };

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

    CScriptParseTreeNode* pTree = InsertGlobalVariablesInParseTree(pReturnNode);

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
            sLabel = m_pSymbolQueryList[nQuery].m_sLabel.Right(m_pSymbolQueryList[nQuery].m_sLabel.GetLength() - 3);

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
        sLabel = m_pSymbolQueryList[nQuery].m_sLabel.Right(m_pSymbolQueryList[nQuery].m_sLabel.GetLength() - 3);
        return OutputIdentifierError(sLabel, -SCRIPT_ERROR_UNDEFINED_IDENTIFIER);
    }

    return 0;
}

// Rebuilds m_pCompiledCode as the final, relocated image.
#if !KOTOR1 || !defined(__arm__)  // K1 armv7 inlines it into GenerateFinalCodeFromParseTree
__attribute__((noinline))
#endif
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

// Identifier-table lookup by name; compares the cached name length before the text.
int CScriptCompilerInternal::GetIdentifierByName(const CExoString& sName)
{
#if KOTOR1
    int nNameLength = sName.GetLength();
#else
    int nNameLength = sName.GetLength();
#endif

    for (int64_t i = 0; i <= m_nIdentifierCount; i++) {
        if (m_pIdentifierList[i].m_nNameLength != nNameLength)
            continue;
        if (m_pIdentifierList[i].m_sName == sName)
            return static_cast<int>(i);
    }
    return -SCRIPT_ERROR_UNDEFINED_IDENTIFIER;
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

int CScriptCompilerInternal::AddSymbolToQueryList(const CExoString& sLabel, int nLocation)
{
    if (m_nSymbolQueryListCapacity == m_nSymbolQueryListCount)
    {
        m_nSymbolQueryListCapacity = m_nSymbolQueryListCapacity + SYMBOL_LIST_GROWTH;
        K2_ASSUME(m_nSymbolQueryListCount < m_nSymbolQueryListCapacity);   // the old capacity was the count
        CScriptCompilerSymbolTableEntry* pNewEntries =
            new CScriptCompilerSymbolTableEntry[m_nSymbolQueryListCapacity];

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

// Appends the width-8 hex form of the current instruction's byte offset.
void CScriptCompilerInternal::PrintBinaryAddress()
{
    sprintf(&m_pCompiledCode[m_nCompiledCodeLocation], "  %08x   ", m_nBinaryCodeOffset);
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
        int nSymbolLength = sSymbol.GetLength();
#endif
        sTarget = sSymbol.Right(nSymbolLength - 3);
        nResult = ValidateLocationOfIdentifier(sTarget);
        if (nResult < 0)
            break;
        nResult = 0;
    }

    return nResult;
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

void CScriptCompilerInternal::InitializeSwitchLabelList()
{
    m_bSwitchHasDefault = 0;
    m_nSwitchLabelCount = 0;
    m_nSwitchLabelCapacity = SWITCH_LABEL_COUNT;
    m_pSwitchLabels = new int[SWITCH_LABEL_COUNT];
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
        int nIdentifier = GetIdentifierByName(m_sFunctionImpName);
        if (nIdentifier < 0)
            return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);

        CScriptCompilerIdListEntry* pEntry = &m_pIdentifierList[nIdentifier];
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
            uint64_t nNameLen = m_sFunctionImpName.GetLength();
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
                nType = KEYWORD_VOID;
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
            if (nType != KEYWORD_VOID)
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
                uint64_t nStrLen = sValue.GetLength();
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
                    for (int k = 0; k < static_cast<int>(nStrLen); k++)
                        m_pCompiledCode[m_nCompiledCodeLocation + 4 + k] = sValue.CStr()[k];
                    m_nCompiledCodeLocation += static_cast<int>(nStrLen) + 4;
                }
                m_nBinaryCodeOffset += static_cast<int>(nStrLen) + 4;
                m_aStackTypes[m_nStackCurrentDepth] = NCS_TYPE_STRING;
                m_nStackCurrentDepth++;
            }

            if (m_pIdentifierList[nIndex].m_pParameterTypes[j] == KEYWORD_OBJECT)
            {
                unsigned long long nObject = m_pIdentifierList[nIndex].m_pOptionalParameterObjectData[j];
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

// Files the same entry type as AddSymbolToQueryList, its label-list twin, in the list ResolveLabels
// reads.
int CScriptCompilerInternal::AddSymbolToLabelList(const CExoString& sLabel, int nLocation)
{
    if (m_nSymbolLabelListCapacity == m_nSymbolLabelListCount)
    {
        m_nSymbolLabelListCapacity = m_nSymbolLabelListCapacity + SYMBOL_LIST_GROWTH;
        K2_ASSUME(m_nSymbolLabelListCount < m_nSymbolLabelListCapacity);   // the old capacity was the count
        CScriptCompilerSymbolTableEntry* pNewEntries =
            new CScriptCompilerSymbolTableEntry[m_nSymbolLabelListCapacity];

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
        return GenerateCodeForSwitchLabels(pNode);

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
            nStructSize = GetStructureSize(pNode->m_sResultStructureName);
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

// Linear search of the built-in type table by name.
int CScriptCompilerInternal::GetStructureSize(const CExoString& sName)
{
    for (int64_t i = 0; i < m_nStructureCount; i++) {
        if (sName == m_pStructureList[i].m_sName)
            return m_pStructureList[i].m_nSize;
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
            nSize = GetStructureSize(pNode->m_sResultStructureName);
        }
        else
        {
            nSize = 4;
        }

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
                PrintBinaryAddress();
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
                uint64_t nNameLen = m_sFunctionImpName.GetLength();
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
                PrintBinaryAddress();
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
            int nStrLen = pNode->m_sName.GetLength();
#endif
            if (m_bCompileSymbolicOutput != 0)
            {
                PrintBinaryAddress();
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
                PrintBinaryAddress();
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
                PrintBinaryAddress();
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
                        nSize = GetStructureSize(m_sFunctionImpReturnStructureName);
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
                uint64_t nNameLen = m_sFunctionImpName.GetLength();
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
                PrintBinaryAddress();
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
                PrintBinaryAddress();
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
                PrintBinaryAddress();
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
                PrintBinaryAddress();
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
                    int nSize = GetStructureSize(m_pVarStackList[m_nVarStackTop].m_sStructureName);
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
                        nSize = GetStructureSize(pNode->m_sResultStructureName);
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
                        PrintBinaryAddress();
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
                        int nSize = GetStructureSize(m_sVarStackVariableTypeName);
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
                // The argument types an action call accepts.
                switch (pLeft->m_nResultType)
                {
                case KEYWORD_INT:
                case KEYWORD_FLOAT:
                case KEYWORD_STRING:
                case KEYWORD_OBJECT:
                case KEYWORD_ACTION:
                case KEYWORD_STRUCT:
                case KEYWORD_ENGINE_STRUCTURE0:
                case KEYWORD_ENGINE_STRUCTURE0 + 1:
                case KEYWORD_ENGINE_STRUCTURE0 + 2:
                case KEYWORD_ENGINE_STRUCTURE0 + 3:
                case KEYWORD_ENGINE_STRUCTURE0 + 4:
                case KEYWORD_ENGINE_STRUCTURE0 + 5:
                case KEYWORD_ENGINE_STRUCTURE0 + 6:
                case KEYWORD_ENGINE_STRUCTURE0 + 7:
                case KEYWORD_ENGINE_STRUCTURE0 + 8:
                case KEYWORD_ENGINE_STRUCTURE9:
                    break;
                default:
                    return OutputWalkTreeError(-SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER, pNode);
                }

                m_aArgumentTypes[nDepth] = static_cast<unsigned char>(pLeft->m_nResultType);
                if (pLeft->m_nResultType != KEYWORD_ACTION)
                {
                    if (pLeft->m_nResultType == KEYWORD_STRUCT)
                    {
                        m_aArgumentStructureNames[nDepth] = pLeft->m_sResultStructureName;
                        int nSize = GetStructureSize(pLeft->m_sResultStructureName);
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
                    int nSize = GetStructureSize(m_pIdentifierList[nIndex].m_pParameterStructureNames[j]);
                    m_nStackCurrentDepth -= nSize >> 2;
                }
                else
                {
                    m_nStackCurrentDepth--;
                }
            }

            if (m_pIdentifierList[nIndex].m_nReturnType != TOKEN_VOID_IDENTIFIER)
            {
                pNode->m_sResultStructureName = "";
                int nType = m_pIdentifierList[nIndex].m_nReturnType;
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
                    PrintBinaryAddress();
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
                PrintBinaryAddress();
                sprintf(&m_pCompiledCode[m_nCompiledCodeLocation + SYMBOLIC_ADDRESS_WIDTH], "JSR FE_%s\n",
                        m_pIdentifierList[nIndex].m_sName.CStr());
                uint64_t nNameLen = m_pIdentifierList[nIndex].m_sName.GetLength();
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
                    PrintBinaryAddress();
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
                    PrintBinaryAddress();
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
                    PrintBinaryAddress();
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
