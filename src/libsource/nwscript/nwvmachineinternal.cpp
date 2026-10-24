// The NWScript VM, CVirtualMachineInternal, and the operand stack it runs on.
//
// The script-situation array (m_aScriptSituations[8]) is the spine of this file. The VM's own
// constructor and destructor walk it, DeleteScript folded in at each step.

#include "k2stdint.h"
#include "nwvmachineinternal.h"
#include "nwscriptinternal.h"
#include "Vector.h"
#include "resgff.h"
#include "gameobjectarray.h"

#include <string.h>

enum { SCRIPT_SITUATION_COUNT = 8 };        // m_aScriptSituations
enum { INSTRUCTION_PTR_STACK_SIZE = 128 };  // m_anInstructionPtrs

// One run of ExecuteCode may execute at most this many instructions.
enum { MAX_INSTRUCTIONS = 131071 };

// CopyFromStack copies at most this many cells from each side of the frame, and gives the copy
// STACK_COPY_HEADROOM spare cells.
enum { MAX_STACK_COPY_CELLS = 65536 };
enum { STACK_COPY_HEADROOM = 16 };

CVirtualMachineInternal::CVirtualMachineInternal()
{
    m_nSituationCount = -1;
    m_pCmdImplementer = 0;
    m_sStack.ClearStack();
    m_sStack.m_pOwner = this;
}

CVirtualMachineInternal::~CVirtualMachineInternal()
{
    for (int i = 0; i < SCRIPT_SITUATION_COUNT; i++)
        DeleteScript(&m_aScriptSituations[i]);
}
int CVirtualMachineInternal::DeleteScript(CVirtualMachineScript* pScript)
{
    CVirtualMachineStack* pStack = pScript->m_pStack;
    if (pStack != 0) {
        delete pStack;
        pScript->m_pStack = 0;
    }
    if (pScript->m_sData != 0) {
        delete[] pScript->m_sData;
        pScript->m_sData = 0;
    }
    pScript->m_nStackSize = 0;
    pScript->m_nInstructionPtr = NCS_HEADER_SIZE;
    pScript->m_nSecondaryPtr = 0;
    pScript->m_nDataSize = 0;
    pScript->m_nCRC = 0;
    return 1;
}

void CVirtualMachineInternal::DeleteScriptSituation(CVirtualMachineScript* pSituation)
{
    delete pSituation;
}

void CVirtualMachineInternal::InitializeScript(CVirtualMachineScript* pScript, char* pData, int nSize)
{
    pScript->m_nSituationIndex = 0;
    pScript->m_pStack = 0;
    pScript->m_nStackSize = 0;
    pScript->m_nInstructionPtr = NCS_HEADER_SIZE;
    pScript->m_nSecondaryPtr = 0;
    pScript->m_nDataSize = nSize;
    pScript->m_sData = new char[nSize];
    memcpy(pScript->m_sData, pData, nSize);  // idiom-exempt: a runtime byte count, not a sizeof
    pScript->m_nCRC = 0;
}

// Moves a caller-owned script situation into the array's next free slot and empties the
// source (the stack and image pointers are handed over, not copied). Returns
// -SCRIPT_ERROR_TOO_MANY_LEVELS_OF_RECURSION when the array is full.
int CVirtualMachineInternal::SetUpScriptSituation(CVirtualMachineScript* pSituation)
{
    int nSituationIndex;
    if (pSituation->m_sData == 0) {
        int nResult = ReadScriptFile(&pSituation->m_sName);
        if (nResult != 0)
            return nResult;
        nSituationIndex = 0;
    } else {
        nSituationIndex = pSituation->m_nSituationIndex;
        int nOld = m_nSituationCount;
        m_nSituationCount = nOld + 1;
        if (nOld > SCRIPT_SITUATION_COUNT - 2) {
            m_nSituationCount = nOld;
            return -SCRIPT_ERROR_TOO_MANY_LEVELS_OF_RECURSION;
        }
    }
    m_aScriptSituations[m_nSituationCount].m_nSituationIndex = nSituationIndex;
    m_aScriptSituations[m_nSituationCount].m_sName = pSituation->m_sName;
    m_aScriptSituations[m_nSituationCount].m_pStack = pSituation->m_pStack;
    m_aScriptSituations[m_nSituationCount].m_nStackSize = pSituation->m_nStackSize;
    m_aScriptSituations[m_nSituationCount].m_nInstructionPtr = pSituation->m_nInstructionPtr;
    m_aScriptSituations[m_nSituationCount].m_nSecondaryPtr = pSituation->m_nSecondaryPtr;
    m_aScriptSituations[m_nSituationCount].m_nCRC = pSituation->m_nCRC;
    if (pSituation->m_sData != 0) {
        m_aScriptSituations[m_nSituationCount].m_sData = pSituation->m_sData;
        m_aScriptSituations[m_nSituationCount].m_nDataSize = pSituation->m_nDataSize;
    }
    pSituation->m_pStack = 0;
    pSituation->m_sData = 0;
    return 0;
}

// Compiles the named script into the next situation slot and runs it, holding the stack
// pointer the script must return to.
int CVirtualMachineInternal::RunScript(CExoString* pScriptName, unsigned long long objectId,
                                       int nParam)
{
#if KOTOR1  // K1: CExoString::IsEmpty out of line, compared against 1
    if (pScriptName == 0 || pScriptName->IsEmpty() == 1)
        return 0;
#else
    if (pScriptName == 0 || pScriptName->m_sString == 0 || *pScriptName->m_sString == 0)
        return 0;
#endif

    // -1 is "no situation live": the very first entry re-seats the embedding VM's own stack.
    if (m_nSituationCount == -1) {
        m_sStack.ClearStack();
        m_sStack.m_pOwner = this;
        m_nInstructionsExecuted = 0;
        m_nInstructionPtr = 0;
    }

    int nResult = ReadScriptFile(pScriptName);
    if (nResult != 0) {
        if (m_nSituationCount != -1)
            pScriptName->CStr();
        return 0;
    }

    K2_ASSUME(m_nSituationCount >= 0 && m_nSituationCount < SCRIPT_SITUATION_COUNT);   // ReadScriptFile pushed a slot, refusing an 8th
    m_aScriptSituations[m_nSituationCount].m_nCRC = 0;
    m_anScriptParam[m_nSituationCount] = nParam;
    m_anScriptOid[m_nSituationCount] = objectId;
    if (m_pCmdImplementer != 0) {
        m_pCmdImplementer->RunScriptCallback(*pScriptName);
        m_pCmdImplementer->m_nScriptRunArg = m_anScriptParam[m_nSituationCount];
        m_pCmdImplementer->m_oidSelf = m_anScriptOid[m_nSituationCount];
    }

    unsigned long long nSpBefore = m_sStack.m_nStackPointer;
    nResult = RunScriptFile(0);
    DeleteScript(&m_aScriptSituations[m_nSituationCount]);
    int nOldCount = m_nSituationCount;
    m_nSituationCount--;
    // Restore the frame the script was entered with, now that the finished one is gone. This is the
    // same publish RunScript does on entry, but from the slot that is now on top again.
    if (nOldCount != 0 && m_pCmdImplementer != 0) {
        K2_ASSUME(m_nSituationCount >= 0 && m_nSituationCount < SCRIPT_SITUATION_COUNT - 1);   // the popped slot was 1..7
        m_pCmdImplementer->m_nScriptRunArg = m_anScriptParam[m_nSituationCount];
        m_pCmdImplementer->m_oidSelf = m_anScriptOid[m_nSituationCount];
    }
    // The name is fetched and discarded for a nested script on both outcomes.
    if (nOldCount != 0)
        pScriptName->CStr();
    if (nResult != 0)
        return 0;

    // Exactly one value left above where the script started is its return value; a stack that came
    // back level means it produced nothing, and anything else is a runaway, trimmed away without
    // being read.
    int nReturn;
    int nSp = static_cast<int>(m_sStack.m_nStackPointer);
    if (nSp == static_cast<int>(nSpBefore) + 1) {
        if (m_sStack.m_pTypeData[static_cast<int>(nSpBefore)] == NCS_TYPE_INTEGER) {
            m_nReturnValueType = NCS_TYPE_INTEGER;
            m_pReturnValue = m_sStack.m_pValueData[static_cast<int>(nSpBefore)];
        }
        m_sStack.SetStackPointer(static_cast<int>(nSpBefore));
        nReturn = 1;
    } else if (nSp == static_cast<int>(nSpBefore)) {
        nReturn = 1;
    } else {
        m_sStack.SetStackPointer(static_cast<int>(nSpBefore));
        nReturn = 0;
    }
    if (m_nSituationCount == -1)
        m_sStack.ClearStack();
    return nReturn;
}

