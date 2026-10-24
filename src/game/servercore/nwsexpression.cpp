// The node list behind CSWSExpression's pattern matcher.

#include "k2stdint.h"
#include "nwsexpression.h"
#include "CExoArrayListImpl.h"

#include "port/libcstring.h"

CSWSExpressionNode::CSWSExpressionNode()
{
    m_pNext = NULL;
    m_nVisits = 0;
}

// The body is empty; only the m_sText teardown runs.
CSWSExpressionNode::~CSWSExpressionNode()
{
}

CSWSExpressionList::CSWSExpressionList() : m_pHead(NULL), m_pCursor(NULL), m_pTail(NULL)
{
}

CSWSExpressionList::~CSWSExpressionList()
{
}

// Frees every node from the head down, recursing into an alternation node's alternate chain
// first. The three anchors are left dangling for the owner to re-initialise.
void CSWSExpressionList::DeleteList()
{
    CSWSExpressionNode* pNode = m_pHead;
    while (pNode != NULL) {
        CSWSExpressionNode* pNext = pNode->m_pNext;
        if (pNode->m_nType == CSWSExpressionNode::NODE_ALTERNATION)
            DeleteAlternate(pNode->m_pAlternate);
        delete pNode;
        pNode = pNext;
    }
}

// Deletes an alternate chain as DeleteList does a main one. The two are separate because this one
// stops at the join node without freeing it, since that node is the point the alternate branches
// converge on and the main list still owns it.
void CSWSExpressionList::DeleteAlternate(CSWSExpressionNode* pNode)
{
    while (pNode != NULL) {
        if (pNode->m_nType == CSWSExpressionNode::NODE_JOIN)
            return;
        CSWSExpressionNode* pNext = pNode->m_pNext;
        if (pNode->m_nType == CSWSExpressionNode::NODE_ALTERNATION)
            DeleteAlternate(pNode->m_pAlternate);
        delete pNode;
        pNode = pNext;
    }
}

// Append after the cursor (not after the tail) and leave the cursor on the new node.
void CSWSExpressionList::AddNode(CSWSExpressionNode* pNode)
{
    if (m_pHead == NULL && m_pTail == NULL) {
        m_pHead = pNode;
        m_pTail = pNode;
        m_pCursor = pNode;
        return;
    }
    if (m_pCursor == m_pTail)
        m_pTail = pNode;
    pNode->m_pNext = m_pCursor->m_pNext;
    m_pCursor->m_pNext = pNode;
    m_pCursor = pNode;
}

void CSWSExpressionList::AddNodeToHead(CSWSExpressionNode* pNode)
{
    if (m_pHead == NULL && m_pTail == NULL) {
        m_pHead = pNode;
        m_pTail = pNode;
        m_pCursor = pNode;
        return;
    }
    if (m_pCursor == m_pHead)
        m_pCursor = pNode;
    pNode->m_pNext = m_pHead;
    m_pHead = pNode;
}


CSWSExpressionNode* CSWSExpression::NewNode(int nType)
{
    CSWSExpressionNode* pNode = new CSWSExpressionNode;
    pNode->m_nType = nType;
    return pNode;
}

CSWSExpression::CSWSExpression()
{
    m_pList = NULL;
    m_bCaseSensitive = 0;
    m_nId = 0;
}

// Tear down the compiled graph, then the captured match strings this class owns, then let the two
// members destroy themselves.
CSWSExpression::~CSWSExpression()
{
    if (m_pList != NULL) {
        m_pList->DeleteList();
        delete m_pList;
    }
    for (int i = 0; i < m_aMatchedStrings.m_count; i++)
        delete m_aMatchedStrings.m_pArray[i];
    m_aMatchedStrings.Clear();
}

void CSWSExpression::ClearGraph()
{
    CSWSExpressionNode* pNode = m_pList->m_pHead;
    while (pNode != NULL) {
        if (pNode->m_nType == CSWSExpressionNode::NODE_ALTERNATION)
            ClearAlternate(pNode->m_pAlternate);
        pNode->m_nVisits = 0;
        pNode = pNode->m_pNext;
    }
}

