#include "k2stdint.h"
#include "twodimarray.h"


#include <stdio.h>
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/crtapi.h"
#if KOTOR1
void ByteSwap(int* p);
void ByteSwap(unsigned short* p);
#endif

// Length of the current line: index of the next CR/LF, or the whole remaining size.
unsigned int C2DA::GetNextLineLength(char* pData, unsigned int nSize) {
    char* p = pData;
    for (unsigned int i = 0; i != nSize; i++, p++) {
        if (*p == '\n' || *p == '\r')
            return i;
    }
    return nSize;
}

int C2DA::SetCExoStringEntry(int nRow, int nColumn, const CExoString& sValue) {
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn >= 0) {
        if (nColumn < m_nColumns) {
            m_papasRows[nRow][nColumn] = sValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetBlankEntry(int nRow, int nColumn) {
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn >= 0) {
        if (nColumn < m_nColumns) {
            m_papasRows[nRow][nColumn] = const_cast<char*>("");
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

// Dual-mode label scan; -1 when absent. Note that CompareNoCase returns nonzero when the strings
// are equal.
int C2DA::GetColumnIndex(const CExoString& sColumn) {
    int nIndex = -1;
    if (m_b2daBinary) {
        for (int i = 0; i < m_nColumns; i++) {
#if KOTOR1
            if (strcasecmp(m_pBinaryLabelData + m_paBinaryColumnLabelOffsets[i], sColumn.CStr()) == 0) {
#else
            if (_stricmp(m_pBinaryLabelData + m_paBinaryColumnLabelOffsets[i], sColumn.CStr()) == 0) {
#endif
                nIndex = i;
                break;
            }
        }
    } else {
        for (int i = 0; i < m_nColumns; i++) {
            if (m_pasColumnLabels[i].CompareNoCase(sColumn)) {
                nIndex = i;
                break;
            }
        }
    }
    return nIndex;
}

int C2DA::GetRowIndex(const CExoString& sRow) {
    int nIndex = -1;
    if (m_b2daBinary) {
        for (int i = 0; i < m_nRows; i++) {
#if KOTOR1
            if (strcasecmp(m_pBinaryLabelData + m_paBinaryRowLabelOffsets[i], sRow.CStr()) == 0) {
#else
            if (_stricmp(m_pBinaryLabelData + m_paBinaryRowLabelOffsets[i], sRow.CStr()) == 0) {
#endif
                nIndex = i;
                break;
            }
        }
    } else {
        for (int i = 0; i < m_nRows; i++) {
            if (m_pasRowLabels[i].CompareNoCase(sRow)) {
                nIndex = i;
                break;
            }
        }
    }
    return nIndex;
}

#if KOTOR1
int C2DA::GetFLOATEntry(int nRow, int nColumn, float* pfValue) {
    if (nRow < 0) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (nColumn < 0) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_nRows <= nRow) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 2) {
                *pfValue = m_pafEntryFLOAT[nIndex];
                return 1;
            }
            *pfValue = 0.0f;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].AsFLOAT();
    return 1;
}
#else
int C2DA::GetFLOATEntry(int nRow, int nColumn, float* pfValue) {
    if (nRow < 0) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (nColumn < 0) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].m_sString
                   ? static_cast<float>(atof(m_papasRows[nRow][nColumn].m_sString)) : 0.0f;
    return 1;
}
#endif


#if KOTOR1
int C2DA::GetCExoStringEntry(int nRow, int nColumn, CExoString* pValue) {
    if (nRow < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (nColumn < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return pValue->GetLength() > 0;
}
#else
int C2DA::GetCExoStringEntry(int nRow, int nColumn, CExoString* pValue) {
    if (nRow < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (nColumn < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return (pValue->m_sString ? static_cast<int>(strlen(pValue->m_sString)) : 0) > 0;
}
#endif

#if KOTOR1
int C2DA::GetINTEntry(int nRow, int nColumn, int* pnValue) {
    if (nRow < 0) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (nColumn < 0) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_nRows <= nRow) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 1) {
                *pnValue = m_panEntryINT[nIndex];
                return 1;
            }
            *pnValue = 0;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if (m_papasRows[nRow][nColumn].GetLength() >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].AsINT();
    }
    return 1;
}
#else
int C2DA::GetINTEntry(int nRow, int nColumn, int* pnValue) {
    if (nRow < 0) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (nColumn < 0) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_nColumns <= nColumn) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if ((m_papasRows[nRow][nColumn].m_sString
             ? static_cast<int>(strlen(m_papasRows[nRow][nColumn].m_sString)) : 0) >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].m_sString
                       ? atoi(m_papasRows[nRow][nColumn].m_sString) : 0;
    }
    return 1;
}
#endif

// Formats with sprintf into a 32-byte stack buffer, then assigns the cell.
int C2DA::SetINTEntry(int nRow, int nColumn, int nValue) {
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn >= 0) {
        if (nColumn < m_nColumns) {
            char szValue[32];
            sprintf(szValue, "%i", nValue);
            m_papasRows[nRow][nColumn] = szValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetFLOATEntry(int nRow, int nColumn, float fValue) {
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn >= 0) {
        if (nColumn < m_nColumns) {
            char szValue[32];
            sprintf(szValue, "%f", fValue);
            m_papasRows[nRow][nColumn] = szValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}


int C2DA::SetCExoStringEntry(int nRow, const CExoString& sColumn, const CExoString& sValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn != -1) {
        m_papasRows[nRow][nColumn] = sValue;
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetCExoStringEntry(const CExoString& sRow, int nColumn, const CExoString& sValue) {
    int nRow = GetRowIndex(sRow);
    int bResult = 0;
    if (nColumn >= 0 && nRow != -1) {
        if (nColumn < m_nColumns) {
            m_papasRows[nRow][nColumn] = sValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetCExoStringEntry(const CExoString& sRow, const CExoString& sColumn,
                             const CExoString& sValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    int bResult = 0;
    if (nRow != -1 && nColumn != -1) {
        m_papasRows[nRow][nColumn] = sValue;
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetBlankEntry(int nRow, const CExoString& sColumn) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn != -1) {
        m_papasRows[nRow][nColumn] = const_cast<char*>("");
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetBlankEntry(const CExoString& sRow, int nColumn) {
    int nRow = GetRowIndex(sRow);
    int bResult = 0;
    if (nColumn >= 0 && nRow != -1) {
        if (nColumn < m_nColumns) {
            m_papasRows[nRow][nColumn] = const_cast<char*>("");
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetBlankEntry(const CExoString& sRow, const CExoString& sColumn) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    int bResult = 0;
    if (nRow != -1 && nColumn != -1) {
        m_papasRows[nRow][nColumn] = const_cast<char*>("");
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetINTEntry(int nRow, const CExoString& sColumn, int nValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn != -1) {
        char szValue[32];
        sprintf(szValue, "%i", nValue);
        m_papasRows[nRow][nColumn] = szValue;
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetINTEntry(const CExoString& sRow, int nColumn, int nValue) {
    int nRow = GetRowIndex(sRow);
    int bResult = 0;
    if (nColumn >= 0 && nRow != -1) {
        if (nColumn < m_nColumns) {
            char szValue[32];
            sprintf(szValue, "%i", nValue);
            m_papasRows[nRow][nColumn] = szValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetINTEntry(const CExoString& sRow, const CExoString& sColumn, int nValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    int bResult = 0;
    if (nRow != -1 && nColumn != -1) {
        char szValue[32];
        sprintf(szValue, "%i", nValue);
        m_papasRows[nRow][nColumn] = szValue;
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetFLOATEntry(int nRow, const CExoString& sColumn, float fValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0)
        return 0;
    int bResult = 0;
    if (nRow < m_nRows && nColumn != -1) {
        char szValue[32];
        sprintf(szValue, "%f", fValue);
        m_papasRows[nRow][nColumn] = szValue;
        bResult = 1;
    }
    return bResult;
}

int C2DA::SetFLOATEntry(const CExoString& sRow, int nColumn, float fValue) {
    int nRow = GetRowIndex(sRow);
    int bResult = 0;
    if (nColumn >= 0 && nRow != -1) {
        if (nColumn < m_nColumns) {
            char szValue[32];
            sprintf(szValue, "%f", fValue);
            m_papasRows[nRow][nColumn] = szValue;
            bResult = 1;
        } else {
            return 0;
        }
    }
    return bResult;
}

int C2DA::SetFLOATEntry(const CExoString& sRow, const CExoString& sColumn, float fValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    int bResult = 0;
    if (nRow != -1 && nColumn != -1) {
        char szValue[32];
        sprintf(szValue, "%f", fValue);
        m_papasRows[nRow][nColumn] = szValue;
        bResult = 1;
    }
    return bResult;
}

#if KOTOR1
int C2DA::GetCExoStringEntry(int nRow, const CExoString& sColumn, CExoString* pValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (nColumn == -1) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return pValue->GetLength() > 0;
}
#else
int C2DA::GetCExoStringEntry(int nRow, const CExoString& sColumn, CExoString* pValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (nColumn == -1) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return (pValue->m_sString ? static_cast<int>(strlen(pValue->m_sString)) : 0) > 0;
}
#endif

#if KOTOR1
int C2DA::GetCExoStringEntry(const CExoString& sRow, int nColumn, CExoString* pValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return pValue->GetLength() > 0;
}
#else
int C2DA::GetCExoStringEntry(const CExoString& sRow, int nColumn, CExoString* pValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return (pValue->m_sString ? static_cast<int>(strlen(pValue->m_sString)) : 0) > 0;
}
#endif

#if KOTOR1
int C2DA::GetCExoStringEntry(const CExoString& sRow, const CExoString& sColumn,
                             CExoString* pValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return pValue->GetLength() > 0;
}
#else
int C2DA::GetCExoStringEntry(const CExoString& sRow, const CExoString& sColumn,
                             CExoString* pValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pValue = m_sDefaultValue;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        return (*pValue = static_cast<char*>(m_pBinaryEntryData + nOffset))[0] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pValue = const_cast<char*>("");
        return 0;
    }
    *pValue = m_papasRows[nRow][nColumn];
    return (pValue->m_sString ? static_cast<int>(strlen(pValue->m_sString)) : 0) > 0;
}
#endif

#if KOTOR1
int C2DA::GetFLOATEntry(int nRow, const CExoString& sColumn, float* pfValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (nColumn == -1) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_nRows <= nRow) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 2) {
                *pfValue = m_pafEntryFLOAT[nIndex];
                return 1;
            }
            *pfValue = 0.0f;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].AsFLOAT();
    return 1;
}
#else
int C2DA::GetFLOATEntry(int nRow, const CExoString& sColumn, float* pfValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (nColumn == -1) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].m_sString
                   ? static_cast<float>(atof(m_papasRows[nRow][nColumn].m_sString)) : 0.0f;
    return 1;
}
#endif

// Unlike the other cores this overload has no empty-cell check and returns 1 after the binary
// scan.
#if KOTOR1
int C2DA::GetFLOATEntry(const CExoString& sRow, int nColumn, float* pfValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 2) {
                *pfValue = m_pafEntryFLOAT[nIndex];
                return 1;
            }
            *pfValue = 0.0f;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return 1;
    }
    *pfValue = m_papasRows[nRow][nColumn].AsFLOAT();
    return 1;
}
#else
int C2DA::GetFLOATEntry(const CExoString& sRow, int nColumn, float* pfValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return 1;
    }
    *pfValue = m_papasRows[nRow][nColumn].m_sString
                   ? static_cast<float>(atof(m_papasRows[nRow][nColumn].m_sString)) : 0.0f;
    return 1;
}
#endif

#if KOTOR1
int C2DA::GetFLOATEntry(const CExoString& sRow, const CExoString& sColumn, float* pfValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pfValue = m_sDefaultValue.AsFLOAT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 2) {
                *pfValue = m_pafEntryFLOAT[nIndex];
                return 1;
            }
            *pfValue = 0.0f;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].AsFLOAT();
    return 1;
}
#else
int C2DA::GetFLOATEntry(const CExoString& sRow, const CExoString& sColumn, float* pfValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pfValue = m_sDefaultValue.m_sString ? static_cast<float>(atof(m_sDefaultValue.m_sString)) : 0.0f;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pfValue = 0.0f;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%f", pfValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pfValue = 0.0f;
        return 0;
    }
    *pfValue = m_papasRows[nRow][nColumn].m_sString
                   ? static_cast<float>(atof(m_papasRows[nRow][nColumn].m_sString)) : 0.0f;
    return 1;
}
#endif

#if KOTOR1
int C2DA::GetINTEntry(int nRow, const CExoString& sColumn, int* pnValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (nColumn == -1) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_nRows <= nRow) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 1) {
                *pnValue = m_panEntryINT[nIndex];
                return 1;
            }
            *pnValue = 0;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if (m_papasRows[nRow][nColumn].GetLength() >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].AsINT();
    }
    return 1;
}
#else
int C2DA::GetINTEntry(int nRow, const CExoString& sColumn, int* pnValue) {
    int nColumn = GetColumnIndex(sColumn);
    if (nRow < 0) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (nColumn == -1) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_nRows <= nRow) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if ((m_papasRows[nRow][nColumn].m_sString
             ? static_cast<int>(strlen(m_papasRows[nRow][nColumn].m_sString)) : 0) >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].m_sString
                       ? atoi(m_papasRows[nRow][nColumn].m_sString) : 0;
    }
    return 1;
}
#endif

#if KOTOR1
int C2DA::GetINTEntry(const CExoString& sRow, int nColumn, int* pnValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 1) {
                *pnValue = m_panEntryINT[nIndex];
                return 1;
            }
            *pnValue = 0;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if (m_papasRows[nRow][nColumn].GetLength() >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].AsINT();
    }
    return 1;
}
#else
int C2DA::GetINTEntry(const CExoString& sRow, int nColumn, int* pnValue) {
    int nRow = GetRowIndex(sRow);
    if (nColumn < 0 || nRow == -1 || m_nColumns <= nColumn) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if ((m_papasRows[nRow][nColumn].m_sString
             ? static_cast<int>(strlen(m_papasRows[nRow][nColumn].m_sString)) : 0) >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].m_sString
                       ? atoi(m_papasRows[nRow][nColumn].m_sString) : 0;
    }
    return 1;
}
#endif