// Runs one saved situation: re-seats the operand stack from the situation's own saved copy,
// publishes the (oid, param) pair to the command implementer, then executes the
// image at the new slot.
int CVirtualMachineInternal::RunScriptSituation(CVirtualMachineScript* pSituation,
                                                unsigned long long objectId, int nParam)
{
    m_sStack.ClearStack();
    m_sStack.m_nStackPointer = pSituation->m_pStack->m_nStackPointer;
    m_sStack.m_nCapacity = pSituation->m_pStack->m_nCapacity;
    m_sStack.m_pOwner = this;
    m_sStack.CopyFromStack(pSituation->m_pStack, 0, 0);
    if (pSituation->m_pStack != 0) {
        delete pSituation->m_pStack;
        pSituation->m_pStack = 0;
    }
    m_nInstructionPtr = 0;
    m_nInstructionsExecuted = 0;

    if (SetUpScriptSituation(pSituation) != 0) {
        delete pSituation;
        return 0;
    }
    // m_nCRC is the slot's "already torn down" flag: DeleteScript just cleared it, so a set flag
    // means SetUpScriptSituation only bumped the counter and wrote nothing usable -- undo the slot.
    if (m_aScriptSituations[m_nSituationCount].m_nCRC != 0) {
        DeleteScript(&m_aScriptSituations[m_nSituationCount]);
        m_nSituationCount--;
        delete pSituation;
        return 0;
    }

    m_anScriptParam[m_nSituationCount] = nParam;
    m_anScriptOid[m_nSituationCount] = objectId;
    if (m_pCmdImplementer != 0) {
        m_pCmdImplementer->m_nScriptRunArg = m_anScriptParam[m_nSituationCount];
        m_pCmdImplementer->m_oidSelf = m_anScriptOid[m_nSituationCount];
    }

    // m_nInstructionPtr, not m_nDataSize: the situation's own image address is what RunScriptFile reads.
    int nResult = RunScriptFile(pSituation->m_nInstructionPtr);
    DeleteScript(&m_aScriptSituations[m_nSituationCount]);
    m_nSituationCount--;
    delete pSituation;
    return nResult == 0;
}