void CSWSExpression::ClearAlternate(CSWSExpressionNode* pNode)
{
    while (pNode != NULL) {
        if (pNode->m_nType == CSWSExpressionNode::NODE_ALTERNATION)
            ClearAlternate(pNode->m_pAlternate);
        else if (pNode->m_nType == CSWSExpressionNode::NODE_JOIN)
            return;
        pNode->m_nVisits = 0;
        pNode = pNode->m_pNext;
    }
}

// Compile a pattern into a fresh match graph hung off m_pList.
//
// The pattern language, from the node types this builds:
//   '('  push a nested list;  ')'  pop it and splice it into its parent
//   '|'  alternation: wrap what the current list holds so far as a NODE_ALTERNATION whose
//        alternate chain is the old head, joined by a NODE_JOIN sentinel
//   '*X' a wildcard class: '**' any run, then *w, *p, *a, *n, *A
//   anything else accumulates into a literal run, flushed as one NODE_LITERAL when the next
//        character is a metacharacter or the string ends
// The graph is terminated with a NODE_ACCEPT.
//
// Returns 1 when the pattern is already compiled, 0 when it compiled, and five negative parse
// errors: -1000 ')' with no open '(', -1001 unbalanced '(' at the end, -1002 '|' with nothing to its
// left, -1003 trailing '*', -1004 unknown '*X' class.
int CSWSExpression::ParseString(CExoString sString)
{
    if (m_bCaseSensitive == 0)
        sString = sString.LowerCase();

    if (m_sExpression == sString)
        return 1;

    if (m_pList != NULL) {
        m_pList->DeleteList();
        delete m_pList;
    }

    // The stack of open lists. Index 0 is the whole pattern, and every unmatched '(' pushes one.
    CExoArrayList<CSWSExpressionList*> aLists;
    aLists.Add(new CSWSExpressionList);

    // The literal run accumulated since the last flush.
    CExoArrayList<char> aChars;

    int nDepth = 0;
#if KOTOR1  // K1: CExoString::GetLength is out of line
    int nLength = sString.GetLength();
#else
    int nLength = (sString.m_sString != NULL) ? static_cast<int>(strlen(sString.m_sString)) : 0;
#endif
    for (int i = 0; i < nLength; i++) {
        char ch = sString[i];
        switch (ch) {
        case '(':
            aLists.Add(new CSWSExpressionList);
            nDepth++;
            break;

        case ')': {
            if (nDepth < 1)
                return -1000;
            CSWSExpressionList* pOuter = aLists.m_pArray[nDepth - 1];
            CSWSExpressionList* pInner = aLists.m_pArray[nDepth];
            if (pOuter->m_pHead == NULL) {
                pOuter->m_pHead = pInner->m_pHead;
                pOuter->m_pTail = pInner->m_pTail;
            } else {
                pInner->m_pTail->m_pNext = pOuter->m_pCursor->m_pNext;
                if (pOuter->m_pCursor == pOuter->m_pTail)
                    pOuter->m_pTail = pInner->m_pTail;
                pOuter->m_pCursor->m_pNext = pInner->m_pHead;
            }
            pOuter->m_pCursor = pInner->m_pTail;
            delete pInner;
            aLists.DelIndex(nDepth);
            nDepth--;
            break;
        }

        case '*': {
            i++;
            if (i >= nLength)
                return -1003;
            char chClass = sString[i];
            switch (chClass) {
            case '*':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_ANY_RUN));
                break;
            case 'A':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_ALPHANUMERIC));
                break;
            case 'a':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_LETTER));
                break;
            case 'n':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_DIGIT));
                break;
            case 'p':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_PUNCTUATION));
                break;
            case 'w':
                aLists.m_pArray[nDepth]->AddNode(NewNode(CSWSExpressionNode::NODE_SPACE));
                break;
            default:
                return -1004;
            }
            break;
        }

        case '|': {
            if (aLists.m_pArray[nDepth]->m_pHead == NULL)
                return -1002;
            CSWSExpressionNode* pJoin = NewNode(CSWSExpressionNode::NODE_JOIN);
            CSWSExpressionNode* pAlt = NewNode(CSWSExpressionNode::NODE_ALTERNATION);
            aLists.m_pArray[nDepth]->AddNode(pJoin);
            pAlt->m_pAlternate = aLists.m_pArray[nDepth]->m_pHead;
            pAlt->m_pNext = pJoin;
            aLists.m_pArray[nDepth]->m_pHead = pAlt;
            aLists.m_pArray[nDepth]->m_pCursor = pAlt;
            break;
        }

        default: {
            aChars.Add(ch);
            // Keep accumulating while the next character is another literal; otherwise flush the
            // run as one literal node.
#if KOTOR1  // K1: CExoString::GetLength is out of line
            if (i + 1 < sString.GetLength()) {
#else
            if (i + 1 < ((sString.m_sString != NULL) ? static_cast<int>(strlen(sString.m_sString)) : 0)) {
#endif
                char chNext = sString[i + 1];
                if (chNext != '(' && chNext != ')' && chNext != '*' && chNext != '|')
                    break;
            }
            CSWSExpressionNode* pNode = NewNode(CSWSExpressionNode::NODE_LITERAL);
            pNode->m_sText = CExoString(aChars.m_pArray, aChars.m_count);
            aChars.m_count = 0;
            aLists.m_pArray[nDepth]->AddNode(pNode);
            break;
        }
        }
    }

    if (nDepth > 0)
        return -1001;

    // Terminate with the accept node. With a tail already present it is linked in directly
    // rather than through AddNode, which would also move the insertion cursor.
    CSWSExpressionNode* pTail = aLists.m_pArray[0]->m_pTail;
    if (pTail != NULL)
        pTail->m_pNext = NewNode(CSWSExpressionNode::NODE_ACCEPT);
    else
        aLists.m_pArray[0]->AddNode(NewNode(CSWSExpressionNode::NODE_ACCEPT));

    m_pList = aLists.m_pArray[0];
    m_sExpression = sString;
    return 0;
}