#if KOTOR1
int C2DA::GetINTEntry(const CExoString& sRow, const CExoString& sColumn, int* pnValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pnValue = m_sDefaultValue.AsINT();
        return 0;
    }
    if (m_b2daBinary) {
        if (m_bEntryCacheBuilt) {
            int nIndex = m_nColumns * nRow + nColumn;
            if (m_pabEntryParsed[nIndex] & 1) {
                *pnValue = m_panEntryINT[nIndex];
                return 1;
            }
            *pnValue = 0;
            return 0;
        }
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if (m_papasRows[nRow][nColumn].GetLength() >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].AsINT();
    }
    return 1;
}
#else
int C2DA::GetINTEntry(const CExoString& sRow, const CExoString& sColumn, int* pnValue) {
    int nRow = GetRowIndex(sRow);
    int nColumn = GetColumnIndex(sColumn);
    if (nRow == -1 || nColumn == -1) {
        *pnValue = m_sDefaultValue.m_sString ? atoi(m_sDefaultValue.m_sString) : 0;
        return 0;
    }
    if (m_b2daBinary) {
        unsigned short nOffset = m_paBinaryEntryOffsets[m_nColumns * nRow + nColumn];
        *pnValue = 0;
        static_cast<void>(sscanf(m_pBinaryEntryData + nOffset, "%i", pnValue));
        return m_pBinaryEntryData[nOffset] != '\0';
    }
    if (m_papasRows[nRow][nColumn] == const_cast<char*>("")) {
        *pnValue = 0;
        return 0;
    }
    if ((m_papasRows[nRow][nColumn].m_sString
             ? static_cast<int>(strlen(m_papasRows[nRow][nColumn].m_sString)) : 0) >= 3
        && m_papasRows[nRow][nColumn].CStr()[0] == '0'
        && (m_papasRows[nRow][nColumn].CStr()[1] == 'x'
            || m_papasRows[nRow][nColumn].CStr()[1] == 'X')) {
        static_cast<void>(sscanf(m_papasRows[nRow][nColumn].CStr(), "%x", pnValue));
    } else {
        *pnValue = m_papasRows[nRow][nColumn].m_sString
                       ? atoi(m_papasRows[nRow][nColumn].m_sString) : 0;
    }
    return 1;
}
#endif