// Interprets NWScript bytecode in one loop over the opcode stream, each arm reading its operands
// off m_sStack, doing its operation, and advancing *pnOffset by the opcode's own encoded length.
//
// The operand stack's cells are 8 bytes with a type tag byte per cell (m_pTypeData): one of the
// single NCS types, NCS_TYPE_INTEGER ... NCS_TYPE_OBJECT (a string cell owns a heap CExoString) or
// an engine structure owned by m_pCmdImplementer. A binary operator's type byte (nType) names the
// operand pair instead, NCS_TYPE_INTEGER_INTEGER and on.
//
// Returns 0, or a negated SCRIPT_ERROR_* strref. SCRIPT_ERROR_INVALID_AUX_CODE is a type byte the
// opcode does not take; SCRIPT_ERROR_STACK_OVERFLOW also covers a missing command implementer and a
// full instruction-pointer table; SCRIPT_ERROR_STACK_UNDERFLOW also covers a bad frame pointer.
int CVirtualMachineInternal::ExecuteCode(int* pnOffset, char* pCode, int nSize)
{
    int nValue1;
    int nValue2;
    float fValue1;
    float fValue2;
    int nResult;

    while (*pnOffset != -1) {
        if (*pnOffset + 1 >= nSize)
            return -SCRIPT_ERROR_IP_OUT_OF_CODE_SEGMENT;
        if (m_nInstructionsExecuted++ > MAX_INSTRUCTIONS - 1)
            return -SCRIPT_ERROR_TOO_MANY_INSTRUCTIONS;

        int nOpcode = static_cast<unsigned char>(pCode[*pnOffset]);
        int nType = static_cast<unsigned char>(pCode[*pnOffset + 1]);
        if (static_cast<unsigned>(nOpcode - NCS_OP_CPDOWNSP) > NCS_OP_STORE_STATE - NCS_OP_CPDOWNSP)
            return -SCRIPT_ERROR_INVALID_OP_CODE;

        switch (nOpcode) {
        // Copy down / copy to top move nCount/4 cells of the value stack.
        case NCS_OP_CPDOWNSP:
        case NCS_OP_CPDOWNBP: {
            uint64_t* pBase;
            if (nOpcode == NCS_OP_CPDOWNSP)
                pBase = &m_sStack.m_nStackPointer;
            else if (nOpcode == NCS_OP_CPDOWNBP)
                pBase = &m_sStack.m_nBasePointer;
            else
                pBase = &m_sStack.m_nStackPointer;

            int nOffset = *pnOffset;
            int nVal = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                       (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                       (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                       static_cast<unsigned char>(pCode[nOffset + 5]);
            int nDst = static_cast<int>(*pBase) - (-nVal >> 2);
            if (nDst < 0)
                return -SCRIPT_ERROR_STACK_UNDERFLOW;

            int nCount = static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 6]) << 8) |
                                 static_cast<unsigned char>(pCode[nOffset + 7]));
            if (nCount >= 4) {
                int nWords = nCount >> 2;
                for (int i = 0; i < nWords; i++)
                    m_sStack.AssignLocationToLocation(
                        static_cast<int>(m_sStack.m_nStackPointer) - nWords + i, nDst + i);
            }
            *pnOffset += 8;
            break;
        }

        // The same move with the two ends swapped.
        case NCS_OP_CPTOPSP:
        case NCS_OP_CPTOPBP: {
            uint64_t* pBase;
            if (nOpcode == NCS_OP_CPTOPSP)
                pBase = &m_sStack.m_nStackPointer;
            else if (nOpcode == NCS_OP_CPTOPBP)
                pBase = &m_sStack.m_nBasePointer;
            else
                pBase = &m_sStack.m_nStackPointer;

            int nOffset = *pnOffset;
            int nVal = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                       (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                       (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                       static_cast<unsigned char>(pCode[nOffset + 5]);
            int nDst = static_cast<int>(*pBase) - (-nVal >> 2);
            if (nDst < 0)
                return -SCRIPT_ERROR_STACK_UNDERFLOW;

            int nCount = static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 6]) << 8) |
                                 static_cast<unsigned char>(pCode[nOffset + 7]));
            if (nCount >= 4) {
                int nWords = nCount >> 2;
                int nSp = static_cast<int>(m_sStack.m_nStackPointer);
                for (int i = 0; i < nWords; i++)
                    m_sStack.AssignLocationToLocation(nDst + i, nSp + i);
            }
            *pnOffset += 8;
            break;
        }

        // DECxSP / INCxSP, and the base-pointer pair. A non-int cell, or a stack with no
        // owner, makes the opcode a silent no-op that still advances *pnOffset; the cell is
        // incremented as a whole 8-byte value.
        case NCS_OP_DECISP:
        case NCS_OP_INCISP:
        case NCS_OP_DECIBP:
        case NCS_OP_INCIBP: {
            if (nType != NCS_TYPE_INTEGER) {
                *pnOffset += 6;
                break;
            }
            int nOffset = *pnOffset;
            int nVal = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                       (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                       (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                       static_cast<unsigned char>(pCode[nOffset + 5]);
            unsigned long long nBase;
            if (nOpcode == NCS_OP_DECISP || nOpcode == NCS_OP_INCISP)
                nBase = m_sStack.m_nStackPointer;
            else
                nBase = m_sStack.m_nBasePointer;
            int nDst = static_cast<int>(nBase) - (-nVal >> 2);
            if (nDst < 0)
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (m_sStack.m_pOwner == 0 || m_sStack.m_pTypeData[nDst] != NCS_TYPE_INTEGER) {
                *pnOffset += 6;
                break;
            }
            if (nOpcode == NCS_OP_INCISP || nOpcode == NCS_OP_INCIBP)
                m_sStack.m_pValueData[nDst] =
                    reinterpret_cast<void*>(reinterpret_cast<long long>(m_sStack.m_pValueData[nDst]) + 1);
            else
                m_sStack.m_pValueData[nDst] =
                    reinterpret_cast<void*>(reinterpret_cast<long long>(m_sStack.m_pValueData[nDst]) - 1);
            *pnOffset += 6;
            break;
        }

        // Reserve one cell and seed it with the type's zero.
        case NCS_OP_RSADD: {
            switch (nType) {
            case NCS_TYPE_INTEGER:
                StackPushInteger(0);
                break;
            case NCS_TYPE_FLOAT:
                StackPushFloat(0.0f);
                break;
            case NCS_TYPE_STRING:
                StackPushString(CExoString(""));
                break;
            case NCS_TYPE_OBJECT:
                StackPushObject(OBJECT_INVALID);
                break;
            default:
                if (nType < NCS_TYPE_ENGINE_STRUCTURE0 || nType > NCS_TYPE_ENGINE_STRUCTURE9)
                    return -SCRIPT_ERROR_INVALID_AUX_CODE;
                if (m_pCmdImplementer == 0)
                    return -SCRIPT_ERROR_STACK_OVERFLOW;
                {
                    void* pNew = m_pCmdImplementer->CopyGameDefinedStructure(nType - NCS_TYPE_ENGINE_STRUCTURE0, 0);
                    m_sStack.AddToTopOfStack(nType);
                    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = pNew;
                }
                break;
            }
            *pnOffset += 2;
            break;
        }

        // A literal baked into the instruction, typed by nType.
        case NCS_OP_CONST: {
            int nOffset = *pnOffset;
            switch (nType) {
            case NCS_TYPE_INTEGER: {
                int nValue = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                             (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                             (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                             static_cast<unsigned char>(pCode[nOffset + 5]);
                StackPushInteger(nValue);
                *pnOffset += 6;
                break;
            }
            case NCS_TYPE_FLOAT: {
                int nBits = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                            (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                            (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                            static_cast<unsigned char>(pCode[nOffset + 5]);
                StackPushFloat(*reinterpret_cast<float*>(&nBits));
                *pnOffset += 6;
                break;
            }
            case NCS_TYPE_STRING: {
                int nLen = static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 2]) << 8) |
                                   static_cast<unsigned char>(pCode[nOffset + 3]));
                char* pData = new char[nLen + 1];
                for (int i = 0; i < nLen; i++)
                    pData[i] = pCode[nOffset + 4 + i];
                pData[nLen] = 0;
                CExoString sValue;
                sValue.Format(const_cast<char*>("%s"), pData);
                delete[] pData;
                StackPushString(sValue);
                // A string literal is nLen + 4 bytes; the other arms are a fixed 6, so the
                // string arm must advance the offset by the extra length itself.
                *pnOffset += nLen + 4;
                break;
            }
            default: {
                if (nType != NCS_TYPE_OBJECT)
                    return -SCRIPT_ERROR_INVALID_AUX_CODE;
                int nId = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                          (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                          (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                          static_cast<unsigned char>(pCode[nOffset + 5]);
                // A zero id means "this situation's object"; every other value pushes
                // OBJECT_INVALID rather than the id the code carries.
                unsigned long long oid = OBJECT_INVALID;
                if (nId == 0 && m_anScriptParam[m_nSituationCount] == 1)
                    oid = m_anScriptOid[m_nSituationCount];
                StackPushObject(oid);
                *pnOffset += 6;
                break;
            }
            }
            break;
        }

        // Hand the command to the implementer.
        case NCS_OP_ACTION: {
            if (m_pCmdImplementer == 0)
                return -SCRIPT_ERROR_COMMAND_IMPLEMENTER_NOT_SET;
            int nCommand = static_cast<short>((static_cast<unsigned char>(pCode[*pnOffset + 2]) << 8) |
                                   static_cast<unsigned char>(pCode[*pnOffset + 3]));
            nResult = m_pCmdImplementer->ExecuteCommand(nCommand, static_cast<int>(static_cast<unsigned char>(pCode[*pnOffset + 4])));
            if (nResult < 0)
                return nResult;
            *pnOffset += 5;
            break;
        }

        // The bitwise and/or/xor group and the logical and/or pair.
        case NCS_OP_LOGANDII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger((nValue2 != 0) & (nValue1 != 0));
            *pnOffset += 2;
            break;
        }
        case NCS_OP_LOGORII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger((nValue1 | nValue2) != 0);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_INCORII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue1 | nValue2);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_EXCORII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue1 ^ nValue2);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_BOOLANDII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue1 & nValue2);
            *pnOffset += 2;
            break;
        }

        // Equality and inequality compare nWords/2 cell pairs.
        case NCS_OP_EQUAL:
        case NCS_OP_NEQUAL: {
            int nWords;
            int nAdvance;
            if (nType >= NCS_TYPE_INTEGER_INTEGER && nType <= NCS_TYPE_STRUCT_STRUCT) {
                if (nType == NCS_TYPE_STRUCT_STRUCT) {
                    nWords = static_cast<short>((static_cast<unsigned char>(pCode[*pnOffset + 2]) << 8) |
                                     static_cast<unsigned char>(pCode[*pnOffset + 3]));
                    nAdvance = 4;
                } else {
                    nWords = 4;
                    nAdvance = 2;
                }
            } else if (nType >= NCS_TYPE_ENGINE_STRUCTURE0_PAIR && nType <= NCS_TYPE_ENGINE_STRUCTURE9_PAIR) {
                nWords = 4;
                nAdvance = 2;
            } else
                return -SCRIPT_ERROR_INVALID_AUX_CODE;

            int nPairs = nWords >> 2;
            if (static_cast<int>(m_sStack.m_nStackPointer) < (nWords >> 1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;

            int bEqual = 1;
            if (nWords >= 4) {
                int nTop = static_cast<int>(m_sStack.m_nStackPointer) - 1;
                for (int i = 0; i < nPairs; i++, nTop--) {
                    int nTag = static_cast<unsigned char>(m_sStack.m_pTypeData[nTop]);
                    if (nTag == NCS_TYPE_STRING) {
                        CExoString* pA = static_cast<CExoString*>(m_sStack.m_pValueData[nTop]);
                        CExoString* pB = static_cast<CExoString*>(m_sStack.m_pValueData[nTop - nPairs]);
                        // CompareNoCase returns 1 when the strings are equal, so a zero
                        // result clears bEqual.
                        if (!pA->CompareNoCase(*pB))
                            bEqual = 0;
                    } else if (nTag == NCS_TYPE_INTEGER || nTag == NCS_TYPE_FLOAT || nTag == NCS_TYPE_OBJECT) {
                        if (m_sStack.m_pValueData[nTop] != m_sStack.m_pValueData[nTop - nPairs])
                            bEqual = 0;
                    } else if (nTag >= NCS_TYPE_ENGINE_STRUCTURE0 && nTag <= NCS_TYPE_ENGINE_STRUCTURE9) {
                        if (m_pCmdImplementer == 0)
                            return -SCRIPT_ERROR_COMMAND_IMPLEMENTER_NOT_SET;
                        if (m_pCmdImplementer->GetEqualGameDefinedStructure(
                                nTag - NCS_TYPE_ENGINE_STRUCTURE0, m_sStack.m_pValueData[nTop],
                                m_sStack.m_pValueData[nTop - nPairs]) == 0)
                            bEqual = 0;
                    } else
                        return -SCRIPT_ERROR_UNKNOWN_TYPE_ON_RUN_TIME_STACK;
                }
            }
            m_sStack.SetStackPointer(static_cast<int>(m_sStack.m_nStackPointer) - 2 * nPairs);
            nResult = (nOpcode == NCS_OP_EQUAL) ? bEqual : (bEqual == 0);
            StackPushInteger(nResult);
            *pnOffset += nAdvance;
            break;
        }

        // GEQ / GT / LT / LEQ.
        case NCS_OP_GEQ: case NCS_OP_GT: case NCS_OP_LT: case NCS_OP_LEQ: {
            int bTrue = 0;
            if (nType == NCS_TYPE_FLOAT_FLOAT) {
                if (!StackPopFloat(&fValue1))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                if (!StackPopFloat(&fValue2))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                switch (nOpcode) {
                case NCS_OP_GEQ: bTrue = (fValue2 >= fValue1); break;
                case NCS_OP_GT: bTrue = (fValue2 >  fValue1); break;
                case NCS_OP_LT: bTrue = (fValue2 <  fValue1); break;
                default: bTrue = (fValue2 <= fValue1); break;
                }
            } else {
                if (nType != NCS_TYPE_INTEGER_INTEGER)
                    return -SCRIPT_ERROR_INVALID_AUX_CODE;
                if (!StackPopInteger(&nValue1))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                if (!StackPopInteger(&nValue2))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                switch (nOpcode) {
                case NCS_OP_GEQ: bTrue = (nValue2 >= nValue1); break;
                case NCS_OP_GT: bTrue = (nValue2 >  nValue1); break;
                case NCS_OP_LT: bTrue = (nValue2 <  nValue1); break;
                default: bTrue = (nValue2 <= nValue1); break;
                }
            }
            StackPushInteger(bTrue);
            *pnOffset += 2;
            break;
        }

        // Shifts: left, arithmetic right, and truncating unsigned right.
        case NCS_OP_SHLEFTII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue2 << nValue1);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_SHRIGHTII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (nValue2 < 0)
                StackPushInteger(-((-nValue2) >> nValue1));
            else
                StackPushInteger(nValue2 >> nValue1);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_USHRIGHTII: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue2 >> nValue1);
            *pnOffset += 2;
            break;
        }

        // ADD / SUB / MUL / DIV, dispatched on nType.
        case NCS_OP_ADD: case NCS_OP_SUB: case NCS_OP_MUL: case NCS_OP_DIV: {
            if (nType == NCS_TYPE_INTEGER_INTEGER) {
                if (!StackPopInteger(&nValue1))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                if (!StackPopInteger(&nValue2))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                switch (nOpcode) {
                case NCS_OP_ADD: nResult = nValue1 + nValue2; break;
                case NCS_OP_SUB: nResult = nValue2 - nValue1; break;
                case NCS_OP_MUL: nResult = nValue1 * nValue2; break;
                default:
                    if (nValue1 == 0)
                        return -SCRIPT_ERROR_DIVIDE_BY_ZERO;
                    nResult = nValue2 / nValue1;
                    break;
                }
                StackPushInteger(nResult);
                *pnOffset += 2;
                break;
            }

            int bFloat = (nType == NCS_TYPE_FLOAT_FLOAT || nType == NCS_TYPE_INTEGER_FLOAT || nType == NCS_TYPE_FLOAT_INTEGER);
            if (bFloat) {
                if (nType == NCS_TYPE_FLOAT_INTEGER) {
                    if (!StackPopInteger(&nValue1))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                    fValue1 = static_cast<float>(nValue1);
                    if (!StackPopFloat(&fValue2))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                } else if (nType == NCS_TYPE_INTEGER_FLOAT) {
                    if (!StackPopFloat(&fValue1))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                    if (!StackPopInteger(&nValue2))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                    fValue2 = static_cast<float>(nValue2);
                } else {
                    if (!StackPopFloat(&fValue1))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                    if (!StackPopFloat(&fValue2))
                        return -SCRIPT_ERROR_STACK_UNDERFLOW;
                }
                switch (nOpcode) {
                case NCS_OP_ADD: StackPushFloat(fValue1 + fValue2); break;
                case NCS_OP_SUB: StackPushFloat(fValue2 - fValue1); break;
                case NCS_OP_MUL: StackPushFloat(fValue1 * fValue2); break;
                default:
                    if (fValue1 == 0.0f)
                        return -SCRIPT_ERROR_DIVIDE_BY_ZERO;
                    StackPushFloat(fValue2 / fValue1);
                    break;
                }
                *pnOffset += 2;
                break;
            }

            if (nType == NCS_TYPE_STRING_STRING) {
                CExoString sTop;
                CExoString sNext;
                nResult = StackPopString(&sTop);
                if (nResult)
                    nResult = StackPopString(&sNext);
                if (nResult) {
                    if (nOpcode == NCS_OP_ADD)
                        StackPushString(sNext + sTop);
                }
                if (!nResult)
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                *pnOffset += 2;
                break;
            }

            if (nType == NCS_TYPE_VECTOR_VECTOR) {
                Vector vTop;
                Vector vNext;
                int bOk = StackPopVector(&vTop);
                if (bOk)
                    bOk = StackPopVector(&vNext);
                if (!bOk)
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                if (nOpcode == NCS_OP_ADD)
                    StackPushVector(Vector(vNext.x + vTop.x, vNext.y + vTop.y, vNext.z + vTop.z));
                else if (nOpcode == NCS_OP_SUB)
                    StackPushVector(Vector(vNext.x - vTop.x, vNext.y - vTop.y, vNext.z - vTop.z));
                *pnOffset += 2;
                break;
            }

            if (nType == NCS_TYPE_VECTOR_FLOAT || nType == NCS_TYPE_FLOAT_VECTOR) {
                float fScale K2_UNSET;
                Vector vValue;
                int bOk;
                if (nType == NCS_TYPE_FLOAT_VECTOR) {
                    bOk = StackPopVector(&vValue);
                    if (bOk)
                        bOk = StackPopFloat(&fScale);
                } else {
                    bOk = StackPopFloat(&fScale);
                    if (bOk)
                        bOk = StackPopVector(&vValue);
                }
                if (!bOk)
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                if (nOpcode == NCS_OP_MUL)
                    StackPushVector(Vector(vValue.x * fScale, vValue.y * fScale, vValue.z * fScale));
                else if (nOpcode == NCS_OP_DIV) {
                    if (fScale == 0.0f)
                        return -SCRIPT_ERROR_DIVIDE_BY_ZERO;
                    float fInv = 1.0f / fScale;
                    StackPushVector(Vector(vValue.x * fInv, vValue.y * fInv, vValue.z * fInv));
                }
                *pnOffset += 2;
                break;
            }

            return -SCRIPT_ERROR_INVALID_AUX_CODE;
        }

        // MOD.
        case NCS_OP_MOD: {
            if (nType != NCS_TYPE_INTEGER_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (!StackPopInteger(&nValue2))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (nValue1 == 0)
                return -SCRIPT_ERROR_DIVIDE_BY_ZERO;
            nResult = nValue2 % nValue1;
            StackPushInteger(nResult);
            *pnOffset += 2;
            break;
        }

        // NEG.
        case NCS_OP_NEG: {
            if (nType == NCS_TYPE_INTEGER) {
                if (!StackPopInteger(&nValue1))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                StackPushInteger(-nValue1);
            } else if (nType == NCS_TYPE_FLOAT) {
                if (!StackPopFloat(&fValue1))
                    return -SCRIPT_ERROR_STACK_UNDERFLOW;
                StackPushFloat(-fValue1);
            } else
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            *pnOffset += 2;
            break;
        }

        // One's complement.
        case NCS_OP_COMP: {
            if (nType != NCS_TYPE_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(~nValue1);
            *pnOffset += 2;
            break;
        }

        // Unwind the stack down to a code-supplied frame.
        case NCS_OP_MOVSP: {
            int nVal = (static_cast<unsigned char>(pCode[*pnOffset + 2]) << 24) |
                       (static_cast<unsigned char>(pCode[*pnOffset + 3]) << 16) |
                       (static_cast<unsigned char>(pCode[*pnOffset + 4]) << 8) |
                       static_cast<unsigned char>(pCode[*pnOffset + 5]);
            m_sStack.SetStackPointer(static_cast<int>(m_sStack.m_nStackPointer) - (-nVal >> 2));
            *pnOffset += 6;
            break;
        }

        // STORE_STATE: record the situation breadcrumb.
        case NCS_OP_STORE_STATEALL:
            m_nSavedInstructionPtr = *pnOffset + nType;
            m_nStackSizeToSave = 0;
            m_nBaseStackSizeToSave = 0;
            *pnOffset += 2;
            break;

        // JMP.
        case NCS_OP_JMP:
            *pnOffset += (static_cast<unsigned char>(pCode[*pnOffset + 2]) << 24) |
                         (static_cast<unsigned char>(pCode[*pnOffset + 3]) << 16) |
                         (static_cast<unsigned char>(pCode[*pnOffset + 4]) << 8) |
                         static_cast<unsigned char>(pCode[*pnOffset + 5]);
            break;

        // JSR: push the return offset, then jump. Each byte is (unsigned char):
        // sign-extending a byte into the upper lanes would corrupt the target offset.
        case NCS_OP_JSR: {
            int nOffset = *pnOffset;
            int nTarget = (static_cast<unsigned char>(pCode[nOffset + 2]) << 24) |
                          (static_cast<unsigned char>(pCode[nOffset + 3]) << 16) |
                          (static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                          static_cast<unsigned char>(pCode[nOffset + 5]);
            *pnOffset = nOffset + 6;
            if (!PushInstructionPtr(nOffset + 6))
                return -SCRIPT_ERROR_STACK_OVERFLOW;
            *pnOffset = nOffset + nTarget;
            break;
        }

        // JZ.
        case NCS_OP_JZ: {
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (nValue1 == 0)
                *pnOffset += (static_cast<unsigned char>(pCode[*pnOffset + 2]) << 24) |
                             (static_cast<unsigned char>(pCode[*pnOffset + 3]) << 16) |
                             (static_cast<unsigned char>(pCode[*pnOffset + 4]) << 8) |
                             static_cast<unsigned char>(pCode[*pnOffset + 5]);
            else
                *pnOffset += 6;
            break;
        }

        // JNZ.
        case NCS_OP_JNZ: {
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            if (nValue1 != 0)
                *pnOffset += (static_cast<unsigned char>(pCode[*pnOffset + 2]) << 24) |
                             (static_cast<unsigned char>(pCode[*pnOffset + 3]) << 16) |
                             (static_cast<unsigned char>(pCode[*pnOffset + 4]) << 8) |
                             static_cast<unsigned char>(pCode[*pnOffset + 5]);
            else
                *pnOffset += 6;
            break;
        }

        // Return from the script.
        case NCS_OP_RETN: {
            if (!PopInstructionPtr(&nResult))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            *pnOffset = nResult;
            break;
        }

        // Move a frame's worth of cells and drop the frame. Bytes 2..3 and 4..5 are cell
        // counts (>>2); bytes 6..7 are a signed cell delta. When the destination frame is
        // below the current one the stack is first unwound onto it.
        case NCS_OP_DESTRUCT: {
            int nOffset = *pnOffset;
            int nRawB = static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 4]) << 8) |
                                static_cast<unsigned char>(pCode[nOffset + 5]));
            int nA = static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 2]) << 8) |
                             static_cast<unsigned char>(pCode[nOffset + 3])) >> 2;
            int nB = nRawB >> 2;
            int nBase = nB +
                        ((static_cast<short>((static_cast<unsigned char>(pCode[nOffset + 6]) << 8) |
                                  static_cast<unsigned char>(pCode[nOffset + 7]))) >> 2);
            int nSp = static_cast<int>(m_sStack.m_nStackPointer);
            if (nBase < nA) {
                nSp = nSp - nA + nBase;
                m_sStack.SetStackPointer(nSp);
            }

            if (nBase > nB) {
                for (int i = -nBase; nB + i != 0; i++)
                    m_sStack.AssignLocationToLocation(nSp + nB + i, nSp + i);
            }

            if (nRawB > 3)
                m_sStack.SetStackPointer(static_cast<int>(m_sStack.m_nStackPointer) - nB);

            *pnOffset += 8;
            break;
        }

        // NOT.
        case NCS_OP_NOTI: {
            if (nType != NCS_TYPE_INTEGER)
                return -SCRIPT_ERROR_INVALID_AUX_CODE;
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            StackPushInteger(nValue1 == 0);
            *pnOffset += 2;
            break;
        }

        // Push the base pointer, then rebase it on the stack pointer.
        case NCS_OP_SAVEBP: {
            int nBp = static_cast<int>(m_sStack.m_nBasePointer);
            int nSp = static_cast<int>(m_sStack.m_nStackPointer);
            StackPushInteger(nBp);
            m_sStack.m_nBasePointer = static_cast<unsigned long long>(nSp);
            *pnOffset += 2;
            break;
        }
        case NCS_OP_RESTOREBP: {
            if (!StackPopInteger(&nValue1))
                return -SCRIPT_ERROR_STACK_UNDERFLOW;
            m_sStack.m_nBasePointer = static_cast<unsigned long long>(nValue1);
            *pnOffset += 2;
            break;
        }

        // The three-word breadcrumb.
        case NCS_OP_STORE_STATE:
            m_nSavedInstructionPtr = *pnOffset + nType;
            m_nBaseStackSizeToSave = (static_cast<unsigned char>(pCode[*pnOffset + 2]) << 24) |
                        (static_cast<unsigned char>(pCode[*pnOffset + 3]) << 16) |
                        (static_cast<unsigned char>(pCode[*pnOffset + 4]) << 8) |
                        static_cast<unsigned char>(pCode[*pnOffset + 5]);
            m_nStackSizeToSave = (static_cast<unsigned char>(pCode[*pnOffset + 6]) << 24) |
                        (static_cast<unsigned char>(pCode[*pnOffset + 7]) << 16) |
                        (static_cast<unsigned char>(pCode[*pnOffset + 8]) << 8) |
                        static_cast<unsigned char>(pCode[*pnOffset + 9]);
            *pnOffset += 10;
            break;

        default:
            return -SCRIPT_ERROR_INVALID_OP_CODE;
        }
    }
    return 0;
}

