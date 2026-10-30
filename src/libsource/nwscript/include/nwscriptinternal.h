#pragma once
#include "k2layout.h"

#ifndef NWSCRIPTCONSTANTS_H
#define NWSCRIPTCONSTANTS_H

// The compiler's token numbers. Punctuation, keywords and the identifier classes the identifier
// table hands out share one numbering space. A variable's or field's type is recorded as its type
// keyword (KEYWORD_INT ... KEYWORD_ENGINE_STRUCTURE0 + n); a function's or constant's return type
// as its identifier class (TOKEN_INTEGER_IDENTIFIER ... TOKEN_ENGINE_STRUCTURE0_IDENTIFIER + n).
//
// The ParseCharacter* handlers are called with the character after the one that dispatched them,
// so ParseCharacterAmpersand produces '&&' first and the bare '&' second. The two angled-bracket
// shift tokens double as the tokenizer's "one bracket seen, its twin may follow" states.
enum ScriptTokenType {
    TOKEN_NONE                         = 0,
    TOKEN_DIVIDE                       = 1,    // '/'
    TOKEN_LINE_COMMENT                 = 2,    // '//', ended by a '\n'
    TOKEN_BLOCK_COMMENT                = 3,    // '/*', ended by '*/'
    TOKEN_NUMBER                       = 4,
    TOKEN_FLOAT                        = 5,
    TOKEN_IDENTIFIER                   = 6,    // an identifier still being read
    TOKEN_STRING                       = 7,
    TOKEN_LOGICAL_AND                  = 8,    // '&&'
    TOKEN_LOGICAL_OR                   = 9,    // '||'
    TOKEN_MINUS                        = 10,
    TOKEN_LEFT_BRACE                   = 11,
    TOKEN_RIGHT_BRACE                  = 12,
    TOKEN_LEFT_BRACKET                 = 13,   // '('
    TOKEN_RIGHT_BRACKET                = 14,   // ')'
    TOKEN_SEMICOLON                    = 15,
    TOKEN_COMMA                        = 16,
    KEYWORD_IF                         = 17,
    KEYWORD_ELSE                       = 18,
    TOKEN_END_OF_SOURCE                = 19,
    TOKEN_GREATER_EQUAL                = 20,
    TOKEN_LESS_EQUAL                   = 21,
    TOKEN_GREATER_THAN                 = 22,
    TOKEN_LESS_THAN                    = 23,
    TOKEN_NOT_EQUAL                    = 24,
    TOKEN_EQUAL                        = 25,   // '=='
    TOKEN_PLUS                         = 26,
    TOKEN_PERCENT                      = 27,
    TOKEN_ASSIGN                       = 28,   // '='
    KEYWORD_INT                        = 29,
    KEYWORD_FLOAT                      = 30,
    KEYWORD_STRING                     = 31,   // the `string` type keyword, not TOKEN_STRING
    KEYWORD_OBJECT                     = 32,
    // An identifier that names no keyword and no identifier-table entry: a variable, a parameter,
    // a field, or the name of a function being declared.
    TOKEN_VARIABLE                     = 33,
    TOKEN_INTEGER_IDENTIFIER           = 34,
    TOKEN_FLOAT_IDENTIFIER             = 35,
    TOKEN_STRING_IDENTIFIER            = 36,
    TOKEN_OBJECT_IDENTIFIER            = 37,
    TOKEN_VOID_IDENTIFIER              = 38,
    TOKEN_VERTICAL_BAR                 = 39,
    TOKEN_CARAT                        = 40,
    TOKEN_AMPERSAND                    = 41,
    TOKEN_SHIFT_LEFT                   = 42,   // '<<'
    TOKEN_SHIFT_RIGHT                  = 43,   // '>>'
    TOKEN_ASTERISK                     = 44,
    TOKEN_HEX_NUMBER                   = 45,
    TOKEN_TRIPLE_SHIFT_RIGHT           = 46,   // '>>>'
    KEYWORD_ACTION                     = 47,
    TOKEN_TILDE                        = 48,
    KEYWORD_RETURN                     = 49,
    KEYWORD_WHILE                      = 50,
    KEYWORD_FOR                        = 51,
    KEYWORD_DO                         = 52,
    KEYWORD_VOID                       = 53,
    KEYWORD_STRUCT                     = 54,
    TOKEN_PERIOD                       = 55,   // the structure-member '.'
    TOKEN_STRUCTURE_IDENTIFIER         = 56,
    KEYWORD_INCLUDE                    = 57,
    TOKEN_EXCLAMATION                  = 58,
    KEYWORD_VECTOR                     = 59,
    KEYWORD_DEFINE                     = 60,
    KEYWORD_ENGINE_NUM_STRUCTURES      = 61,
    KEYWORD_ENGINE_STRUCTURE           = 62,   // ENGINE_STRUCTURE_<n>, in the identifier file
    // The names the identifier file gives engine structures 0..9 (`effect`, `location`, ...);
    // structure n is KEYWORD_ENGINE_STRUCTURE0 + n.
    KEYWORD_ENGINE_STRUCTURE0          = 63,
    KEYWORD_ENGINE_STRUCTURE9          = 72,
    TOKEN_ENGINE_STRUCTURE0_IDENTIFIER = 73,   // + n for engine structure n
    TOKEN_ENGINE_STRUCTURE9_IDENTIFIER = 82,
    KEYWORD_OBJECT_SELF                = 83,
    KEYWORD_OBJECT_INVALID             = 84,
    TOKEN_LEFT_SQUARE                  = 86,
    TOKEN_RIGHT_SQUARE                 = 87,
    TOKEN_PLUS_PLUS                    = 88,
    TOKEN_MINUS_MINUS                  = 89,
    TOKEN_MINUS_ASSIGN                 = 90,
    TOKEN_PLUS_ASSIGN                  = 91,
    TOKEN_ASTERISK_ASSIGN              = 92,
    TOKEN_SLASH_ASSIGN                 = 93,
    TOKEN_PERCENT_ASSIGN               = 94,
    TOKEN_AMPERSAND_ASSIGN             = 95,
    TOKEN_CARAT_ASSIGN                 = 96,
    TOKEN_VERTICALBAR_ASSIGN           = 97,
    TOKEN_SHIFT_LEFT_ASSIGN            = 98,
    TOKEN_SHIFT_RIGHT_ASSIGN           = 99,
    TOKEN_TRIPLE_SHIFT_RIGHT_ASSIGN    = 100,
    TOKEN_QUESTION_MARK                = 101,
    TOKEN_COLON                        = 102,
    KEYWORD_CASE                       = 103,
    KEYWORD_BREAK                      = 104,
    KEYWORD_SWITCH                     = 105,
    KEYWORD_DEFAULT                    = 106,
    KEYWORD_CONTINUE                   = 107,
};

