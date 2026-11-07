// CScriptCompilerInternal's parser: the shift-reduce parse of the token stream into a parse tree,
// the subroutine stack it runs on, and the parse-time identifier and global-variable bookkeeping.

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

void CScriptCompilerInternal::TokenInitialize()
{
    m_nTokenType = TOKEN_NONE;
    m_nTokenLength = 0;
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

int CScriptCompilerInternal::PopSRStack(int* pnType, int* pnSubType, int* pnLocation,
                                        CScriptParseTreeNode** ppNode, CScriptParseTreeNode** ppReturnNode)
{
    if (m_nSubroutineStackSize < 0)
        return 0;
    *pnType = m_pSubroutineStack[m_nSubroutineStackSize].m_nType;
    *pnSubType = m_pSubroutineStack[m_nSubroutineStackSize].m_nSubType;
    *pnLocation = m_pSubroutineStack[m_nSubroutineStackSize].m_nLocation;
    *ppNode = m_pSubroutineStack[m_nSubroutineStackSize].m_pNode;
    *ppReturnNode = m_pSubroutineStack[m_nSubroutineStackSize].m_pReturnNode;
    m_nSubroutineStackSize = m_nSubroutineStackSize - 1;
    return 1;
}

void CScriptCompilerInternal::ModifySRStackReturnTree(CScriptParseTreeNode* pReturnNode)
{
    if (m_nSubroutineStackSize >= 0)
        m_pSubroutineStack[m_nSubroutineStackSize].m_pReturnNode = pReturnNode;
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
        m_nTokenLength = pDecl->m_sName.GetLength();
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

int CScriptCompilerInternal::AddUserDefinedIdentifier(CScriptParseTreeNode* pNode, int nFlags)
{
    m_pIdentifierList[m_nIdentifierCount].m_sName = pNode->m_pLeft->m_pLeft->m_sName;
#if KOTOR1
    m_pIdentifierList[m_nIdentifierCount].m_nNameLength = pNode->m_pLeft->m_pLeft->m_sName.GetLength();
#else
    m_pIdentifierList[m_nIdentifierCount].m_nNameLength =
        pNode->m_pLeft->m_pLeft->m_sName.GetLength();
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

            for (int nLoop = nResult; nLoop >= 0; nLoop--)
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