// Restores the saved stack pointer on both exits. SetStackPointer only ever lowers the
// pointer, so a script that popped below the saved depth has to be raised back explicitly.
int CVirtualMachineInternal::RunScriptFile(int nArg)
{
    int nSavedInstructionPtr = m_nInstructionPtr;
    m_nReturnValueType = 0;
    m_pReturnValue = 0;
    unsigned long long nSavedStackPointer = m_sStack.m_nStackPointer;
    m_anInstructionPtrs[m_nInstructionPtr] = -1;
    m_nInstructionPtr = m_nInstructionPtr + 1;

    int nArgLocal = nArg;
    int nResult = ExecuteCode(&nArgLocal, m_aScriptSituations[m_nSituationCount].m_sData,
                              m_aScriptSituations[m_nSituationCount].m_nDataSize);
    if (nResult < 0 || nSavedInstructionPtr != m_nInstructionPtr) {
        CExoString sName(m_aScriptSituations[m_nSituationCount].m_sName.CStr());
        m_pCmdImplementer->ReportError(sName, -nResult);
        int nSavedSp = static_cast<int>(nSavedStackPointer);
        if (m_sStack.m_nStackPointer > static_cast<unsigned long long>(nSavedSp))
            m_sStack.SetStackPointer(nSavedSp);
        else
            m_sStack.m_nStackPointer = nSavedSp;
        m_nInstructionPtr = nSavedInstructionPtr;
    }
    return nResult;
}