// Frees the text-mode string arrays, resets the header fields, releases the resource and frees the
// binary-mode offset tables. The two owned ushort offset tables are allocated with `new ushort[n]`
// but scalar-deleted.
void C2DA::Unload2DArray() {
    if (!m_bLoaded)
        return;
    if (m_papasRows) {
        for (int64_t i = 0; i < m_nRows; i++) {
            if (m_papasRows[i])
                delete[] m_papasRows[i];
        }
        delete[] m_papasRows;
        m_papasRows = 0;
    }
    if (m_pasColumnLabels) {
        delete[] m_pasColumnLabels;
        m_pasColumnLabels = 0;
    }
    if (m_pasRowLabels) {
        delete[] m_pasRowLabels;
        m_pasRowLabels = 0;
    }
    m_nRows = 0;
    m_nColumns = 0;
    m_sDefaultValue = const_cast<char*>("");
    m_pRes->Release();
    m_bLoaded = 0;
    m_b2daBinary = 0;
    m_pBinaryEntryData = 0;
    m_paBinaryEntryOffsets = 0;
#ifdef K2_FIX_ENGINE_BUGS
    delete[] m_paBinaryRowLabelOffsets;
#else
    delete m_paBinaryRowLabelOffsets;
#endif
    m_paBinaryRowLabelOffsets = 0;
#ifdef K2_FIX_ENGINE_BUGS
    delete[] m_paBinaryColumnLabelOffsets;
#else
    delete m_paBinaryColumnLabelOffsets;
#endif
    m_paBinaryColumnLabelOffsets = 0;
    m_pBinaryLabelData = 0;
}