// The identifier file declares at most this many engine structures.
enum { ENGINE_STRUCTURE_COUNT = 10 };

// Bits 'A'..'F' and 'a'..'f' of a bit set indexed by (character - 'A').
enum : long long {
    HEX_LETTER_BITS = (1LL << ('F' - 'A' + 1)) - 1,
    HEX_LETTER_MASK = HEX_LETTER_BITS | (HEX_LETTER_BITS << ('a' - 'A')),
};

// The size of the token buffer, m_szTokenText.
enum { MAX_TOKEN_LENGTH = 512 };

// CScriptCompilerInternal::m_nCompileMode: what the tokenizer feeds its tokens to.
enum ScriptCompileMode {
    COMPILE_MODE_PARSE_TREE      = 0,   // GenerateParseTree, compiling a script
    COMPILE_MODE_IDENTIFIER_LIST = 1,   // GenerateIdentifierList, reading the identifier file
};

// CScriptCompilerIdListEntry::m_nIdentifierType.
enum ScriptIdentifierType {
    IDENTIFIER_TYPE_CONSTANT = 0,
    IDENTIFIER_TYPE_FUNCTION = 1,
};

// The include-file stack's depth.
enum { INCLUDE_FILE_COUNT = 16 };

// The most parameters a function may declare (ExpandParameterSpace's ceiling).
enum { MAX_PARAMETER_COUNT = 32 };

// Parse-tree node types (CScriptParseTreeNode::m_nType). A separate numbering space from the
// tokens.
enum ScriptParseTreeNodeType {
    NODE_COMPOUND_STATEMENT            = 0,    // `{ ... }`, which opens a variable scope
    NODE_STATEMENT                     = 1,    // one link of a statement chain
    NODE_DECLARATION                   = 2,    // a type and its variable list
    NODE_TYPE_INT                      = 3,
    NODE_TYPE_FLOAT                    = 4,
    NODE_TYPE_STRING                   = 5,
    NODE_TYPE_OBJECT                   = 6,
    NODE_VARIABLE_LIST                 = 7,
    NODE_VARIABLE                      = 8,
    NODE_STATEMENT_LIST                = 9,
    NODE_IF_BLOCK                      = 10,
    NODE_IF_CHOICE                     = 11,
    NODE_IF_CONDITION                  = 12,
    NODE_ACTION                        = 13,   // a call: its argument list and its ACTION_ID
    NODE_ACTION_ID                     = 14,
    NODE_ASSIGNMENT                    = 15,
    NODE_ACTION_ARG_LIST               = 16,
    NODE_CONSTANT_INTEGER              = 17,
    NODE_CONSTANT_FLOAT                = 18,
    NODE_CONSTANT_STRING               = 19,
    NODE_INTEGER_EXPRESSION            = 20,
    NODE_NON_VOID_EXPRESSION           = 21,
    NODE_LOGICAL_OR                    = 22,
    NODE_LOGICAL_AND                   = 23,
    NODE_INCLUSIVE_OR                  = 24,
    NODE_EXCLUSIVE_OR                  = 25,
    NODE_BOOLEAN_AND                   = 26,
    NODE_EQUAL                         = 27,
    NODE_NOT_EQUAL                     = 28,
    NODE_GREATER_EQUAL                 = 29,
    NODE_GREATER_THAN                  = 30,
    NODE_LESS_THAN                     = 31,
    NODE_LESS_EQUAL                    = 32,
    NODE_SHIFT_LEFT                    = 33,
    NODE_SHIFT_RIGHT                   = 34,
    NODE_ADD                           = 35,
    NODE_SUBTRACT                      = 36,
    NODE_MULTIPLY                      = 37,
    NODE_DIVIDE                        = 38,
    NODE_MODULUS                       = 39,
    NODE_NEGATE                        = 40,
    NODE_ACTION_PARAMETER              = 41,   // an `action` argument: its state is stored, not run
    NODE_UNSIGNED_SHIFT_RIGHT          = 42,
    NODE_STRUCTURE_PART                = 43,   // `a.b`
    NODE_ONES_COMPLEMENT               = 44,
    NODE_WHILE_BLOCK                   = 45,
    NODE_WHILE_CHOICE                  = 46,
    NODE_WHILE_CONDITION               = 47,
    NODE_DOWHILE_BLOCK                 = 48,
    NODE_DOWHILE_CONDITION             = 49,
    NODE_FUNCTIONAL_UNIT               = 50,   // one top-level unit and the chain after it
    NODE_TYPE_STRUCT                   = 51,
    NODE_STRUCTURE_DEFINITION          = 52,
    NODE_FUNCTION_IDENTIFIER           = 53,
    NODE_FUNCTION_DECLARATION          = 54,
    NODE_FUNCTION                      = 55,
    NODE_FUNCTION_PARAMETER_NAME       = 56,
    NODE_TYPE_VOID                     = 57,
    NODE_RETURN                        = 58,
    NODE_BOOLEAN_NOT                   = 59,
    NODE_TYPE_ENGINE_STRUCTURE0        = 60,   // + n for engine structure n
    NODE_TYPE_ENGINE_STRUCTURE9        = 69,
    NODE_CONSTANT_OBJECT               = 70,
    NODE_CONSTANT_VECTOR               = 72,
    NODE_GLOBAL_VARIABLES              = 73,
    NODE_POST_INCREMENT                = 74,
    NODE_POST_DECREMENT                = 75,
    NODE_PRE_INCREMENT                 = 76,
    NODE_PRE_DECREMENT                 = 77,
    NODE_CONDITIONAL_BLOCK             = 78,   // `a ? b : c`
    NODE_CONDITIONAL_CHOICE            = 79,
    NODE_CONDITIONAL_CONDITION         = 80,
    NODE_SWITCH_BLOCK                  = 81,
    NODE_SWITCH_CONDITION              = 82,
    NODE_DEFAULT                       = 83,
    NODE_CASE                          = 84,
    NODE_BREAK                         = 85,
    NODE_CONTINUE                      = 86,
    NODE_CONTINUE_TARGET               = 87,   // where a loop's `continue` lands
};