// The VM's own return-value slot; its type tag reads NCS_TYPE_INTEGER once a script has produced
// a value.
int CVirtualMachineInternal::GetRunScriptReturnValue(int* pnValue, void** ppUnk)
{
    if (m_nReturnValueType == NCS_TYPE_INTEGER) {
        *pnValue = m_nReturnValueType;
        *ppUnk = m_pReturnValue;
        return 1;
    }
    return 0;
}

// The pushed-instruction-offset table behind the call/return handling.
int CVirtualMachineInternal::PushInstructionPtr(int nValue)
{
    m_anInstructionPtrs[m_nInstructionPtr] = nValue;
    int nOld = m_nInstructionPtr;
    m_nInstructionPtr = nOld + 1;
    return nOld < INSTRUCTION_PTR_STACK_SIZE - 1;
}

// In the StackPush* family, AddToTopOfStack reserves the cell and writes its type tag, then the
// caller fills the 8-byte value cell at m_nStackPointer - 1.
int CVirtualMachineInternal::StackPushInteger(int nValue)
{
    m_sStack.AddToTopOfStack(NCS_TYPE_INTEGER);
    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = reinterpret_cast<void*>(static_cast<intptr_t>(nValue));
    return 1;
}

int CVirtualMachineInternal::StackPushFloat(float fValue)
{
    m_sStack.AddToTopOfStack(NCS_TYPE_FLOAT);
    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = reinterpret_cast<void*>(static_cast<intptr_t>(*reinterpret_cast<int*>(&fValue)));
    return 1;
}

int CVirtualMachineInternal::StackPushString(const CExoString& sValue)
{
    CExoString* pCopy = new CExoString(sValue.CStr());
    m_sStack.AddToTopOfStack(NCS_TYPE_STRING);
    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = pCopy;
    return 1;
}

// Pushing x, y, z leaves z on top, so the pop reads z first, then y, then x.
int CVirtualMachineInternal::StackPushVector(Vector vValue)
{
    StackPushFloat(vValue.x);
    StackPushFloat(vValue.y);
    StackPushFloat(vValue.z);
    return 1;
}

int CVirtualMachineInternal::StackPushObject(unsigned long long oid)
{
    m_sStack.AddToTopOfStack(NCS_TYPE_OBJECT);
    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = reinterpret_cast<void*>(static_cast<intptr_t>(static_cast<int>(oid)));
    return 1;
}

int CVirtualMachineInternal::StackPushEngineStructure(int nType, void* pValue)
{
    // nType is the structure's index; its cell tag is NCS_TYPE_ENGINE_STRUCTURE0 + nType, the
    // range AddToTopOfStack registers a DestroyGameDefinedStructure cleanup for.
    if (m_pCmdImplementer == 0)
        return 0;
    void* pCopy = m_pCmdImplementer->CopyGameDefinedStructure(nType, pValue);
    m_sStack.AddToTopOfStack(nType + NCS_TYPE_ENGINE_STRUCTURE0);
    m_sStack.m_pValueData[static_cast<int>(m_sStack.m_nStackPointer) - 1] = pCopy;
    return 1;
}

void CVirtualMachineInternal::SetCommandImplementer(CVirtualMachineCmdImplementer* p)
{
    m_pCmdImplementer = p;
    if (p != 0)
        p->InitializeCommands();
}

// CVirtualMachineStack's own methods, next to the VM that owns one by value. The StackPop* family
// folds the cleanup loop of SetStackPointer into each body; the int-returning accessors truncate
// the stack's 64-bit counters.
int CVirtualMachineStack::GetStackPointer()
{
    return static_cast<int>(m_nStackPointer);
}

int CVirtualMachineStack::GetBasePointer()
{
    return static_cast<int>(m_nBasePointer);
}

void CVirtualMachineStack::SetBasePointer(int nBase)
{
    m_nBasePointer = nBase;
}

void CVirtualMachineStack::InitializeStack()
{
    ClearStack();
}

CVirtualMachineStack::CVirtualMachineStack()
    : m_nStackPointer(0), m_nBasePointer(0), m_nCapacity(0), m_pTypeData(0), m_pValueData(0)
{
}

CVirtualMachineStack::~CVirtualMachineStack()
{
    ClearStack();
}

void CVirtualMachineStack::ClearStack()
{
    if (m_pTypeData == 0) {
        m_nStackPointer = 0;
        m_nBasePointer = 0;
        m_nCapacity = 0;
        if (m_pValueData != 0) {
            delete[] m_pValueData;
            m_pValueData = 0;
        }
        return;
    }
    if (m_pValueData != 0) {
        for (int i = static_cast<int>(m_nStackPointer - 1); i >= 0; i--) {
            if (m_pTypeData[i] == NCS_TYPE_STRING) {
                CExoString* pString = static_cast<CExoString*>(m_pValueData[i]);
                if (pString != 0)
                    delete pString;
            } else if (m_pTypeData[i] >= NCS_TYPE_ENGINE_STRUCTURE0 && m_pTypeData[i] <= NCS_TYPE_ENGINE_STRUCTURE9) {
                m_pOwner->m_pCmdImplementer->DestroyGameDefinedStructure(m_pTypeData[i] - NCS_TYPE_ENGINE_STRUCTURE0,
                                                                         m_pValueData[i]);
            }
            m_pValueData[i] = 0;
        }
    }
    m_nStackPointer = 0;
    m_nBasePointer = 0;
    m_nCapacity = 0;
    if (m_pTypeData != 0) {
        delete[] m_pTypeData;
        m_pTypeData = 0;
    }
    if (m_pValueData != 0) {
        delete[] m_pValueData;
        m_pValueData = 0;
    }
}

