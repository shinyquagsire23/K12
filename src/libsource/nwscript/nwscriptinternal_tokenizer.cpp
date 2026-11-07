// CScriptCompilerInternal's lexer: the per-character dispatcher, the ParseCharacter* handlers it
// routes each byte to, and the token finishers they share. Compiled as its own translation unit.

#include "k2stdint.h"
#include "nwscriptinternal.h"

#include "port/libcstring.h"
#include <stdio.h>
#include <stdlib.h>
#include "exostringinline.h"

// Maps a constant's identifier class (TOKEN_INTEGER_IDENTIFIER ... TOKEN_STRING_IDENTIFIER) to the
// literal token it stands for.
static const int s_IdentifierKeywordTypes[3] = { TOKEN_NUMBER, TOKEN_FLOAT, TOKEN_STRING };

// Parses the token buffer as a float; hand-rolled because the token is not NUL-terminated.
float CScriptCompilerInternal::ParseFloatFromTokenString()
{
    float fValue = 0.0f;
    float fSign = 1.0f;

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

    return fSign < 0.0f ? fValue * fSign : fValue;
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

int CScriptCompilerInternal::HandleIdentifierToken()
{
    if (m_nTokenType == TOKEN_IDENTIFIER) {
        int nRet = TestIdentifierToken();
        if (nRet)
            return nRet;
    }
    return HandleToken();
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
        if (!((nChar >= 'a' && nChar <= 'f') || (nChar >= 'A' && nChar <= 'F')))
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        m_szTokenText[m_nTokenLength] = static_cast<char>(nChar < 'G' ? nChar + ('a' - 'A') : nChar);
    } else {
        if ((m_nTokenType & ~1) != TOKEN_IDENTIFIER)
            return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
        m_szTokenText[m_nTokenLength] = static_cast<char>(nChar);
    }
    int nLength = m_nTokenLength;
    m_nTokenLength = nLength + 1;
    if (nLength > MAX_TOKEN_LENGTH - 2)
        return -SCRIPT_ERROR_TOKEN_TOO_LONG;
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

// Runs while a string token is live; nChar is the character being consumed.
int CScriptCompilerInternal::ParseStringCharacter(int nChar, int nEscaped)
{
    if (m_nTokenType != TOKEN_STRING)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    if (nChar == '"')
        return ParseCharacterQuotationMark();
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

int CScriptCompilerInternal::ParseCharacterSemicolon()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_SEMICOLON;
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

int CScriptCompilerInternal::ParseCharacterTilde()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_TILDE;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterComma()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_COMMA;
    return HandleToken();
}

int CScriptCompilerInternal::ParseCharacterEllipsis()
{
    if (m_nTokenType != TOKEN_NONE)
        return -SCRIPT_ERROR_UNEXPECTED_CHARACTER;
    m_nTokenType = TOKEN_IDENTIFIER;
    m_szTokenText[0] = '#';
    m_nTokenLength = 1;
    return 0;
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
        if (m_nTokenLength != m_pEngineStructureNames[nIndex].GetLength())
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
        int nNewLength = m_pIdentifierList[nIndex].m_sStringData.GetLength();
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

// The lexer's per-character dispatcher, called once per byte ParseSource pumps in. It advances the
// token state machine in place and returns 0 on success, -1 when the character cannot start here,
// a negative value when the token buffer would overflow, or a forwarded parser error.
// EFFECTIVE: same calls and store targets; the orig tail-duplicates the inlined helpers' length increment per arm.
int CScriptCompilerInternal::ParseNextCharacter(int nChar, int nLookahead)
{
    if (nChar == -1)
    {
        m_nTokenType = TOKEN_END_OF_SOURCE;

        int nResult = HandleToken();
        if (nResult < 0)
            return nResult;

        return 0;
    }

    int nTokenType = m_nTokenType;

    if (nTokenType == TOKEN_LINE_COMMENT || nTokenType == TOKEN_BLOCK_COMMENT)
    {
        ParseCommentedOutCharacter(nChar);
        return 0;
    }

    if (nTokenType == TOKEN_STRING)
        return ParseStringCharacter(nChar, nLookahead);

    if (nChar >= '0' && nChar <= '9')
        return ParseCharacterNumeric(nChar);

    if (nChar == '.')
        return ParseCharacterPeriod();

    if ((nChar | ('a' - 'A')) == 'x' && m_nTokenLength == 1 && m_szTokenText[0] == '0')
        return ParseCharacterAlphabet(nChar);

    if ((nTokenType & ~1) == TOKEN_NUMBER)
    {
        int nResult = HandleToken();
        if (nResult < 0)
            return nResult;

        if (nChar == 'f' && nTokenType == TOKEN_FLOAT)
            return 0;
    }

    if ((nChar >= 'a' && nChar <= 'z') || nChar == '_' || (nChar >= 'A' && nChar <= 'Z'))
        return ParseCharacterAlphabet(nChar);

    if (m_nTokenType == TOKEN_HEX_NUMBER)
    {
        int nResult = HandleToken();
        if (nResult < 0)
            return nResult;
    }
    else if (m_nTokenType == TOKEN_IDENTIFIER)
    {
        int nResult = HandleIdentifierToken();
        if (nResult < 0)
            return nResult;
    }

    switch (nChar)
    {
    case '!':
        return ParseCharacterExclamationPoint(nLookahead);
    case '"':
        return ParseCharacterQuotationMark();
    case '#':
        return ParseCharacterEllipsis();
    case '%':
        return ParseCharacterPercentSign(nLookahead);
    case '&':
        return ParseCharacterAmpersand(nLookahead);
    case '(':
        return ParseCharacterLeftBracket();
    case ')':
        return ParseCharacterRightBracket();
    case '*':
        return ParseCharacterAsterisk(nLookahead);
    case '+':
        return ParseCharacterPlusSign(nLookahead);
    case ',':
        return ParseCharacterComma();
    case '-':
        return ParseCharacterHyphen(nLookahead);
    case '/':
        return ParseCharacterSlash(nLookahead);
    case ':':
        return ParseCharacterColon();
    case ';':
        return ParseCharacterSemicolon();
    case '<':
        return ParseCharacterLeftAngle(nLookahead);
    case '=':
        return ParseCharacterEqualSign(nLookahead);
    case '>':
        return ParseCharacterRightAngle(nLookahead);
    case '?':
        return ParseCharacterQuestionMark();
    case '[':
        return ParseCharacterLeftSquareBracket();
    case ']':
        return ParseCharacterRightSquareBracket();
    case '^':
        return ParseCharacterCarat(nLookahead);
    case '{':
        return ParseCharacterLeftBrace();
    case '|':
        return ParseCharacterVerticalBar(nLookahead);
    case '}':
        return ParseCharacterRightBrace();
    case '~':
        return ParseCharacterTilde();
    }

    return 0;
}