// The grammar rules of GenerateParseTree's shift/reduce stack (CScriptSRStackEntry::m_nType). A
// rule's alternatives (m_nSubType) and steps (m_nLocation) are numbered within the rule.
enum ScriptGrammarRule {
    GRAMMAR_PROGRAM                    = 0,
    GRAMMAR_FUNCTIONAL_UNIT            = 1,
    GRAMMAR_END_OF_PROGRAM             = 2,
    GRAMMAR_PARAMETER_LIST             = 3,
    GRAMMAR_PARAMETER_LIST_TAIL        = 4,    // a ',' or the closing ')'
    GRAMMAR_STRUCTURE_BODY             = 5,
    GRAMMAR_FIELD_LIST                 = 6,
    GRAMMAR_FIELD_LIST_TAIL            = 7,    // a ',' or the closing ';'
    GRAMMAR_COMPOUND_STATEMENT_BODY    = 8,
    GRAMMAR_STATEMENT_LIST             = 9,
    GRAMMAR_STATEMENT                  = 10,
    GRAMMAR_TYPE_SPECIFIER             = 11,
    GRAMMAR_NON_VOID_TYPE_SPECIFIER    = 12,
    GRAMMAR_VARIABLE_DECLARATOR        = 13,
    GRAMMAR_VARIABLE_DECLARATOR_TAIL   = 14,   // a ',' or the closing ';'
    GRAMMAR_ARGUMENT_LIST              = 15,
    GRAMMAR_ARGUMENT_LIST_TAIL         = 16,   // a ',' or the closing ')'
    GRAMMAR_BOOLEAN_EXPRESSION         = 17,
    GRAMMAR_NON_VOID_EXPRESSION        = 18,
    GRAMMAR_EXPRESSION                 = 19,
    GRAMMAR_ASSIGNMENT_EXPRESSION      = 20,
    GRAMMAR_CONDITIONAL_EXPRESSION     = 21,
    GRAMMAR_LOGICAL_OR_EXPRESSION      = 22,
    GRAMMAR_LOGICAL_AND_EXPRESSION     = 23,
    GRAMMAR_INCLUSIVE_OR_EXPRESSION    = 24,
    GRAMMAR_EXCLUSIVE_OR_EXPRESSION    = 25,
    GRAMMAR_BOOLEAN_AND_EXPRESSION     = 26,
    GRAMMAR_EQUALITY_EXPRESSION        = 27,
    GRAMMAR_RELATIONAL_EXPRESSION      = 28,
    GRAMMAR_SHIFT_EXPRESSION           = 29,
    GRAMMAR_ADDITIVE_EXPRESSION        = 30,
    GRAMMAR_MULTIPLICATIVE_EXPRESSION  = 31,
    GRAMMAR_UNARY_EXPRESSION           = 32,
    GRAMMAR_POSTFIX_EXPRESSION         = 33,
    GRAMMAR_PRIMARY_EXPRESSION         = 34,
    GRAMMAR_CONSTANT                   = 35,
};

// The compiled script (NCS) format: "NCS V1.0", a 'B', the 4-byte big-endian file size, then the
// code. Every instruction is an opcode byte and a type byte, followed by big-endian operands.
enum { NCS_HEADER_SIZE = 13 };

// STORE_STATE's type byte: the deferred code starts this many bytes on, past the STORE_STATE itself
// and the JMP that skips the deferred code for now.
enum { NCS_STORE_STATE_RESUME_OFFSET = 16 };

enum NcsOpcode {
    NCS_OP_CPDOWNSP                    = 1,
    NCS_OP_RSADD                       = 2,
    NCS_OP_CPTOPSP                     = 3,
    NCS_OP_CONST                       = 4,
    NCS_OP_ACTION                      = 5,
    NCS_OP_LOGANDII                    = 6,
    NCS_OP_LOGORII                     = 7,
    NCS_OP_INCORII                     = 8,
    NCS_OP_EXCORII                     = 9,
    NCS_OP_BOOLANDII                   = 10,
    NCS_OP_EQUAL                       = 11,
    NCS_OP_NEQUAL                      = 12,
    NCS_OP_GEQ                         = 13,
    NCS_OP_GT                          = 14,
    NCS_OP_LT                          = 15,
    NCS_OP_LEQ                         = 16,
    NCS_OP_SHLEFTII                    = 17,
    NCS_OP_SHRIGHTII                   = 18,
    NCS_OP_USHRIGHTII                  = 19,
    NCS_OP_ADD                         = 20,
    NCS_OP_SUB                         = 21,
    NCS_OP_MUL                         = 22,
    NCS_OP_DIV                         = 23,
    NCS_OP_MOD                         = 24,
    NCS_OP_NEG                         = 25,
    NCS_OP_COMP                        = 26,
    NCS_OP_MOVSP                       = 27,
    NCS_OP_STORE_STATEALL              = 28,
    NCS_OP_JMP                         = 29,
    NCS_OP_JSR                         = 30,
    NCS_OP_JZ                          = 31,
    NCS_OP_RETN                        = 32,
    NCS_OP_DESTRUCT                    = 33,
    NCS_OP_NOTI                        = 34,
    NCS_OP_DECISP                      = 35,
    NCS_OP_INCISP                      = 36,
    NCS_OP_JNZ                         = 37,
    NCS_OP_CPDOWNBP                    = 38,
    NCS_OP_CPTOPBP                     = 39,
    NCS_OP_DECIBP                      = 40,
    NCS_OP_INCIBP                      = 41,
    NCS_OP_SAVEBP                      = 42,
    NCS_OP_RESTOREBP                   = 43,
    NCS_OP_STORE_STATE                 = 44,
};