// Re-seeds this stack as a slice copy of pFrom. Both cell arrays are reallocated, the base and
// top regions are copied across, and every cell whose payload is owned (a heap CExoString, or
// an engine structure) is deep-copied so the two stacks share no allocation.
//
// The two bounds are the saved situation's own entry counts, scaled down. They arrive as byte
// sizes, so an entry count is `bound >> 2`. When both scale to zero the slice degenerates to
// everything pFrom currently holds.
void CVirtualMachineStack::CopyFromStack(CVirtualMachineStack* pFrom, int nTopBound, int nBaseBound)
{
    ClearStack();
#ifdef K2_FIX_ENGINE_BUGS
    if (m_pTypeData != 0)
        delete[] m_pTypeData;
    if (m_pValueData != 0)
        delete[] m_pValueData;
#else
    if (m_pTypeData != 0)
        delete m_pTypeData;
    if (m_pValueData != 0)
        delete m_pValueData;
#endif
    m_pTypeData = 0;
    m_pValueData = 0;
    m_nStackPointer = 0;
    m_nBasePointer = 0;
    m_nCapacity = 0;

    int nBase;
    int nTop;
    if (((nTopBound >> 2) | (nBaseBound >> 2)) == 0) {
        nBase = static_cast<int>(pFrom->m_nBasePointer);
        nTop = static_cast<int>(pFrom->m_nStackPointer) - nBase;
    } else {
        nTop = nTopBound >> 2;
        nBase = nBaseBound >> 2;
    }
    if (nBase > MAX_STACK_COPY_CELLS)
        nBase = MAX_STACK_COPY_CELLS;
    if (nBase < 0)
        nBase = 0;
    if (static_cast<uint64_t>(nBase) > pFrom->m_nBasePointer)
        nBase = static_cast<int>(pFrom->m_nBasePointer);
    if (nTop > MAX_STACK_COPY_CELLS)
        nTop = MAX_STACK_COPY_CELLS;
    if (nTop < 0)
        nTop = 0;
#ifdef K2_FIX_ENGINE_BUGS
    if (static_cast<uint64_t>(nTop) > pFrom->m_nStackPointer)
        nTop = static_cast<int>(pFrom->m_nStackPointer);
#else
    if (static_cast<uint64_t>(nBase) > pFrom->m_nBasePointer)
        nBase = static_cast<int>(pFrom->m_nBasePointer);
#endif

    int nCapacity = nBase + nTop;
    if (nCapacity == 0)
        return;
    m_pTypeData = new char[nCapacity + STACK_COPY_HEADROOM];
    m_nCapacity = nCapacity + STACK_COPY_HEADROOM;
    m_pValueData = new void*[nCapacity + STACK_COPY_HEADROOM];
    m_nStackPointer = nCapacity;
    m_nBasePointer = nBase;

    memcpy(&m_pTypeData[nBase], &pFrom->m_pTypeData[pFrom->m_nStackPointer - nTop], nTop);  // idiom-exempt: a runtime entry count, not a sizeof
    memcpy(m_pTypeData, &pFrom->m_pTypeData[pFrom->m_nBasePointer - nBase], nBase);  // idiom-exempt: a runtime entry count, not a sizeof

    for (int i = 0; i < nBase; i++) {
        long long nSrc = pFrom->m_nBasePointer - nBase + i;
        void* pValue;
        if (m_pTypeData[i] == NCS_TYPE_STRING) {
            CExoString* pSrc = static_cast<CExoString*>(pFrom->m_pValueData[nSrc]);
            CExoString* pNew = new CExoString();
            *pNew = *pSrc;
            pValue = pNew;
        } else if (m_pTypeData[i] >= NCS_TYPE_ENGINE_STRUCTURE0 && m_pTypeData[i] <= NCS_TYPE_ENGINE_STRUCTURE9) {
            pValue = m_pOwner->m_pCmdImplementer->CopyGameDefinedStructure(
                pFrom->m_pTypeData[nSrc] - NCS_TYPE_ENGINE_STRUCTURE0, pFrom->m_pValueData[nSrc]);
        } else {
            pValue = pFrom->m_pValueData[nSrc];
        }
        m_pValueData[i] = pValue;
    }
    for (int i = 0; i < nTop; i++) {
        int nSrc = static_cast<int>(pFrom->m_nStackPointer - nTop + i);
        if (m_pTypeData[m_nBasePointer + i] == NCS_TYPE_STRING) {
            CExoString* pSrc = static_cast<CExoString*>(pFrom->m_pValueData[nSrc]);
            CExoString* pNew = new CExoString();
            *pNew = *pSrc;
            m_pValueData[m_nBasePointer + i] = pNew;
        } else if (m_pTypeData[m_nBasePointer + i] >= NCS_TYPE_ENGINE_STRUCTURE0
                   && m_pTypeData[m_nBasePointer + i] <= NCS_TYPE_ENGINE_STRUCTURE9) {
            m_pValueData[m_nBasePointer + i] = m_pOwner->m_pCmdImplementer->CopyGameDefinedStructure(
                pFrom->m_pTypeData[nSrc] - NCS_TYPE_ENGINE_STRUCTURE0, pFrom->m_pValueData[nSrc]);
        } else {
            m_pValueData[m_nBasePointer + i] = pFrom->m_pValueData[nSrc];
        }
    }
}

// Frees whatever the cells in [nNew, m_nStackPointer) own and drops the pointer to nNew. This
// is the single cleanup implementation: ClearStack, the pops and the destructors all route
// through it.
void CVirtualMachineStack::SetStackPointer(int nNew)
{
    if (m_nStackPointer <= static_cast<uint64_t>(nNew))
        return;
    for (int i = static_cast<int>(m_nStackPointer - 1); i >= nNew; i--) {
        if (m_pTypeData[i] == NCS_TYPE_STRING) {
            CExoString* pString = static_cast<CExoString*>(m_pValueData[i]);
            if (pString != 0)
                delete pString;
        } else if (m_pTypeData[i] >= NCS_TYPE_ENGINE_STRUCTURE0 && m_pTypeData[i] <= NCS_TYPE_ENGINE_STRUCTURE9) {
            m_pOwner->m_pCmdImplementer->DestroyGameDefinedStructure(m_pTypeData[i] - NCS_TYPE_ENGINE_STRUCTURE0,
                                                                     m_pValueData[i]);
        }
    }
    m_nStackPointer = nNew;
}

// Each pop guards the depth and the cell's type, hands back the payload and trims the cell. The
// value cells are untyped 8-byte slots, so the int pop truncates the whole cell while the float pop
// reads a float out of its low half, unlike StackPushFloat, which sign-extends the bits across it.
int CVirtualMachineInternal::StackPopInteger(int* pnOut)
{
    int nTop = m_sStack.GetStackPointer();
    if (nTop < 1)
        return 0;
    nTop--;
    if (m_sStack.m_pTypeData[nTop] != NCS_TYPE_INTEGER)
        return 0;
    *pnOut = static_cast<int>(reinterpret_cast<intptr_t>(m_sStack.m_pValueData[nTop]));
    m_sStack.SetStackPointer(nTop);
    return 1;
}

int CVirtualMachineInternal::StackPopFloat(float* pfOut)
{
    int nTop = m_sStack.GetStackPointer();
    if (nTop < 1)
        return 0;
    nTop--;
    if (m_sStack.m_pTypeData[nTop] != NCS_TYPE_FLOAT)
        return 0;
    *pfOut = *reinterpret_cast<float*>(&m_sStack.m_pValueData[nTop]);
    m_sStack.SetStackPointer(nTop);
    return 1;
}

int CVirtualMachineInternal::StackPopVector(Vector* pvOut)
{
    float fValue;
    int nResult = StackPopFloat(&fValue);
    if (nResult == 1)
        pvOut->z = fValue;
    if (nResult == 1)
        nResult = StackPopFloat(&fValue);
    if (nResult == 1)
        pvOut->y = fValue;
    if (nResult == 1)
        nResult = StackPopFloat(&fValue);
    if (nResult == 1)
        pvOut->x = fValue;
    return nResult;
}

int CVirtualMachineInternal::StackPopString(CExoString* psOut)
{
    int nTop = m_sStack.GetStackPointer();
    if (nTop < 1)
        return 0;
    nTop--;
    if (m_sStack.m_pTypeData[nTop] != NCS_TYPE_STRING)
        return 0;
    *psOut = *static_cast<CExoString*>(m_sStack.m_pValueData[nTop]);
    m_sStack.SetStackPointer(nTop);
    return 1;
}

int CVirtualMachineInternal::StackPopObject(unsigned long long* poidOut)
{
    int nTop = m_sStack.GetStackPointer();
    if (nTop < 1)
        return 0;
    nTop--;
    if (m_sStack.m_pTypeData[nTop] != NCS_TYPE_OBJECT)
        return 0;
    *poidOut = reinterpret_cast<unsigned long long>(m_sStack.m_pValueData[nTop]);
    m_sStack.SetStackPointer(nTop);
    return 1;
}

int CVirtualMachineInternal::StackPopEngineStructure(int nType, void** ppOut)
{
    int nTop = m_sStack.GetStackPointer();
    if (nTop < 1)
        return 0;
    nTop--;
    if (m_sStack.m_pTypeData[nTop] != nType + NCS_TYPE_ENGINE_STRUCTURE0)
        return 0;
    if (m_pCmdImplementer == 0)
        return 0;
    *ppOut = m_pCmdImplementer->CopyGameDefinedStructure(nType, m_sStack.m_pValueData[nTop]);
    m_sStack.SetStackPointer(nTop);
    return 1;
}

// Pushes a saved copy of the current situation, the newest slot, into a fresh heap
// CVirtualMachineScript. The image copy is guarded on the situation's index, not on the image
// pointer.
int CVirtualMachineInternal::StackPopCommand(CVirtualMachineScript** ppOut)
{
    CVirtualMachineScript* pScript = new CVirtualMachineScript();
    *ppOut = pScript;
    CVirtualMachineScript* pSrc = &m_aScriptSituations[m_nSituationCount];
    int nSituationIndex = pSrc->m_nSituationIndex;
    if (nSituationIndex != 0) {
        pScript->m_nDataSize = pSrc->m_nDataSize;
        pScript->m_sData = new char[pSrc->m_nDataSize];
        memcpy(pScript->m_sData, pSrc->m_sData, pSrc->m_nDataSize);  // idiom-exempt: a runtime byte count, not a sizeof
    } else {
        pScript->m_nDataSize = 0;
        pScript->m_sData = 0;
    }
    pScript->m_nCRC = pSrc->m_nCRC;
    pScript->m_nSituationIndex = nSituationIndex;
    pScript->m_nSecondaryPtr = 0;
    pScript->m_nInstructionPtr = m_nSavedInstructionPtr;
    pScript->m_nStackSize = static_cast<int>(m_sStack.m_nStackPointer);
    (*ppOut)->m_pStack = new CVirtualMachineStack();
    (*ppOut)->m_pStack->m_pOwner = this;
    (*ppOut)->m_pStack->CopyFromStack(&m_sStack, m_nStackSizeToSave, m_nBaseStackSizeToSave);
    pScript->m_sName = m_aScriptSituations[m_nSituationCount].m_sName;
    return 1;
}