C2DA::~C2DA() {
    Unload2DArray();
#if KOTOR1
    delete[] m_pabEntryParsed;
    delete[] m_panEntryINT;
    delete[] m_pafEntryFLOAT;
#endif
}

C2DA::C2DA() {
#if KOTOR1
    m_bEntryCacheBuilt = 0;
#endif
    m_nRows = 0;
    m_nColumns = 0;
    m_pasColumnLabels = 0;
    m_papasRows = 0;
    m_pasRowLabels = 0;
#if KOTOR1
    m_pabEntryParsed = 0;
    m_panEntryINT = 0;
    m_pafEntryFLOAT = 0;
#endif
    m_sDefaultValue = const_cast<char*>("");
    m_bLoaded = 0;
    m_b2daBinary = 0;
    __builtin_memset(&m_pBinaryEntryData, 0, 4 * sizeof(char*));
    m_paBinaryColumnLabelOffsets = 0;
}

C2DA::C2DA(CResRef resRef, int bRequest) {
#if KOTOR1
    m_bEntryCacheBuilt = 0;
#endif
    m_nRows = 0;
    m_nColumns = 0;
    m_pasColumnLabels = 0;
    m_papasRows = 0;
    m_pasRowLabels = 0;
#if KOTOR1
    m_pabEntryParsed = 0;
    m_panEntryINT = 0;
    m_pafEntryFLOAT = 0;
#endif
    m_sDefaultValue = const_cast<char*>("");
    m_bLoaded = 0;
    m_b2daBinary = 0;
    __builtin_memset(&m_pBinaryEntryData, 0, 4 * sizeof(char*));
    m_paBinaryColumnLabelOffsets = 0;
    SetResRef(resRef, bRequest);
}