// An instruction's type byte. The single types also tag the VM's stack cells.
enum NcsType {
    NCS_TYPE_NONE                      = 0,
    NCS_TYPE_STACK_OPERATION           = 1,    // CPDOWNSP, CPTOPSP, DESTRUCT and their BP forms
    NCS_TYPE_INTEGER                   = 3,
    NCS_TYPE_FLOAT                     = 4,
    NCS_TYPE_STRING                    = 5,
    NCS_TYPE_OBJECT                    = 6,
    NCS_TYPE_ENGINE_STRUCTURE0         = 16,   // + n for engine structure n
    NCS_TYPE_ENGINE_STRUCTURE9         = 25,
    NCS_TYPE_INTEGER_INTEGER           = 32,
    NCS_TYPE_FLOAT_FLOAT               = 33,
    NCS_TYPE_OBJECT_OBJECT             = 34,
    NCS_TYPE_STRING_STRING             = 35,
    NCS_TYPE_STRUCT_STRUCT             = 36,
    NCS_TYPE_INTEGER_FLOAT             = 37,
    NCS_TYPE_FLOAT_INTEGER             = 38,
    NCS_TYPE_ENGINE_STRUCTURE0_PAIR    = 48,   // + n for two of engine structure n
    NCS_TYPE_ENGINE_STRUCTURE9_PAIR    = 57,
    NCS_TYPE_VECTOR_VECTOR             = 58,
    NCS_TYPE_VECTOR_FLOAT              = 59,
    NCS_TYPE_FLOAT_VECTOR              = 60,
};

