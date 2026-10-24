// CScriptCompilerInternal's character dispatcher, compiled as its own translation unit.

#include "k2stdint.h"
#include "nwscriptinternal.h"

// The lexer's per-character dispatcher, called once per byte ParseSource pumps in. It advances the
// token state machine in place and returns 0 on success, -1 when the character cannot start here,
// a negative value when the token buffer would overflow, or a forwarded parser error.
//
// Inside a `//` or `/* */` comment the '*' is remembered in m_nTokenLength so a following '/' can
// confirm the block terminator one character later. A digit extends an existing number, float,
// identifier or hex token or opens a new number; a '0' followed by 'x' promotes to a hex token;
// everything else splits into the alpha run (identifiers, plus hex digits once the token is hex) and
// the punctuation switch.
int CScriptCompilerInternal::ParseNextCharacter(int nChar, int nLookahead)
{
    int nResult;

    if (nChar == -1)
    {
        m_nTokenType = TOKEN_END_OF_SOURCE;

        if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
            nResult = GenerateIdentifierList();
        else
            nResult = GenerateParseTree();

        if (nResult < 0)
            return nResult;

        TokenInitialize();
        return 0;
    }

    int nTokenType = m_nTokenType;

    if (static_cast<unsigned int>(nTokenType - TOKEN_LINE_COMMENT) < 2)
    {
        if (nChar == '\n' && nTokenType == TOKEN_LINE_COMMENT)
        {
            TokenInitialize();
            nTokenType = m_nTokenType;
        }

        if (nTokenType == TOKEN_BLOCK_COMMENT)
        {
            if (nChar == '*')
            {
                if (m_nTokenLength == 0)
                {
                    m_nTokenLength = 1;
                    return 0;
                }
            }
            else
            {
                if (nChar != '/')
                {
                    m_nTokenLength = 0;
                    return 0;
                }

                if (m_nTokenLength == 1)
                    TokenInitialize();
            }
        }

        return 0;
    }

    if (nTokenType == TOKEN_STRING)
        return ParseStringCharacter(nChar, nLookahead);

    char c = static_cast<char>(nChar);

    if (nChar >= '0' && nChar <= '9')
    {
        if (static_cast<unsigned int>(nTokenType - TOKEN_NUMBER) < 4 || nTokenType == TOKEN_HEX_NUMBER)
            goto AppendNumericText;

        if (nTokenType != TOKEN_NONE)
            return -1;

        m_nTokenType = TOKEN_NUMBER;
        m_nTokenLength = 0;

    AppendNumericText:
        m_szTokenText[m_nTokenLength] = c;
        nResult = m_nTokenLength;
        m_nTokenLength = nResult + 1;
        return nResult < MAX_TOKEN_LENGTH ? 0 : -SCRIPT_ERROR_TOKEN_TOO_LONG;
    }

    if (nChar == '.')
        return ParseCharacterPeriod();

    if ((nChar | ('a' - 'A')) == 'x' && m_nTokenLength == 1 && m_szTokenText[0] == '0')
    {
        if (nTokenType == TOKEN_HEX_NUMBER)
        {
            if (static_cast<unsigned int>(nChar - 'A') > 'f' - 'A')
                return -1;

            if ((static_cast<int64_t>(1) << (nChar - 'A') & HEX_LETTER_MASK) == 0)
                return -1;

            m_szTokenText[1] = static_cast<char>(nChar < 'G' ? nChar + ('a' - 'A') : nChar);
            m_nTokenLength = 2;
            return 0;
        }

        if (nTokenType == TOKEN_NUMBER)
        {
            m_szTokenText[1] = c;
            m_nTokenType = TOKEN_HEX_NUMBER;
            m_nTokenLength = 2;
            return 0;
        }

        if (nTokenType != TOKEN_NONE)
        {
            if ((nTokenType & ~1) != TOKEN_IDENTIFIER)
                return -1;

            m_szTokenText[1] = c;
            m_nTokenLength = 2;
            return 0;
        }

        m_nTokenType = TOKEN_IDENTIFIER;
        m_nTokenLength = 0;
        m_szTokenText[0] = c;
        m_nTokenLength = 1;
        return 0;
    }

    if ((nTokenType & ~1) == TOKEN_NUMBER)
    {
        if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
            nResult = GenerateIdentifierList();
        else
            nResult = GenerateParseTree();

        if (nResult < 0)
            return nResult;

        TokenInitialize();

        if (nChar == 'f' && nTokenType == TOKEN_FLOAT)
            return 0;
    }

    if ((nChar >= 'a' && nChar <= 'z') || nChar == '_' || (nChar >= 'A' && nChar <= 'Z'))
    {
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_IDENTIFIER;
            m_nTokenLength = 0;
            m_szTokenText[0] = c;
            goto AppendAlphaText;
        }

        if ((nChar | ('a' - 'A')) == 'x' && m_nTokenType == TOKEN_NUMBER)
        {
            if (m_nTokenLength != 1 || m_szTokenText[0] != '0')
                return -1;

            m_szTokenText[1] = c;
            m_nTokenType = TOKEN_HEX_NUMBER;
            m_nTokenLength = 2;
            return 0;
        }

        if (m_nTokenType == TOKEN_HEX_NUMBER)
        {
            if (static_cast<unsigned int>(nChar - 'A') > 'f' - 'A')
                return -1;

            if ((static_cast<int64_t>(1) << (nChar - 'A') & HEX_LETTER_MASK) == 0)
                return -1;

            m_szTokenText[m_nTokenLength] = static_cast<char>(nChar < 'G' ? nChar + ('a' - 'A') : nChar);
            goto AppendAlphaText;
        }

        if ((nTokenType & ~1) != TOKEN_IDENTIFIER)
            return -1;

        m_szTokenText[m_nTokenLength] = c;

    AppendAlphaText:
        nResult = m_nTokenLength;
        m_nTokenLength = nResult + 1;

        if (nResult <= MAX_TOKEN_LENGTH - 2)
            return 0;

        return -SCRIPT_ERROR_TOKEN_TOO_LONG;
    }

    if (m_nTokenType == TOKEN_HEX_NUMBER)
    {
    GenerateAndInitialize:
        if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
            nResult = GenerateIdentifierList();
        else
            nResult = GenerateParseTree();

        if (nResult < 0)
            return nResult;

        TokenInitialize();
    }
    else if (m_nTokenType == TOKEN_IDENTIFIER)
    {
        nResult = TestIdentifierToken();

        if (nResult == 0)
            goto GenerateAndInitialize;

        if (nResult < 0)
            return nResult;
    }

    switch (nChar)
    {
    case '!':
        return ParseCharacterExclamationPoint(nLookahead);


    case '#':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_szTokenText[0] = '#';
            m_nTokenType = TOKEN_IDENTIFIER;
            m_nTokenLength = 1;
            return 0;
        }
        break;


    case '&':
        return ParseCharacterAmpersand(nLookahead);


    case '(':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_LEFT_BRACKET;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '-':
        return ParseCharacterHyphen(nLookahead);


    case '/':
        return ParseCharacterSlash(nLookahead);


    case ':':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_COLON;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case ';':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_SEMICOLON;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '[':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_LEFT_SQUARE;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case ']':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_RIGHT_SQUARE;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '{':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_LEFT_BRACE;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '|':
        return ParseCharacterVerticalBar(nLookahead);


    case '}':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_RIGHT_BRACE;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '"':
        if (m_nTokenType == TOKEN_STRING)
        {
            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }

        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_STRING;
            m_nTokenLength = 0;
            return 0;
        }
        break;


    case '%':
        return ParseCharacterPercentSign(nLookahead);


    case ')':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_RIGHT_BRACKET;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '*':
        return ParseCharacterAsterisk(nLookahead);


    case '+':
        return ParseCharacterPlusSign(nLookahead);


    case ',':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_COMMA;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '<':
        return ParseCharacterLeftAngle(nLookahead);


    case '=':
        return ParseCharacterEqualSign(nLookahead);


    case '>':
        return ParseCharacterRightAngle(nLookahead);


    case '?':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_QUESTION_MARK;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    case '^':
        return ParseCharacterCarat(nLookahead);


    case '~':
        if (m_nTokenType == TOKEN_NONE)
        {
            m_nTokenType = TOKEN_TILDE;

            if (m_nCompileMode == COMPILE_MODE_IDENTIFIER_LIST)
                nResult = GenerateIdentifierList();
            else
                nResult = GenerateParseTree();

            if (nResult < 0)
                return nResult;

            TokenInitialize();
            return 0;
        }
        break;


    default:
        return 0;
    }

    return -1;
}