// On underflow the counter is re-zeroed, not left at -1, and the out-param is untouched.
int CVirtualMachineInternal::PopInstructionPtr(int* pnOut)
{
    if (m_nInstructionPtr-- <= 0) {
        m_nInstructionPtr = 0;
        return 0;
    }
    *pnOut = m_anInstructionPtrs[m_nInstructionPtr];
    return 1;
}

void CVirtualMachineStack::ModifyIntegerAtLocation(int nLocation, int nValue)
{
    if (nLocation < 0)
        return;
    if (m_pOwner == 0)
        return;
    if (m_pTypeData[nLocation] != NCS_TYPE_INTEGER)
        return;
    m_pValueData[nLocation] = static_cast<char*>(m_pValueData[nLocation]) + nValue;
}

// Reserves one cell, growing both arrays when the stack is full, and seeds the value cell for
// the known tags. The engine-structure tags refuse to push when the stack has no owner, since
// there would be nothing to hand the cleanup to: the type byte is written but the pointer
// stays where it was.
void CVirtualMachineStack::AddToTopOfStack(int nType)
{
    if (m_nStackPointer == static_cast<unsigned long long>(m_nCapacity)) {
        unsigned long long nOldCount = m_nStackPointer;
        long long nNewSize = m_nCapacity + 256;
        m_nCapacity = static_cast<int>(nNewSize);
        void** pNewValues = new void*[nNewSize];
        char* pNewTypes = new char[nNewSize];
        if (nOldCount != 0) {
            memcpy(pNewValues, m_pValueData, nOldCount * sizeof(void*));
            memcpy(pNewTypes, m_pTypeData, nOldCount * sizeof(*pNewTypes));
        }
        delete[] m_pTypeData;
        delete[] m_pValueData;
        m_pTypeData = pNewTypes;
        m_pValueData = pNewValues;
    }
    K2_ASSUME(m_nStackPointer < static_cast<uint64_t>(m_nCapacity));   // the pointer never passes the capacity
    m_pTypeData[m_nStackPointer] = static_cast<char>(nType);
    if (nType == NCS_TYPE_STRING) {
        *reinterpret_cast<int64_t*>(&m_pValueData[m_nStackPointer]) = 0;
    } else if (nType >= NCS_TYPE_ENGINE_STRUCTURE0 && nType <= NCS_TYPE_ENGINE_STRUCTURE9) {
        if (m_pOwner == 0)
            return;
        *reinterpret_cast<int64_t*>(&m_pValueData[m_nStackPointer]) = 0;
    } else if (nType == NCS_TYPE_INTEGER) {
        *reinterpret_cast<int64_t*>(&m_pValueData[m_nStackPointer]) = 0;
    } else if (nType == NCS_TYPE_FLOAT) {
        float fValue = 0.0f;
        m_pValueData[m_nStackPointer] = *reinterpret_cast<void**>(&fValue);
    } else if (nType == NCS_TYPE_OBJECT) {
        *reinterpret_cast<int64_t*>(&m_pValueData[m_nStackPointer]) = OBJECT_INVALID;
    } else {
        return;
    }
    m_nStackPointer++;
}

// Copies one stack cell onto another, deep-copying the tags whose payload is owned: type 5
// gets a fresh CExoString, with the destination's old one freed first, and the
// engine-structure range goes through the implementer's Destroy/Copy pair.
//
// The source cell is the first parameter and the destination the second. Assigning onto the
// current top cell pushes a new one first.
void CVirtualMachineStack::AssignLocationToLocation(int nSrc, int nDst)
{
    if (nSrc < 0 || nDst < 0)
        return;
#ifdef K2_FIX_ENGINE_BUGS
    if (nSrc == nDst)
        return;
#endif
    if (m_pOwner == 0)
        return;
    if (m_nStackPointer == static_cast<unsigned long long>(nDst))
        AddToTopOfStack(m_pTypeData[nSrc]);
    char nType = m_pTypeData[nSrc];
    void* pValue;
    if (static_cast<unsigned char>(nType) == NCS_TYPE_STRING) {
        CExoString* pOld = static_cast<CExoString*>(m_pValueData[nDst]);
        if (pOld != 0)
            delete pOld;
        CExoString* pSrc = static_cast<CExoString*>(m_pValueData[nSrc]);
        K2_ASSUME(pSrc != 0);   // a string cell always owns its CExoString (StackPushString, LoadStack)
        CExoString* pNew = new CExoString();
        *pNew = *pSrc;
        pValue = pNew;
    } else if (static_cast<unsigned char>(nType) >= NCS_TYPE_ENGINE_STRUCTURE0 && static_cast<unsigned char>(nType) <= NCS_TYPE_ENGINE_STRUCTURE9) {
        if (m_pValueData[nDst] != 0)
            m_pOwner->m_pCmdImplementer->DestroyGameDefinedStructure(nType - NCS_TYPE_ENGINE_STRUCTURE0, m_pValueData[nDst]);
        pValue = m_pOwner->m_pCmdImplementer->CopyGameDefinedStructure(m_pTypeData[nSrc] - NCS_TYPE_ENGINE_STRUCTURE0,
                                                                       m_pValueData[nSrc]);
    } else {
        pValue = m_pValueData[nSrc];
    }
    m_pValueData[nDst] = pValue;
    m_pTypeData[nDst] = m_pTypeData[nSrc];
}

// Writes the stack as the GFF "Stack" list, with BasePointer / StackPointer / TotalSize on the
// outer struct, then one element per live cell carrying its type tag plus, for the tags whose
// payload is a scalar, the payload under "Value". Tags outside 3..6 fall through to the
// implementer's save slot.
int CVirtualMachineStack::SaveStack(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldINT(pStruct, static_cast<int>(m_nBasePointer), const_cast<char*>("BasePointer"));
    pGff->WriteFieldINT(pStruct, static_cast<int>(m_nStackPointer), const_cast<char*>("StackPointer"));
    pGff->WriteFieldINT(pStruct, m_nCapacity, const_cast<char*>("TotalSize"));
    if (m_nCapacity < 1)
        return 1;
    CResList list;
    pGff->AddList(&list, pStruct, const_cast<char*>("Stack"));
    for (unsigned long long i = 0, nOffset = 0; m_nStackPointer > i; i++, nOffset += 8) {
        CResStruct elem;
        pGff->AddListElement(&elem, &list, static_cast<unsigned int>(i));
        pGff->WriteFieldCHAR(&elem, m_pTypeData[i], const_cast<char*>("Type"));
        switch (m_pTypeData[i]) {
        case NCS_TYPE_INTEGER:
            pGff->WriteFieldINT(&elem, *reinterpret_cast<int*>(reinterpret_cast<char*>(m_pValueData) + nOffset), const_cast<char*>("Value"));
            break;
        case NCS_TYPE_FLOAT:
            pGff->WriteFieldFLOAT(&elem, *reinterpret_cast<float*>(reinterpret_cast<char*>(m_pValueData) + nOffset), const_cast<char*>("Value"));
            break;
        case NCS_TYPE_STRING:
            pGff->WriteFieldCExoString(&elem, **reinterpret_cast<CExoString**>(reinterpret_cast<char*>(m_pValueData) + nOffset), const_cast<char*>("Value"));
            break;
        case NCS_TYPE_OBJECT:
            pGff->WriteFieldDWORD(&elem, *reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(m_pValueData) + nOffset), const_cast<char*>("Value"));
            break;
        default:
            m_pOwner->m_pCmdImplementer->SaveGameDefinedStructure(m_pTypeData[i] - NCS_TYPE_ENGINE_STRUCTURE0,
                                                                  *reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset), pGff, &elem);
            break;
        }
    }
    return 1;
}