// The compiler and the VM report errors as negated dialog.tlk strrefs; the text of strref n is the
// message for error -n.
enum ScriptErrorStrRef {
    SCRIPT_ERROR_UNEXPECTED_CHARACTER                                        = 1,
    SCRIPT_ERROR_FATAL_COMPILER_ERROR                                        = 2,
    SCRIPT_ERROR_PROGRAM_COMPOUND_STATEMENT_AT_START                         = 3,
    SCRIPT_ERROR_UNEXPECTED_END_COMPOUND_STATEMENT                           = 4,
    SCRIPT_ERROR_AFTER_END_COMPOUND_STATEMENT                                = 5,
    SCRIPT_ERROR_PARSING_VARIABLE_LIST                                       = 6,
    SCRIPT_ERROR_UNKNOWN_STATE_IN_COMPILER                                   = 7,
    SCRIPT_ERROR_INVALID_DECLARATION_TYPE                                    = 8,
    SCRIPT_ERROR_NO_LEFT_BRACKET_ON_EXPRESSION                               = 9,
    SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_EXPRESSION                              = 10,
    SCRIPT_ERROR_BAD_START_OF_STATEMENT                                      = 11,
    SCRIPT_ERROR_NO_LEFT_BRACKET_ON_ARG_LIST                                 = 12,
    SCRIPT_ERROR_NO_RIGHT_BRACKET_ON_ARG_LIST                                = 13,
    SCRIPT_ERROR_NO_SEMICOLON_AFTER_EXPRESSION                               = 14,
    SCRIPT_ERROR_PARSING_ASSIGNMENT_STATEMENT                                = 15,
    SCRIPT_ERROR_CANNOT_ASSIGN_TO_LEFT_SIDE                                  = 16,
    SCRIPT_ERROR_BAD_CONSTANT_TYPE                                           = 17,
    SCRIPT_ERROR_IDENTIFIER_LIST_FULL                                        = 18,
    SCRIPT_ERROR_NON_INTEGER_ID_FOR_INTEGER_CONSTANT                         = 19,
    SCRIPT_ERROR_NON_FLOAT_ID_FOR_FLOAT_CONSTANT                             = 20,
    SCRIPT_ERROR_NON_STRING_ID_FOR_STRING_CONSTANT                           = 21,
    SCRIPT_ERROR_VARIABLE_ALREADY_USED_WITHIN_SCOPE                          = 22,
    SCRIPT_ERROR_VARIABLE_DEFINED_WITHOUT_TYPE                               = 23,
    SCRIPT_ERROR_INCORRECT_VARIABLE_STATE_LEFT_ON_STACK                      = 24,
    SCRIPT_ERROR_NON_INTEGER_EXPRESSION_WHERE_INTEGER_REQUIRED               = 25,
    SCRIPT_ERROR_VOID_EXPRESSION_WHERE_NON_VOID_REQUIRED                     = 26,
    SCRIPT_ERROR_INVALID_PARAMETERS_FOR_ASSIGNMENT                           = 27,
    SCRIPT_ERROR_DECLARATION_DOES_NOT_MATCH_PARAMETERS                       = 28,
    SCRIPT_ERROR_LOGICAL_OPERATION_HAS_INVALID_OPERANDS                      = 29,
    SCRIPT_ERROR_EQUALITY_TEST_HAS_INVALID_OPERANDS                          = 30,
    SCRIPT_ERROR_COMPARISON_TEST_HAS_INVALID_OPERANDS                        = 31,
    SCRIPT_ERROR_SHIFT_OPERATION_HAS_INVALID_OPERANDS                        = 32,
    SCRIPT_ERROR_ARITHMETIC_OPERATION_HAS_INVALID_OPERANDS                   = 33,
    SCRIPT_ERROR_UNKNOWN_OPERATION_IN_SEMANTIC_CHECK                         = 34,
    SCRIPT_ERROR_SCRIPT_TOO_LARGE                                            = 35,
    SCRIPT_ERROR_RETURN_STATEMENT_HAS_NO_PARAMETERS                          = 36,
    SCRIPT_ERROR_NO_WHILE_AFTER_DO_KEYWORD                                   = 37,
    SCRIPT_ERROR_FUNCTION_DEFINITION_MISSING_NAME                            = 38,
    SCRIPT_ERROR_FUNCTION_DEFINITION_MISSING_PARAMETER_LIST                  = 39,
    SCRIPT_ERROR_MALFORMED_PARAMETER_LIST                                    = 40,
    SCRIPT_ERROR_BAD_TYPE_SPECIFIER                                          = 41,
    SCRIPT_ERROR_NO_SEMICOLON_AFTER_STRUCTURE                                = 42,
    SCRIPT_ERROR_ELLIPSIS_IN_IDENTIFIER                                      = 43,
    SCRIPT_ERROR_FILE_NOT_FOUND                                              = 44,
    SCRIPT_ERROR_INCLUDE_RECURSIVE                                           = 45,
    SCRIPT_ERROR_INCLUDE_TOO_MANY_LEVELS                                     = 46,
    SCRIPT_ERROR_PARSING_RETURN_STATEMENT                                    = 47,
    SCRIPT_ERROR_PARSING_IDENTIFIER_LIST                                     = 48,
    SCRIPT_ERROR_PARSING_FUNCTION_DECLARATION                                = 49,
    SCRIPT_ERROR_DUPLICATE_FUNCTION_IMPLEMENTATION                           = 50,
    SCRIPT_ERROR_TOKEN_TOO_LONG                                              = 51,
    SCRIPT_ERROR_UNDEFINED_STRUCTURE                                         = 52,
    SCRIPT_ERROR_LEFT_OF_STRUCTURE_PART_NOT_STRUCTURE                        = 53,
    SCRIPT_ERROR_RIGHT_OF_STRUCTURE_PART_NOT_FIELD_IN_STRUCTURE              = 54,
    SCRIPT_ERROR_UNDEFINED_FIELD_IN_STRUCTURE                                = 55,
    SCRIPT_ERROR_STRUCTURE_REDEFINED                                         = 56,
    SCRIPT_ERROR_VARIABLE_USED_TWICE_IN_SAME_STRUCTURE                       = 57,
    SCRIPT_ERROR_FUNCTION_IMPLEMENTATION_AND_DEFINITION_DIFFER               = 58,
    SCRIPT_ERROR_MISMATCHED_TYPES                                            = 59,
    SCRIPT_ERROR_INTEGER_NOT_AT_TOP_OF_STACK                                 = 60,
    SCRIPT_ERROR_RETURN_TYPE_AND_FUNCTION_TYPE_MISMATCHED                    = 61,
    SCRIPT_ERROR_NOT_ALL_CONTROL_PATHS_RETURN_A_VALUE                        = 62,
    SCRIPT_ERROR_UNDEFINED_IDENTIFIER                                        = 63,
    SCRIPT_ERROR_NO_FUNCTION_MAIN_IN_SCRIPT                                  = 64,
    SCRIPT_ERROR_FUNCTION_MAIN_MUST_HAVE_A_VOID_RETURN_VALUE                 = 65,
    SCRIPT_ERROR_FUNCTION_MAIN_MUST_HAVE_NO_PARAMETERS                       = 66,
    SCRIPT_ERROR_NON_VOID_FUNCTION_CANNOT_BE_A_STATEMENT                     = 67,
    SCRIPT_ERROR_BAD_VARIABLE_NAME                                           = 68,
    SCRIPT_ERROR_NON_OPTIONAL_PARAMETER_CANNOT_FOLLOW_OPTIONAL_PARAMETER     = 69,
    SCRIPT_ERROR_TYPE_DOES_NOT_HAVE_AN_OPTIONAL_PARAMETER                    = 70,
    SCRIPT_ERROR_NON_CONSTANT_IN_FUNCTION_DECLARATION                        = 71,
    SCRIPT_ERROR_PARSING_CONSTANT_VECTOR                                     = 72,
    SCRIPT_ERROR_OPERAND_MUST_BE_AN_INTEGER_LVALUE                           = 73,
    SCRIPT_ERROR_CONDITIONAL_REQUIRES_SECOND_EXPRESSION                      = 74,
    SCRIPT_ERROR_CONDITIONAL_MUST_HAVE_MATCHING_RETURN_TYPES                 = 75,
    SCRIPT_ERROR_MULTIPLE_DEFAULT_STATEMENTS_WITHIN_SWITCH                   = 76,
    SCRIPT_ERROR_MULTIPLE_CASE_CONSTANT_STATEMENTS_WITHIN_SWITCH             = 77,
    SCRIPT_ERROR_CASE_PARAMETER_NOT_A_CONSTANT_INTEGER                       = 78,
    SCRIPT_ERROR_SWITCH_MUST_EVALUATE_TO_AN_INTEGER                          = 79,
    SCRIPT_ERROR_NO_COLON_AFTER_DEFAULT_LABEL                                = 80,
    SCRIPT_ERROR_NO_COLON_AFTER_CASE_LABEL                                   = 81,
    SCRIPT_ERROR_NO_SEMICOLON_AFTER_STATEMENT                                = 82,
    SCRIPT_ERROR_BREAK_OUTSIDE_OF_LOOP_OR_CASE_STATEMENT                     = 83,
    SCRIPT_ERROR_TOO_MANY_PARAMETERS                                         = 84,  // dialog.tlk repeats strref 83's text here
    SCRIPT_ERROR_UNABLE_TO_OPEN_FILE_FOR_WRITING                             = 85,
    SCRIPT_ERROR_UNTERMINATED_STRING_CONSTANT                                = 86,
    SCRIPT_ERROR_NO_FUNCTION_STARTING_CONDITIONAL_IN_SCRIPT                  = 87,
    SCRIPT_ERROR_FUNCTION_STARTING_CONDITIONAL_MUST_HAVE_AN_INT_RETURN_VALUE = 88,
    SCRIPT_ERROR_FUNCTION_STARTING_CONDITIONAL_MUST_HAVE_NO_PARAMETERS       = 89,
    SCRIPT_ERROR_SKIPPING_DECLARATION_VIA_CASE_STATEMENT_DISALLOWED          = 90,
    SCRIPT_ERROR_SKIPPING_DECLARATION_VIA_DEFAULT_STATEMENT_DISALLOWED       = 91,
    SCRIPT_ERROR_ELSE_WITHOUT_IF_STATEMENT                                   = 92,
    SCRIPT_ERROR_TOO_MANY_INSTRUCTIONS                                       = 93,
    SCRIPT_ERROR_TOO_MANY_LEVELS_OF_RECURSION                                = 94,
    SCRIPT_ERROR_FILE_NOT_OPENED                                             = 95,
    SCRIPT_ERROR_FILE_NOT_COMPILED_SUCCESSFULLY                              = 96,
    SCRIPT_ERROR_INVALID_AUX_CODE                                            = 97,
    SCRIPT_ERROR_NULL_VIRTUAL_MACHINE_NODE                                   = 98,
    SCRIPT_ERROR_STACK_OVERFLOW                                              = 99,
    SCRIPT_ERROR_STACK_UNDERFLOW                                             = 100,
    SCRIPT_ERROR_INVALID_OP_CODE                                             = 101,
    SCRIPT_ERROR_INVALID_EXTRA_DATA_ON_OP_CODE                               = 102,
    SCRIPT_ERROR_INVALID_COMMAND                                             = 103,
    SCRIPT_ERROR_FAKE_SHORTCUT_LOGICAL_OPERATION                             = 104,
    SCRIPT_ERROR_DIVIDE_BY_ZERO                                              = 105,
    SCRIPT_ERROR_FAKE_ABORT_SCRIPT                                           = 106,
    SCRIPT_ERROR_IP_OUT_OF_CODE_SEGMENT                                      = 107,
    SCRIPT_ERROR_COMMAND_IMPLEMENTER_NOT_SET                                 = 108,
    SCRIPT_ERROR_UNKNOWN_TYPE_ON_RUN_TIME_STACK                              = 109,
};

