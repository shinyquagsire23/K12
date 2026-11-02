// CScriptCompilerInternal's identifier file: reading nwscript.nss into the engine's action and
// constant table.

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
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_INTEGER_IDENTIFIER;
                break;
            case KEYWORD_FLOAT:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_FLOAT_IDENTIFIER;
                break;
            case KEYWORD_STRING:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_STRING_IDENTIFIER;
                break;
            case KEYWORD_OBJECT:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_OBJECT_IDENTIFIER;
                break;
            case KEYWORD_VOID:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_VOID_IDENTIFIER;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 0:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 1:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 1;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 2:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 2;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 3:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 3;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 4:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 4;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 5:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 5;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 6:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 6;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 7:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 7;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 8:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 8;
                break;
            case KEYWORD_ENGINE_STRUCTURE0 + 9:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + 9;
                break;
            case KEYWORD_VECTOR:
                m_pIdentifierList[m_nIdentifierCount].m_nReturnType = TOKEN_STRUCTURE_IDENTIFIER;
                m_pIdentifierList[m_nIdentifierCount].m_sReturnStructureName = "vector";
                break;
            default:
                return -SCRIPT_ERROR_PARSING_IDENTIFIER_LIST;
            }
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