// CR/LF runs commit; following tab/space runs are tentative until another
// CR/LF (or end-of-buffer) commits them.
// The characters that separate a 2DA's tokens (tab, line feed, carriage return and space), as bits
// over their codes; the token copy also stops at the terminator.
enum : unsigned long long {
    TOKEN_SEPARATOR_BITS = (1ULL << '\t') | (1ULL << '\n') | (1ULL << '\r') | (1ULL << ' '),
};

int C2DA::SkipNewLines(char** ppData, unsigned int* pnSize) {
    unsigned int nSize = *pnSize;
    char* pData = *ppData;
    unsigned int nCommitted = 0;
    unsigned int nCurrent = 0;
    const unsigned long long kWhitespace = TOKEN_SEPARATOR_BITS;
    while (nCommitted < nSize) {
        unsigned char c = *pData;
        if (c > ' ' || !((1ULL << c) & kWhitespace))
            break;
        if (nCurrent < nSize) {
            while (c == '\r' || c == '\n') {
                if (nCurrent == nSize - 1) {
                    nCommitted = nSize;
                    goto done;
                }
                c = *++pData;
                nCurrent++;
            }
        }
        nCommitted = nCurrent;
        for (; nCurrent < nSize && (*pData == ' ' || *pData == '\t'); pData++)
            nCurrent++;
        if (nCurrent == nSize) {
            nCommitted = nSize;
            break;
        }
    }
done:
    *ppData = *ppData + nCommitted;
    *pnSize = *pnSize - nCommitted;
    return nCommitted != 0;
}