#endif


#ifndef CSCRIPTPARSETREENODE_H
#define CSCRIPTPARSETREENODE_H

#include "exostring.h"

// One node of the NWScript parse tree, with a token type, an integer and float value payload, the
// source position, two child links and a name.
class CScriptParseTreeNode {
public:
    // Inlined at every call site, so the body belongs in the class.
    __attribute__((always_inline)) CScriptParseTreeNode()
    {
        m_nType = 0;
        m_sName = "";
        m_nSavedStackDepth = 0;
        __builtin_memset(&m_nValue, 0, sizeof(m_nValue) + sizeof(m_nCodeStart) +
                                          sizeof(m_nSavedEnclosingState) + sizeof(m_fValue) +
                                          sizeof(m_fVectorX) + sizeof(m_fVectorY) +
                                          sizeof(m_fVectorZ) + sizeof(m_nLine) +
                                          sizeof(m_nColumn));
        m_pLeft = 0;
        m_pRight = 0;
        m_nResultType = 0;
    }

    int        m_nType;
    CExoString m_sName;
    int        m_nValue;
    int        m_nCodeStart;
    int        m_nSavedEnclosingState;
    float      m_fValue;
    float      m_fVectorX;
    float      m_fVectorY;
    float      m_fVectorZ;
    int        m_nLine;
    int        m_nColumn;
    CScriptParseTreeNode* m_pLeft;
    CScriptParseTreeNode* m_pRight;
    int        m_nResultType;
    CExoString m_sResultStructureName;
    int        m_nSavedStackDepth;
};

K2_LAYOUT_ASSERT(sizeof(CScriptParseTreeNode) == 112, "CScriptParseTreeNode span");

#endif

#ifndef CSCRIPTCOMPILERIDLISTENTRY_H
#define CSCRIPTCOMPILERIDLISTENTRY_H

#include "exostring.h"

// One slot of CScriptCompilerInternal's user-defined identifier table, holding the name and its
// length, the token and type metadata, and the per-parameter arrays built while parsing a
// prototype.
class CScriptCompilerIdListEntry {
public:
    CScriptCompilerIdListEntry();
    ~CScriptCompilerIdListEntry();
    // Returns a negative value on error.
    int  ExpandParameterSpace();

    CExoString m_sName;
    int        m_nNameLength;
    int        m_nIdentifierType;
    int        m_nReturnType;
    int        m_bImplementationInPlace;
    CExoString m_sReturnStructureName;
    CExoString m_sStringData;
    int        m_nIntegerData;
    float      m_fFloatData;
    float      m_fVectorData[3];
    int        m_nActionId;
    int        m_nParameterCount;
    int        m_nNonOptionalParameterCount;
    int        m_nParameterSpace;
    unsigned char* m_pParameterTypes;
    CExoString* m_pParameterStructureNames;
    int*       m_pOptionalParameterFlags;
    int*       m_pOptionalParameterIntegerData;
    float*     m_pOptionalParameterFloatData;
    CExoString* m_pOptionalParameterStringData;
    unsigned long long* m_pOptionalParameterObjectData;
    float*     m_pOptionalParameterVectorData;
    int        m_nCodeOffset;
    int        m_nCodeEnd;
    int        m_nOutputStart;
    int        m_nOutputEnd;
};

K2_LAYOUT_ASSERT(sizeof(CScriptCompilerIdListEntry) == 184, "identifier-table slot stride");

#endif

#ifndef CSOURCESCRIPT_H
#define CSOURCESCRIPT_H

#include "CResHelper.h"
#include "resnss.h"

class CExoString;

// One loaded NWScript source file (.nss) as a resource handle; it adds no data members of its
// own to CResHelper.
class CSourceScript : public CResHelper<CResNSS, 2009> {
public:
    // Defined out of line in nwscriptinternal.cpp, the unit that constructs it.
    CSourceScript();

    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 2 slots; both are the destructor group, and only D0 has its own body
    virtual ~CSourceScript();
    // </VTABLE-GEN>

    // Loads the named .nss through this handle and publishes its text and size. Returns 0 on
    // success and a negative code when the resource cannot be demanded. noinline: only one of the
    // two callers uses this copy.
    __attribute__((noinline))
    int  LoadScript(const CExoString& sName, char** ppSourceText, unsigned int* pnSize)
    {
        CResRef resRef(sName);
        SetResRef(resRef, 0);

        if (m_pRes->Demand() == 0)
            return -SCRIPT_ERROR_FILE_NOT_FOUND;

        *ppSourceText = static_cast<char*>(m_pRes->GetNSSDataPtr());
        *pnSize = m_pRes->GetNSSSize();
        return 0;
    }
    // Releases the underlying resource; m_pRes itself is left alone.
    void UnloadScript();
};

#endif

#ifndef CSCRIPTCOMPILERINTERNAL_H
#define CSCRIPTCOMPILERINTERNAL_H

