// The subroutine-stack accessor, in its own translation unit so that GenerateFinalCodeFromParseTree
// only sees a declaration and the call stays out of line.

#include "nwscriptinternal.h"

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