// Reads back what SaveStack writes, with the same tag map and the same implementer split, but the
// engine-structure arm sits at the load slot and takes the cell's address, since the load call
// allocates the cell. Both cell arrays are sized from TotalSize, the capacity, not from the
// entry count.
int CVirtualMachineStack::LoadStack(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct elem;
    int nSuccess;
    m_nBasePointer = pGff->ReadFieldINT(pStruct, const_cast<char*>("BasePointer"), nSuccess, 0);
    m_nStackPointer = pGff->ReadFieldINT(pStruct, const_cast<char*>("StackPointer"), nSuccess, 0);
    int nCapacity = pGff->ReadFieldINT(pStruct, const_cast<char*>("TotalSize"), nSuccess, 0);
    m_nCapacity = nCapacity;
#ifdef K2_FIX_ENGINE_BUGS
    if (nCapacity < 1) {
        m_nStackPointer = 0;
        m_nBasePointer = 0;
        return 1;
    }
    if (m_nStackPointer > static_cast<uint64_t>(nCapacity))
        m_nStackPointer = nCapacity;
    if (m_nBasePointer > m_nStackPointer)
        m_nBasePointer = m_nStackPointer;
#else
    if (nCapacity < 1)
        return 1;
#endif
    m_pTypeData = new char[nCapacity];
    m_pValueData = new void*[nCapacity];
    CResList list;
    pGff->GetList(&list, pStruct, const_cast<char*>("Stack"));
    unsigned long long nOffset = 0;
    for (unsigned long long i = 0; m_nStackPointer > i; i++, nOffset += 8) {
        pGff->GetListElement(&elem, &list, static_cast<unsigned int>(i));
        K2_ASSUME(i < static_cast<uint64_t>(nCapacity));   // the stack pointer is clamped to the capacity above
        m_pTypeData[i] = pGff->ReadFieldCHAR(&elem, const_cast<char*>("Type"), nSuccess, 0);
        switch (m_pTypeData[i]) {
        case NCS_TYPE_INTEGER: {
            int nValue = pGff->ReadFieldINT(&elem, const_cast<char*>("Value"), nSuccess, 0);
            *reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset) = reinterpret_cast<void*>(static_cast<long long>(nValue));
            break;
        }
        case NCS_TYPE_FLOAT: {
            float fValue = pGff->ReadFieldFLOAT(&elem, const_cast<char*>("Value"), nSuccess, 0.0f);
            *reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset) = reinterpret_cast<void*>(static_cast<long long>(*reinterpret_cast<int*>(&fValue)));
            break;
        }
        case NCS_TYPE_STRING: {
            CExoString* pValue = new CExoString;
            *pValue = pGff->ReadFieldCExoString(&elem, const_cast<char*>("Value"), nSuccess, CExoString(""));
            *reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset) = pValue;
            break;
        }
        case NCS_TYPE_OBJECT: {
            int nValue = static_cast<int>(pGff->ReadFieldDWORD(&elem, const_cast<char*>("Value"), nSuccess, 0));
            *reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset) = reinterpret_cast<void*>(static_cast<long long>(nValue));
            break;
        }
        default:
            m_pOwner->m_pCmdImplementer->LoadGameDefinedStructure(m_pTypeData[i] - NCS_TYPE_ENGINE_STRUCTURE0,
                                                                  reinterpret_cast<void**>(reinterpret_cast<char*>(m_pValueData) + nOffset), pGff, &elem);
            break;
        }
    }
    return 1;
}

// Saves one whole script situation, which is the compiled image, the pointer bookkeeping and the
// operand stack, the last as a nested "Stack" struct handed to the Stack pair above.
//
// The CodeSize/Code arm is a three-way fallback. Write the situation's image if it already has
// one; otherwise write a zero CodeSize if the name still compiles; otherwise serialize the
// situation at the current slot, DeleteScript it and step the counter back, since the save
// path consumes the temporary situation it just compiled.
int CVirtualMachineInternal::SaveScriptSituation(CVirtualMachineScript* pSituation, CResGFF* pGff, CResStruct* pStruct)
{
    if (pSituation->m_sData != 0 && pSituation->m_nDataSize != 0) {
        pGff->WriteFieldINT(pStruct, pSituation->m_nDataSize, const_cast<char*>("CodeSize"));
        pGff->WriteFieldVOID(pStruct, pSituation->m_sData, pSituation->m_nDataSize, const_cast<char*>("Code"));
    } else {
        if (ReadScriptFile(&pSituation->m_sName) != 0) {
            pGff->WriteFieldINT(pStruct, 0, const_cast<char*>("CodeSize"));
        } else {
            pGff->WriteFieldINT(pStruct, m_aScriptSituations[m_nSituationCount].m_nDataSize, const_cast<char*>("CodeSize"));
            pGff->WriteFieldVOID(pStruct, m_aScriptSituations[m_nSituationCount].m_sData,
                                 m_aScriptSituations[m_nSituationCount].m_nDataSize, const_cast<char*>("Code"));
            DeleteScript(&m_aScriptSituations[m_nSituationCount]);
            m_nSituationCount--;
        }
    }
    pSituation->m_nCRC = 0;
    pGff->WriteFieldDWORD(pStruct, 0, const_cast<char*>("CRC"));
    pGff->WriteFieldINT(pStruct, pSituation->m_nInstructionPtr, const_cast<char*>("InstructionPtr"));
    pGff->WriteFieldINT(pStruct, pSituation->m_nSecondaryPtr, const_cast<char*>("SecondaryPtr"));
    pGff->WriteFieldCExoString(pStruct, pSituation->m_sName, const_cast<char*>("Name"));
    pGff->WriteFieldINT(pStruct, pSituation->m_nStackSize, const_cast<char*>("StackSize"));
    CResStruct stackStruct;
    pGff->AddStructToStruct(&stackStruct, pStruct, const_cast<char*>("Stack"), 0);
    pSituation->m_pStack->SaveStack(pGff, &stackStruct);
    return 1;
}

// The reader half of SaveScriptSituation, with the same field order. A freshly loaded
// situation is always the newest, so its index is forced to 1.
int CVirtualMachineInternal::LoadScriptSituation(CVirtualMachineScript** ppSituation, CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct stackStruct;
    int nSuccess;
    *ppSituation = new CVirtualMachineScript;
    (*ppSituation)->m_nDataSize = pGff->ReadFieldINT(pStruct, const_cast<char*>("CodeSize"), nSuccess, 0);
    if ((*ppSituation)->m_nDataSize != 0)
        (*ppSituation)->m_sData = new char[(*ppSituation)->m_nDataSize];
    else
        (*ppSituation)->m_sData = 0;
    pGff->ReadFieldVOID(pStruct, (*ppSituation)->m_sData, (*ppSituation)->m_nDataSize, const_cast<char*>("Code"), nSuccess, 0);
    (*ppSituation)->m_nCRC = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("CRC"), nSuccess, 0);
    if (nSuccess == 0)
        (*ppSituation)->m_nCRC = 0;
    (*ppSituation)->m_nSituationIndex = 1;
    (*ppSituation)->m_nInstructionPtr = pGff->ReadFieldINT(pStruct, const_cast<char*>("InstructionPtr"), nSuccess, 0);
    (*ppSituation)->m_nSecondaryPtr = pGff->ReadFieldINT(pStruct, const_cast<char*>("SecondaryPtr"), nSuccess, 0);
    (*ppSituation)->m_sName = pGff->ReadFieldCExoString(pStruct, const_cast<char*>("Name"), nSuccess, CExoString(""));
    (*ppSituation)->m_nStackSize = pGff->ReadFieldINT(pStruct, const_cast<char*>("StackSize"), nSuccess, 0);
    pGff->GetStructFromStruct(&stackStruct, pStruct, const_cast<char*>("Stack"));
    (*ppSituation)->m_pStack = new CVirtualMachineStack;
    (*ppSituation)->m_pStack->m_pOwner = this;
    (*ppSituation)->m_pStack->LoadStack(pGff, &stackStruct);
    return 1;
}

// Loads a compiled .ncs into the next free situation slot. Returns 0 on success, or the negated
// SCRIPT_ERROR_TOO_MANY_LEVELS_OF_RECURSION (too many situations), SCRIPT_ERROR_FILE_NOT_OPENED
// (resource demand failed) or SCRIPT_ERROR_FILE_NOT_COMPILED_SUCCESSFULLY (malformed header). The
// situation counter is incremented up front and put back on every failure path.
//
// The "NCS V" prefix, the '.', the two version digits and the trailing 'B' are checked one byte at
// a time; each digit is range-guarded to '1'..'9', contributing 0 when out of range. The payload
// past the header is what the situation keeps.
int CVirtualMachineInternal::ReadScriptFile(CExoString* pScriptName)
{
    int nOld = m_nSituationCount;
    m_nSituationCount = nOld + 1;
    if (nOld > SCRIPT_SITUATION_COUNT - 2) {
        m_nSituationCount = nOld;
        return -SCRIPT_ERROR_TOO_MANY_LEVELS_OF_RECURSION;
    }
    if (pScriptName != 0) {
        CResRef resRef(*pScriptName);
        SetResRef(resRef, 0);
    } else {
        CResRef resRef("");
        SetResRef(resRef, 0);
    }
    if (m_pRes->Demand() == 0) {
        m_nSituationCount--;
        return -SCRIPT_ERROR_FILE_NOT_OPENED;
    }
    char* pData = static_cast<char*>(m_pRes->GetNCSDataPtr());
    int nSize = m_pRes->GetNCSSize();
#ifdef K2_FIX_ENGINE_BUGS
    m_aScriptSituations[m_nSituationCount].m_sName = pScriptName != 0 ? *pScriptName : CExoString("");
#else
    m_aScriptSituations[m_nSituationCount].m_sName = *pScriptName;
#endif
    if (pData[0] == 'N' && pData[1] == 'C' && pData[2] == 'S' && pData[3] == ' '
        && pData[4] == 'V' && pData[6] == '.') {
        int nVersion = 0;
        if (pData[5] >= '1' && pData[5] <= '9')
            nVersion = (pData[5] - '0') * 10;
        if (pData[7] >= '1' && pData[7] <= '9')
            nVersion += pData[7] - '0';
        if (nVersion == 10 && pData[8] == 'B') {
            InitializeScript(&m_aScriptSituations[m_nSituationCount], pData + NCS_HEADER_SIZE, nSize - NCS_HEADER_SIZE);
            m_pRes->Release();
            SetResRef(CResRef(""), 0);
            return 0;
        }
    }
    m_nSituationCount--;
    m_pRes->Release();
    return -SCRIPT_ERROR_FILE_NOT_COMPILED_SUCCESSFULLY;
}