// One entry of the global-variable table the code generator fills.
struct CScriptCompilerTable700Entry {
    CExoString m_sName;
    int        m_nType;
    int        m_nRecursionLevel;
    int        m_nRunTimeLocation;
    CExoString m_sStructureName;
    CScriptCompilerTable700Entry()
    {
        m_nType = 0;
        m_nRecursionLevel = 0;
        m_nRunTimeLocation = 0;
    }
};
// One built-in type descriptor, with its name, the first and last field index and the size in
// bytes.
struct CScriptCompilerTable720Entry {
    CExoString m_sName;
    int        m_nFirstField;
    int        m_nLastField;
    int        m_nSize;
    CScriptCompilerTable720Entry()
    {
        m_nFirstField = 0;
        m_nLastField = 0;
        m_nSize = 0;
    }
};
// One field of a built-in type, with its name and its byte offset.
struct CScriptCompilerTable728Entry {
    unsigned char m_nType;
    CExoString m_sStructureName;
    CExoString m_sName;
    int        m_nOffset;
    CScriptCompilerTable728Entry()
    {
        m_nType = 0;
        m_nOffset = 0;
    }
};

// One entry of the two symbol lists, a label and its location in the compiled script.
struct CScriptCompilerSymbolListEntry {
    CExoString m_sLabel;
    int        m_nCodeLocation;
    CScriptCompilerSymbolListEntry()
    {
        m_sLabel = "";
        m_nCodeLocation = 0;
    }
};
K2_LAYOUT_ASSERT(sizeof(CScriptCompilerSymbolListEntry) == 24, "symbol-list stride");

// One slot of the include-file stack, holding the file name, its loaded script and its saved
// cursor.
struct CScriptCompilerIncludeFile {
    CExoString m_sFileName;
    CSourceScript m_script;
    int m_nLine;
    int m_nColumn;
    int m_nTokenType;
    int m_nTokenLength;
};

// One entry of the subroutine/expression stack.
struct CScriptSRStackEntry {
    int m_nType;
    int m_nSubType;
    int m_nLocation;
    CScriptParseTreeNode* m_pNode;
    CScriptParseTreeNode* m_pReturnNode;
};

// The NWScript compiler core owns the source text, the tokenizer cursor, the identifier and symbol
// tables and the parse tree, and drives the three compile modes (parse tree, identifier list,
// conditional).
//
// The member list below is incomplete: runs whose fields are not identified yet stay as char arrays,
// and are only split once a body is seen to read or write them.
class CScriptCompilerInternal {
public:
    CScriptCompilerInternal();
    ~CScriptCompilerInternal();

    void Initialize();
    void InitializePreDefinedStructures();
    void ShutDown() throw();
    void SetIdentifierSpecification(const CExoString& sFile);

    int  CompileFile(const CExoString& sSource);
    int  CompileScriptChunk(const CExoString& sSource);
    int  CompileScriptConditional(const CExoString& sSource);
    int  GetCompiledScriptCode(char** ppCode, int* pnSize);
    void ClearCompiledScriptCode();
    int  OutputError(int nType, const CExoString& sFile, int nLine, const CExoString& sMsg);
    int  GenerateFinalCodeFromParseTree();
    int  InstallLoader();
    void InitializeFinalCode();
    void FinalizeFinalCode();
    int  DetermineLocationOfCode();
    int  WriteFinalCodeToFile(const CExoString& sFile);
    int  ResolveLabels();
    void PrintBinaryAddress();
    int  CleanUpDuringCompile(int nMode);
    int  WriteResolvedOutput();
    int  OutputIdentifierError(const CExoString& sName, int nError);
    int  PrintParseIdentifierFileError(int nLine, int nError);
    int  PrintParseSourceError(int nLine, int nError);

    void SetCompileDebugLevel(int bEnable);
    void SetCompileSymbolicOutput(int bEnable);
    void SetOutputAlias(const CExoString& sAlias);
    void SetOptimizeBinaryCodeLength(int bEnable);
    void SetCompileConditionalFile(int bEnable);

    void TokenInitialize();
    int  HandleToken();
    int  HandleIdentifierToken();
    int  ParseNextCharacter(int nChar, int nLookahead);
    int  ParseSource(char* pSource, int nLength);
    int  TestIdentifierToken();
    float ParseFloatFromTokenString();
    int  GenerateParseTree();
    int  TraverseTreeForSwitchLabels(CScriptParseTreeNode* pNode);

    int  ParseCharacterNumeric(int nChar);
    int  ParseCharacterPeriod();
    int  ParseCharacterAlphabet(int nChar);
    int  ParseCharacterSlash(int nChar);
    int  ParseCharacterAmpersand(int nChar);
    int  ParseCharacterVerticalBar(int nChar);
    int  ParseCharacterAsterisk(int nChar);
    int  ParseCommentedOutCharacter(int nChar);
    int  ParseStringCharacter(int nChar, int nEscaped);
    int  ParseCharacterQuotationMark();
    int  ParseCharacterHyphen(int nChar);
    int  ParseCharacterLeftBrace();
    int  ParseCharacterRightBrace();
    int  ParseCharacterLeftBracket();
    int  ParseCharacterRightBracket();
    int  ParseCharacterLeftSquareBracket();
    int  ParseCharacterRightSquareBracket();
    int  ParseCharacterRightAngle(int nChar);
    int  ParseCharacterLeftAngle(int nChar);
    int  ParseCharacterExclamationPoint(int nChar);
    int  ParseCharacterEqualSign(int nChar);
    int  ParseCharacterPlusSign(int nChar);
    int  ParseCharacterPercentSign(int nChar);
    int  ParseCharacterSemicolon();
    int  ParseCharacterCarat(int nChar);
    int  ParseCharacterTilde();
    int  ParseCharacterComma();
    int  ParseCharacterEllipsis();
    int  ParseCharacterQuestionMark();
    int  ParseCharacterColon();

    void InitializeIncludeFile(int nIndex);
    void ShutdownIncludeFile(int nIndex);
    int  ParseIdentifierFile();
    int  GenerateIdentifierList();

    void PushSRStack(int nType, int nSubType, int nLocation, CScriptParseTreeNode* pNode);
    int  PopSRStack(int* pnType, int* pnSubType, int* pnLocation,
                    CScriptParseTreeNode** ppNode, CScriptParseTreeNode** ppReturnNode);
    void ModifySRStackReturnTree(CScriptParseTreeNode* pReturnNode);