// Skips leading tab/space (counting what it skipped), then copies either a "-quoted token or an
// unquoted token up to the next NUL, tab, CR, LF or space.
__attribute__((always_inline))
int C2DA::GetNextToken(char** ppData, unsigned int* pnSize,
                       char* pToken, unsigned int* pnSkipped) {
    unsigned int nSize = *pnSize;
    if (nSize == 0) {
        *pToken = '\0';
        return 0;
    }
    char* pData = *ppData;
    if (*pData == '\0')
        pData++;

    unsigned int nSkipped = 0;
    if (*pData == ' ' || *pData == '\t') {
        do {
            do {
                nSkipped++;
            } while (pData[nSkipped] == '\t');
        } while (nSkipped < nSize && pData[nSkipped] == ' ');
        pData += nSkipped;
    }
    *pnSkipped = nSkipped;
    *pnSize -= nSkipped;

    char* pOut = pToken;
    unsigned int nCopied = 0;
    char c = *pData;
    if (c == '"') {
        char* pQuote = pData;
        char c2 = pQuote[1];
        if (*pnSize != 0) {
            do {
                if (c2 == '\0' || c2 == '"')
                    break;
                pOut[nCopied] = c2;
                c2 = pQuote[nCopied + 2];
                nCopied++;
            } while (nCopied < *pnSize);
        }
        pData = pQuote + nCopied;
        pOut += nCopied;
        if (c2 == '"') {
            pData += 2;
            nCopied++;
        } else {
            pData++;
        }
    } else if (*pnSize == 0) {
        // nothing to copy; pData stays where it is
    } else {
        while (1) {
            if (static_cast<unsigned char>(c) < ' ' + 1 && ((1ULL << static_cast<unsigned char>(c)) & (TOKEN_SEPARATOR_BITS | 1ULL)))
                break;
            pOut[nCopied] = c;
            nCopied++;
            if (nCopied >= *pnSize)
                break;
            c = pData[nCopied];
        }
        pData += nCopied;
        pOut += nCopied;
    }

    *ppData = pData;
    *pnSize = *pnSize - nCopied;
    *pOut = '\0';
    return nCopied != 0;
}

// Writes the "2DA V2.b" binary format: a deduped string table of every cell value, then the
// magics, the default-value line, the labels, the 16-bit per-cell offsets and the string blob.
// Returns 1 on success and 0 on failure, including when the string table outgrows the 16-bit cell
// offsets.
int C2DA::SaveBinary2da(char* pFileName) {
    int bResult = 0;
    if (m_bLoaded) {
        int nCells = m_nRows * m_nColumns;

        CExoString* paUniqueStrings = new CExoString[nCells];
        int* pnCellIndices = new int[nCells];
        int* pnStringOffsets = new int[nCells];

        int nUniqueCount = 0;
        for (int nRow = 0; nRow < m_nRows; nRow++) {
            for (int nColumn = 0; nColumn < m_nColumns; nColumn++) {
                int nIndex = 0;
                for (; nIndex < nUniqueCount; nIndex++) {
                    if (paUniqueStrings[nIndex] == m_papasRows[nRow][nColumn])
                        break;
                }
                // Cells visited so far bound both the unique count and this cell's index.
                K2_ASSUME(nUniqueCount <= nRow * m_nColumns + nColumn && nRow * m_nColumns + nColumn < nCells);
                if (nIndex == nUniqueCount) {
                    paUniqueStrings[nUniqueCount] = m_papasRows[nRow][nColumn];
                    nUniqueCount++;
                }
                pnCellIndices[nRow * m_nColumns + nColumn] = nIndex;
            }
        }

        int nStringTableSize = 0;
        K2_ASSUME(nUniqueCount <= nCells);
        if (nUniqueCount > 0) {
            for (int i = 0; i < nUniqueCount; i++) {
                pnStringOffsets[i] = nStringTableSize;
                nStringTableSize += static_cast<int>(strlen(paUniqueStrings[i].CStr()) + 1);
            }
        }

        if (nStringTableSize < 65536) {
#if KOTOR1
            FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exoresman/twodimarray.cpp",
                                 "SaveBinary2da", 2040, pFileName, "wb");
#else
            FILE* pFile = fopen(pFileName, "wb");
#endif
            if (pFile) {
                fwrite("2DA ", 4, 1, pFile);
                fwrite("V2.b", 4, 1, pFile);
                fputc('\n', pFile);

                if (m_sDefaultValue[0] != '\0')
                    fprintf(pFile, "DEFAULT: %s\n", m_sDefaultValue.CStr());

                for (int nColumn = 0; nColumn < m_nColumns; nColumn++) {
                    fprintf(pFile, "%s\t", m_pasColumnLabels[nColumn].CStr());
                }
                fputc('\0', pFile);
                fwrite(&m_nRows, 4, 1, pFile);

                for (int nRow = 0; nRow < m_nRows; nRow++) {
                    fprintf(pFile, "%s\t", m_pasRowLabels[nRow].CStr());
                }

                for (int nRow = 0; nRow < m_nRows; nRow++) {
                    for (int nColumn = 0; nColumn < m_nColumns; nColumn++) {
                        K2_ASSUME(nRow * m_nColumns + nColumn < nCells);
                        fwrite(&pnStringOffsets[pnCellIndices[nRow * m_nColumns + nColumn]], 2, 1, pFile);
                    }
                }

                fwrite(&nStringTableSize, 2, 1, pFile);

                for (int i = 0; i < nUniqueCount; i++) {
                    if (paUniqueStrings[i][0] != '\0')
                        fputs(paUniqueStrings[i].CStr(), pFile);
                    fputc('\0', pFile);
                }

                fclose(pFile);
                bResult = 1;
                delete[] paUniqueStrings;
                delete[] pnCellIndices;
                delete[] pnStringOffsets;
            } else {
                delete[] paUniqueStrings;
                delete[] pnCellIndices;
                delete[] pnStringOffsets;
            }
        } else {
            delete[] paUniqueStrings;
            delete[] pnCellIndices;
            delete[] pnStringOffsets;
        }
    }
    return bResult;
}


