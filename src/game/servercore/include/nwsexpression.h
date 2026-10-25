#pragma once

// One entry of a server object's listen-expression table. A listening object registers (id, pattern)
// pairs; every line of speech in earshot is run past TestString, and the id of the first pattern that
// matches is what the object's OnDialogue script receives.

#include "exostring.h"
#include "CExoArrayListDtors.h"

class CSWSExpressionNode;
class CSWSExpressionList;

class CSWSExpression {
public:
    // Declared only, so that callers emit a real call.
    CSWSExpression();

    int m_nId;  // the id handed back to script when this pattern matches

    unsigned char _pad04[0x08 - 0x04];

    // The substrings captured by the last successful match, owned here. Element 0 is the whole
    // match, which never reaches script.
    CExoArrayList<CExoString*> m_aMatchedStrings;

    // When clear, both the pattern and the subject text are lower-cased first.
    int           m_bCaseSensitive;
    unsigned char _pad1c[0x20 - 0x1c];

    // The compiled match graph, allocated by ParseString and owned here.
    CSWSExpressionList* m_pList;

    // The pattern text, saved as the GFF key "ExpressionString".
    CExoString m_sExpression;

    // Declared on purpose, because the real destructor tears the match graph down and frees the
    // captured strings, so leaving it implicit would synthesise and expand a trivial one.
    ~CSWSExpression();

    // Allocates a fresh graph node of the given type. Linking it into the list is a separate step.
    CSWSExpressionNode* NewNode(int nType);

    // A pattern metacharacter: grouping, alternation or a '*' class.
    int IsAToken(char c) { return c == '(' || c == '*' || c == '|' || c == ')'; }

    // Resets every node's visit count across the whole graph, recursing into an alternation node's
    // alternate chain.
    void ClearGraph();

    // ClearGraph's twin for the alternate chain does the same walk, but stops at the shared join
    // node instead of clearing it.
    void ClearAlternate(CSWSExpressionNode* pNode);

    // Whether the string matches this expression, by a backtracking walk of the compiled graph. When
    // bStoreMatches is set, a successful match replaces the capture list with the whole subject in slot
    // 0 followed by one capture per path node. Not const: every exit resets the graph's per-node visit
    // counters.
    int TestString(const CExoString& sString, int bStoreMatches);

    // Compiles the string into this expression's match graph, replacing whatever was there. Takes its
    // argument by value. Returns 1 on success, 0 or a negative parse error otherwise.
    int ParseString(CExoString sString);
};

#include <stddef.h>

// An intrusive singly-linked list of expression nodes, used while an expression compiles a pattern
// into its match graph. All three anchors are null on construction.
class CSWSExpressionNode;

class CSWSExpressionList {
public:
    CSWSExpressionNode* m_pHead;
    CSWSExpressionNode* m_pCursor;  // the "insert after me" position
    CSWSExpressionNode* m_pTail;

    CSWSExpressionList();
    // Empty, user-declared, and out of line.
    ~CSWSExpressionList();

    void DeleteList();
    void DeleteAlternate(CSWSExpressionNode* pNode);
    void AddNode(CSWSExpressionNode* pNode);
    void AddNodeToHead(CSWSExpressionNode* pNode);
};

// One node of a compiled match graph.

class CSWSExpressionNode {
public:
    // The backtrack visit counter, zeroed when the walk reaches this node forwards from a
    // predecessor and incremented when it comes back here. A count above 1 therefore means
    // "returned by backtracking", which is how the two repeatable node kinds decide what to consume
    // next.
    int                 m_nVisits;
    // The node kind NewNode stores, by the pattern syntax ParseString builds it from. The five
    // character classes each match one character.
    enum {
        NODE_LITERAL      = 0,  // a literal run, m_sText
        NODE_ANY_RUN      = 1,  // "**"
        NODE_ALTERNATION  = 2,  // '|', owning the alternate chain m_pAlternate
        NODE_JOIN         = 3,  // the sentinel the alternates converge on
        NODE_SPACE        = 4,  // "*w": ' '
        NODE_PUNCTUATION  = 5,  // "*p": '!', '.' or '?'
        NODE_LETTER       = 6,  // "*a": 'a'-'z'
        NODE_DIGIT        = 7,  // "*n": '0'-'9'
        NODE_ALPHANUMERIC = 8,  // "*A": a letter or a digit
        NODE_ACCEPT       = 9,  // the end of the pattern
    };
    int                 m_nType;
    // The subject position the match had reached when it last left this node; restored on backtracking,
    // and read after a successful match to cut the subject into captures.
    int                 m_nPos;
    char                _pad0c[0x10 - 0x0c];
    CSWSExpressionNode* m_pNext;
    CExoString          m_sText;      // the literal text, and the only non-trivial member
    CSWSExpressionNode* m_pAlternate; // head of the alternate chain, valid for NODE_ALTERNATION

    CSWSExpressionNode();
    // Empty, user-declared; the generated part destroys m_sText.
    ~CSWSExpressionNode();
};