    CScriptParseTreeNode* CreateScriptParseTreeNode(int nType, CScriptParseTreeNode* pLeft,
                                                    CScriptParseTreeNode* pRight);
    CScriptParseTreeNode* DuplicateScriptParseTree(CScriptParseTreeNode* pNode);
    int  CheckForBadLValue(CScriptParseTreeNode* pNode);
    void DeleteParseTree(int nMode, CScriptParseTreeNode* pNode);
    CScriptParseTreeNode* InsertGlobalVariablesInParseTree(CScriptParseTreeNode* pNode);
    int  WalkParseTree(CScriptParseTreeNode* pNode);
    int  PreVisitGenerateCode(CScriptParseTreeNode* pNode);
    int  InVisitGenerateCode(CScriptParseTreeNode* pNode);
    int  PostVisitGenerateCode(CScriptParseTreeNode* pNode);
    int  OutputWalkTreeError(int nError, CScriptParseTreeNode* pNode);
    int  CleanUpAfterCompile(int nMode, CScriptParseTreeNode* pNode);
    int  FoundReturnStatementOnAllBranches(CScriptParseTreeNode* pNode);
    int  AddToGlobalVariableList(CScriptParseTreeNode* pNode);
    void DeleteCompileStack();

    int  AddUserDefinedIdentifier(CScriptParseTreeNode* pNode, int nFlags);
#if KOTOR1  // K1 keeps it out of line
    void ClearAllSymbolLists() __attribute__((noinline));
#else
    void ClearAllSymbolLists();
#endif
    void ClearUserDefinedIdentifiers();
    void ClearSwitchLabelList();
    int  GetStructureSize(const CExoString& sName);
    int  GetStructureField(const CExoString& sStructureName,
                           const CExoString& sFieldName);
    int  GetIdentifierByName(const CExoString& sName);
    int  AddSymbolToQueryList(const CExoString& sLabel, int nLocation);
    int  AddSymbolToLabelList(const CExoString& sLabel, int nLocation);
    void AddVariableToStack(int nType, const CExoString& sName, int nFlags);
    void AddStructureToStack(const CExoString& sName, int nFlags);
    void InitializeSwitchLabelList();
    int  GenerateCodeForSwitchLabels(CScriptParseTreeNode* pNode);
    int  ValidateLocationOfIdentifier(const CExoString& sName);

    int        m_bCompileDebugLevel;
    int        m_bCompileConditionalFile;
    CExoString m_sSourceFile;
    CExoString m_sOutputAlias;
    int        m_nLine;
    int        m_nColumn;
    int        m_nTokenType;
    int        m_nTokenLength;
    char       m_szTokenText[MAX_TOKEN_LENGTH];
    CScriptSRStackEntry* m_pSubroutineStack;
    int        m_nSubroutineStackCapacity;
    int        m_nSubroutineStackSize;
    int        m_nCompileMode;
    int        m_bCompileIdentifierConstants;
    int        m_nIdentifierListState;
    int        m_nIdentifierListVectorComponent;
    int        m_nIdentifierListEngineStructure;
    int        m_nIdentifierListReturnType;
    CScriptCompilerIdListEntry* m_pIdentifierList;
    int        m_nIdentifierCount;
    int        m_nIdentifierBase;
    int        m_nNextActionId;
    int        m_nIncludeFileDepth;
    CScriptCompilerIncludeFile m_includeFiles[INCLUDE_FILE_COUNT];
    int        m_nVarStackRecursionLevel;
    CScriptCompilerTable700Entry* m_pVarStackList;
    int        m_nVarStackTop;
    int        m_nVarStackVariableType;
    CExoString m_sVarStackVariableTypeName;
    CScriptCompilerTable720Entry* m_pStructureList;
    CScriptCompilerTable728Entry* m_pStructureFieldList;
    int        m_nStructureCount;
    int        m_nFieldCount;
    int        m_nStructureDefinition;
    int        m_nStructureDefinitionFieldStart;
    int        m_bGlobalVariableDefinition;
    int        m_nGlobalVariableCount;
    int        m_nGlobalVariableSize;
    CScriptParseTreeNode* m_pGlobalVariableParseTree;
    int        m_nLoopIdentifier;
    int        m_nSwitchLevel;
    int        m_nSwitchIdentifier;
    int        m_nSwitchStackDepth;
    CExoString m_sUndefinedIdentifier;
    int        m_bSwitchHasDefault;
    int        m_nSwitchLabelCount;
    int        m_nSwitchLabelCapacity;
    int*       m_pSwitchLabels;
    int        m_nEngineStructureCount;
    int*       m_pEngineStructureValid;
    CExoString* m_pEngineStructureNames;
    int        m_bAssignmentToVariable;
    int        m_bInStructurePart;
    int        m_bInFunctionImplementation;
    CExoString m_sFunctionImpName;
    int        m_nFunctionImpReturnType;
    CExoString m_sFunctionImpReturnStructureName;
    int        m_nFunctionImpAbortStackPointer;
    int        m_nStackCurrentDepth;
    char       m_aStackTypes[532];
    int        m_nSymbolQueryListCapacity;
    int        m_nSymbolQueryListCount;
    CScriptCompilerSymbolListEntry* m_pSymbolQueryList;
    int        m_nSymbolLabelListCapacity;
    int        m_nSymbolLabelListCount;
    CScriptCompilerSymbolListEntry* m_pSymbolLabelList;
    int        m_bCompileSymbolicOutput;
    int        m_bOptimizeBinaryCodeLength;
    int        m_bGeneratingFinalCode;
    int        m_bConditionalCompile;
    char*      m_pCompiledCode;
    int        m_nCompiledCodeSize;
    int        m_nCompiledCodeLocation;
    int        m_nBinaryCodeOffset;
    unsigned char m_aArgumentTypes[MAX_PARAMETER_COUNT];
    CExoString m_aArgumentStructureNames[MAX_PARAMETER_COUNT];
    int        m_nResolvedCodeSize;
};

K2_LAYOUT_ASSERT(sizeof(CScriptCompilerIncludeFile) == 72, "include-file slot stride");
K2_LAYOUT_ASSERT(sizeof(CScriptSRStackEntry) == 32, "SR stack entry stride");
K2_LAYOUT_ASSERT(sizeof(CScriptCompilerInternal) == 3184, "CScriptCompilerInternal span");

#endif