// Run a subject past the compiled graph. Returns 1 on a match, 0 otherwise.
//
// This is a backtracking walk rather than an NFA simulation. aPath is the list of nodes on the path
// taken so far, nIndex the node last committed to, and each node records in m_nPos how far into the
// subject the match had got when the walk left it. On failure the walk pops aPath until it finds a
// node that can be retried: a literal never can, a wildcard or an alternation can, and m_nVisits
// selects the next alternative. m_nVisits resets to 0 whenever a node is entered forwards, so
// "visited again" means "backtracked into".
//
// aPath[0] is a throwaway join node that gives the backtrack walk a bottom to hit; it is deleted
// on the way out and never linked into the graph.
int CSWSExpression::TestString(const CExoString& sString, int bStoreMatches)
{
    CExoString sTest;
    if (m_bCaseSensitive)
        sTest = sString;
    else
        sTest = sString.LowerCase();

    if (m_pList == NULL)
        return 0;

    CSWSExpressionNode* pNode = m_pList->m_pHead;

    CExoArrayList<CSWSExpressionNode*> aPath;
    aPath.Add(NewNode(CSWSExpressionNode::NODE_JOIN));

#if KOTOR1  // K1: CExoString::GetLength is out of line
    int nLen = sTest.GetLength();
#else
    int nLen = (sTest.m_sString != NULL) ? static_cast<int>(strlen(sTest.m_sString)) : 0;
#endif
    int nPos = 0;
    int nIndex = 0;

    while (pNode != NULL) {
        pNode->m_nVisits++;
        switch (pNode->m_nType) {
        case CSWSExpressionNode::NODE_LITERAL: {
            // For a literal run, the whole of m_sText has to be present at nPos.
            aPath.Add(pNode);
#if KOTOR1  // K1: CExoString::GetLength is out of line
            int nTextLen =
                pNode->m_sText.GetLength();
#else
            int nTextLen =
                (pNode->m_sText.m_sString != NULL) ? static_cast<int>(strlen(pNode->m_sText.m_sString)) : 0;
#endif
            for (int i = 0; i < nTextLen; i++) {
                if (sTest[nPos + i] != pNode->m_sText[i]) {
                    nIndex++;
                    goto backtrack;
                }
            }
            nPos += nTextLen;
            break;
        }

        case CSWSExpressionNode::NODE_ANY_RUN:
            // A wildcard swallows one more character each time the walk returns here; at the end
            // of the subject there is nothing left to give and the match fails.
            aPath.Add(pNode);
            if (nPos == nLen && pNode->m_nVisits >= 2) {
                delete aPath.m_pArray[0];
                ClearGraph();
                return 0;
            }
            if (pNode->m_nVisits > 1)
                nPos++;
            break;

        case CSWSExpressionNode::NODE_ALTERNATION:
            // For an alternation, m_pNext is the first branch, m_pAlternate the second.
            pNode->m_nPos = nPos;
            aPath.Add(pNode);
            if (pNode->m_nVisits == 2) {
                nIndex++;
                pNode = pNode->m_pAlternate;
                pNode->m_nVisits = 0;
                continue;
            }
            if (pNode->m_nVisits == 1) {
                nIndex++;
                pNode = pNode->m_pNext;
                pNode->m_nVisits = 0;
                continue;
            }
            aPath.DelIndex(nIndex + 1);
            goto backtrack;

        case CSWSExpressionNode::NODE_SPACE:
        case CSWSExpressionNode::NODE_PUNCTUATION:
        case CSWSExpressionNode::NODE_LETTER:
        case CSWSExpressionNode::NODE_DIGIT:
        case CSWSExpressionNode::NODE_ALPHANUMERIC: {
            // All five character classes share one arm, re-reading m_nType to pick the test.
            if (nPos == nLen) {
                delete aPath.m_pArray[0];
                ClearGraph();
                return 0;
            }
            aPath.Add(pNode);
            char ch = sTest[nPos];
            if ((pNode->m_nType == CSWSExpressionNode::NODE_LETTER && ch >= 'a' && ch <= 'z') ||
                (pNode->m_nType == CSWSExpressionNode::NODE_DIGIT && ch >= '0' && ch <= '9') ||
                (pNode->m_nType == CSWSExpressionNode::NODE_ALPHANUMERIC &&
                 ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))) ||
                (pNode->m_nType == CSWSExpressionNode::NODE_SPACE && ch == ' ') ||
                (pNode->m_nType == CSWSExpressionNode::NODE_PUNCTUATION &&
                 (ch == '!' || ch == '.' || ch == '?'))) {
                nPos++;
                break;
            }
            aPath.DelIndex(nIndex + 1);
            goto backtrack;
        }

        case CSWSExpressionNode::NODE_ACCEPT:
            // The accept state, but only once the whole subject was consumed.
            if (nPos != nLen)
                goto backtrack;
            if (bStoreMatches) {
                for (int i = 0; i < m_aMatchedStrings.m_count; i++)
                    delete m_aMatchedStrings.m_pArray[i];
                m_aMatchedStrings.Clear();
                // Slot 0 is the whole subject; the rest are the pieces each path node consumed.
                // Alternation nodes carry no text of their own and contribute no capture.
                m_aMatchedStrings.Add(new CExoString(sString));
                int nStart = 0;
                for (int i = 1; i < aPath.m_count; i++) {
                    if (aPath.m_pArray[i]->m_nType != CSWSExpressionNode::NODE_ALTERNATION) {
                        m_aMatchedStrings.Add(new CExoString(sString.CStr() + nStart,
                                                             aPath.m_pArray[i]->m_nPos - nStart));
                        nStart = aPath.m_pArray[i]->m_nPos;
                    }
                }
            }
            delete aPath.m_pArray[0];
            ClearGraph();
            return 1;

        default:
            // The alternation join sentinel matches nothing.
            pNode = pNode->m_pNext;
            pNode->m_nVisits = 0;
            continue;
        }

        pNode->m_nPos = nPos;
        nIndex++;
        pNode = pNode->m_pNext;
        pNode->m_nVisits = 0;
        continue;

    backtrack:
        // Unwind the path until something can be retried. Literals are dropped outright; anything
        // else is put back in play with the position it had reached, and its untouched m_nVisits
        // picks the next alternative.
        while (1) {
            pNode = aPath.m_pArray[nIndex];
            aPath.DelIndex(nIndex);
            if (nIndex <= 0) {
                delete aPath.m_pArray[0];
                ClearGraph();
                return 0;
            }
            nIndex--;
            if (pNode->m_nType != CSWSExpressionNode::NODE_LITERAL)
                break;
        }
        nPos = pNode->m_nPos;
    }

    // Ran off the end of the graph without reaching the accept node.
    delete aPath.m_pArray[0];
    ClearGraph();
    return 1;
}