// Parses a "2DA V2.0" text table or a "2DA V2.b" binary table out of the demanded CRes2DA
// resource. Text mode owns three CExoString arrays and releases the resource after copying;
// binary mode points into the resource data, owns two ushort offset tables and keeps it demanded.
int C2DA::Load2DArray() {
#if KOTOR1
    m_pRes->IsLoaded();
#endif
    Unload2DArray();

    CRes2DA* pRes = m_pRes;
    if (!pRes->Demand())
        return 0;

    char* pData = static_cast<char*>(pRes->Get2DADataPtr());
    unsigned int nSize = pRes->Get2DASize();
    m_b2daBinary = pRes->m_b2daBinary;

    char szToken[256];
    unsigned int nSkipped;

    SkipNewLines(&pData, &nSize);

    unsigned int nLineLength = 0;
    while (nLineLength < nSize && pData[nLineLength] != '\n' && pData[nLineLength] != '\r')
        nLineLength++;

    char* pTokenPos = pData;
    unsigned int nTokenRemaining = nLineLength;
    GetNextToken(&pTokenPos, &nTokenRemaining, szToken, &nSkipped);
    _strupr(szToken);

    if (memcmp(szToken, "DEFAULT:", 9) == 0) {
        GetNextToken(&pTokenPos, &nTokenRemaining, szToken, &nSkipped);
        m_sDefaultValue = szToken;
        pData = pTokenPos;
        nSize -= nLineLength;
    } else if (memcmp(szToken, "DEFAULT", 8) == 0) {
// The default-value line may carry a separating colon, with or without a space after it. pData is
        // not advanced here, so that line is read again as the column-label line; nSize is still
        // shortened by its length.
        GetNextToken(&pTokenPos, &nTokenRemaining, szToken, &nSkipped);
        char* pszValue = szToken;
        if (szToken[0] == ':') {
            pszValue = szToken + 1;
            if (__strlen_chk(pszValue, sizeof(szToken) - 1) == 0) {
                GetNextToken(&pTokenPos, &nTokenRemaining, szToken, &nSkipped);
                pszValue = szToken;
            }
        }
        m_sDefaultValue = pszValue;
        nSize -= nLineLength;
    }

    SkipNewLines(&pData, &nSize);

    nLineLength = 0;
    while (nLineLength < nSize && pData[nLineLength] != '\n' && pData[nLineLength] != '\r')
        nLineLength++;

    char* pCountPos = pData;
    unsigned int nCountRemaining = nLineLength;
    m_nColumns = 0;
    if (GetNextToken(&pCountPos, &nCountRemaining, szToken, &nSkipped)) {
        do {
            m_nColumns++;
        } while (GetNextToken(&pCountPos, &nCountRemaining, szToken, &nSkipped));
    }

    if (m_nColumns <= 0) {
        m_bLoaded = 1;
        Unload2DArray();
        return 0;
    }

    if (!m_b2daBinary) {
        m_pasColumnLabels = new CExoString[m_nColumns];

        int nColumn = 0;
        if (GetNextToken(&pData, &nSize, szToken, &nSkipped)) {
            do {
                m_pasColumnLabels[nColumn] = _strlwr(szToken);
                nColumn++;
            } while (GetNextToken(&pData, &nSize, szToken, &nSkipped));
        }

        SkipNewLines(&pData, &nSize);

// Count rows.
        char* pRowScan = pData;
        unsigned int nRowScanSize = nSize;
        m_nRows = 0;
        if (nRowScanSize != 0) {
            do {
                unsigned int nRowLineLen = 0;
                while (nRowLineLen < nRowScanSize && pRowScan[nRowLineLen] != '\n' && pRowScan[nRowLineLen] != '\r')
                    nRowLineLen++;
                pRowScan += nRowLineLen;
                nRowScanSize -= nRowLineLen;
                SkipNewLines(&pRowScan, &nRowScanSize);
                m_nRows++;
            } while (nRowScanSize != 0);
        }

        if (m_nRows <= 0) {
            m_bLoaded = 1;
            pRes->Release();
            return 1;
        }

        m_pasRowLabels = new CExoString[m_nRows];
        m_papasRows = new CExoString*[m_nRows];
        for (int nRow = 0; nRow < m_nRows; nRow++) {
            m_papasRows[nRow] = new CExoString[m_nColumns];
            if (!m_papasRows[nRow]) {
                m_bLoaded = 1;
                Unload2DArray();
                return 0;
            }
        }

        int nRow = 0;
        while (nSize != 0) {
            unsigned int nRowLineLen = 0;
            while (nRowLineLen < nSize && pData[nRowLineLen] != '\n' && pData[nRowLineLen] != '\r')
                nRowLineLen++;

            char* pLinePos = pData;
            unsigned int nLineRemaining = nSize;
            GetNextToken(&pLinePos, &nLineRemaining, szToken, &nSkipped);
            K2_ASSUME(nRow < m_nRows);   // the row count pass split the same text the same way
            m_pasRowLabels[nRow] = _strlwr(szToken);

            for (nColumn = 0; nColumn < m_nColumns; nColumn++) {
                if (!GetNextToken(&pLinePos, &nLineRemaining, szToken, &nSkipped)) {
                    m_papasRows[nRow][nColumn] = m_sDefaultValue;
                } else if (memcmp(szToken, "****", 5) == 0) {
                    m_papasRows[nRow][nColumn] = const_cast<char*>("");
                } else {
                    m_papasRows[nRow][nColumn] = szToken;
                }
            }

            pData += nRowLineLen;
            nSize -= nRowLineLen;
            SkipNewLines(&pData, &nSize);
            nRow++;
        }

        pRes->Release();
    } else {
        m_pBinaryLabelData = pData;
        m_paBinaryColumnLabelOffsets = new unsigned short[m_nColumns];

        int nOffset = 0;
        for (int i = 0; i < m_nColumns; i++) {
            m_paBinaryColumnLabelOffsets[i] = static_cast<unsigned short>(nOffset);
            while (pData[nOffset] != '\0' && pData[nOffset] != '\t')
                nOffset++;
            pData[nOffset] = '\0';
            nOffset++;
        }

        int nRowCount = *reinterpret_cast<int*>(pData + nOffset + 1);
        m_nRows = nRowCount;
#if KOTOR1
        ByteSwap(&m_nRows);
#endif
        nOffset += 5;

        m_paBinaryRowLabelOffsets = new unsigned short[m_nRows];
        for (int i = 0; i < m_nRows; i++) {
            m_paBinaryRowLabelOffsets[i] = static_cast<unsigned short>(nOffset);
            while (pData[nOffset] != '\0' && pData[nOffset] != '\t')
                nOffset++;
            pData[nOffset] = '\0';
            nOffset++;
        }

        m_paBinaryEntryOffsets = reinterpret_cast<unsigned short*>(pData + nOffset);
#if KOTOR1
        unsigned short* pDataSize = m_paBinaryEntryOffsets + m_nRows * m_nColumns;
        unsigned short nDataSize = *pDataSize++;
        ByteSwap(&nDataSize);
        m_pBinaryEntryData = reinterpret_cast<char*>(pDataSize);

        m_bEntryCacheBuilt = 0;
        int nCells = m_nRows * m_nColumns;
        m_pabEntryParsed = new unsigned char[nCells];
        m_panEntryINT = new int[nCells];
        m_pafEntryFLOAT = new float[nCells];
        for (int nRow = 0; nRow < m_nRows; nRow++) {
            for (int nCol = 0; nCol < m_nColumns; nCol++) {
                int nIndex = nRow * m_nColumns + nCol;
                int bINT = GetINTEntry(nRow, nCol, &m_panEntryINT[nIndex]);
                int bFLOAT = GetFLOATEntry(nRow, nCol, &m_pafEntryFLOAT[nIndex]);
                m_pabEntryParsed[nIndex] = (bINT != 0) | ((bFLOAT != 0) << 1);
            }
        }
        m_bEntryCacheBuilt = 1;
#else
        m_pBinaryEntryData = pData + nOffset + m_nRows * m_nColumns * 2 + 2;
#endif
    }

    m_bLoaded = 1;
    return 1;
}
