#include "k2stdint.h"
#include "resgff.h"
#include <stddef.h>
#include <stdio.h>

#include "port/libcstring.h"
#include "port/crtapi.h"

// The initial capacity of each table InitializeForWriting allocates, and the first step each one
// grows by when full.
enum {
    GFF_STRUCT_GROW = 110,
    GFF_FIELD_GROW = 646,
    GFF_LABEL_GROW = 98,
    GFF_FIELD_DATA_GROW = 1836,
    GFF_INDICES_GROW = 4052,
};

// The on-disk sizes WriteGFFData lays the file out by: the header, a struct or field record, a label.
enum {
    GFF_HEADER_SIZE = 56,
    GFF_RECORD_SIZE = 12,
    GFF_LABEL_SIZE = 16,
};
K2_LAYOUT_ASSERT(sizeof(GFFHeader) == GFF_HEADER_SIZE && sizeof(CResGFFStruct) == GFF_RECORD_SIZE &&
                 sizeof(CResGFFField) == GFF_RECORD_SIZE && sizeof(GFFLabel) == GFF_LABEL_SIZE,
                 "the GFF file records");

CResGFF::CResGFF()
{
#if KOTOR1
    // K1: the header and the struct buffer are cleared first.
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_paFields = 0;
    m_nFieldCapacity = 0;
    m_paLabels = 0;
    m_nLabelCapacity = 0;
    m_pFieldData = 0;
    m_nFieldDataCapacity = 0;
    m_pFieldIndices = 0;
    m_nFieldIndicesCapacity = 0;
    m_nFieldIndicesUsed = 0;
    m_pListIndices = 0;
    m_nListIndicesCapacity = 0;
    m_nListIndicesUsed = 0;
#if !KOTOR1
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_nStructGrow = GFF_STRUCT_GROW;
    m_nFieldGrow = GFF_FIELD_GROW;
    m_nLabelGrow = GFF_LABEL_GROW;
    m_nFieldDataGrow = GFF_FIELD_DATA_GROW;
    m_nFieldIndicesGrow = GFF_INDICES_GROW;
    m_nListIndicesGrow = GFF_INDICES_GROW;
    m_nLabelCacheTerminator = 0;
    m_nValid = 0;
    m_bResourceBacked = 0;
    m_bHoldingResObject = 0;
    *reinterpret_cast<unsigned int*>(m_sFileType) = 0;  // raw-offset: GFF file bytes
}

CResGFF::CResGFF(unsigned short resType, char* sType, const CResRef& resRef)
{
#if KOTOR1
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_paFields = 0;
    m_nFieldCapacity = 0;
    m_paLabels = 0;
    m_nLabelCapacity = 0;
    m_pFieldData = 0;
    m_nFieldDataCapacity = 0;
    m_pFieldIndices = 0;
    m_nFieldIndicesCapacity = 0;
    m_nFieldIndicesUsed = 0;
    m_pListIndices = 0;
    m_nListIndicesCapacity = 0;
    m_nListIndicesUsed = 0;
#if !KOTOR1
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_nStructGrow = GFF_STRUCT_GROW;
    m_nFieldGrow = GFF_FIELD_GROW;
    m_nLabelGrow = GFF_LABEL_GROW;
    m_nFieldDataGrow = GFF_FIELD_DATA_GROW;
    m_nFieldIndicesGrow = GFF_INDICES_GROW;
    m_nListIndicesGrow = GFF_INDICES_GROW;
    m_nLabelCacheTerminator = 0;
    m_nValid = 0;
    m_bResourceBacked = 0;
    m_bHoldingResObject = 0;
    *reinterpret_cast<unsigned int*>(m_sFileType) = 0;  // raw-offset: GFF file bytes
    strncpy(m_sFileType, sType, 4);
    g_pExoResMan->SetResObject(resRef, resType, this);
    Demand();
    if (m_pFileData == 0) {
        g_pExoResMan->ReleaseResObject(this);
    } else {
        m_bHoldingResObject = 1;
    }
}

CResGFF::~CResGFF()
{
    if (m_bResourceBacked == 0) {
        if (m_pListIndices != 0) {
            delete[] m_pListIndices;
        }
        m_pListIndices = 0;
        if (m_pFieldIndices != 0) {
            delete[] m_pFieldIndices;
        }
        m_pFieldIndices = 0;
        if (m_pFieldData != 0) {
            delete[] m_pFieldData;
        }
        m_pFieldData = 0;
        if (m_paLabels != 0) {
            delete[] m_paLabels;
        }
        m_paLabels = 0;
#if KOTOR1
        m_paFields.Delete();
        m_paStructs.Delete();
#else
        if (m_paFields != 0) {
            delete[] m_paFields;
        }
        m_paFields = 0;
        if (m_paStructs != 0) {
            delete[] m_paStructs;
        }
        m_paStructs = 0;
#endif
        if (m_pHeader != 0) {
            operator delete(m_pHeader);
        }
        m_pHeader = 0;
    } else {
        if (m_bHoldingResObject != 0) {
            Release();
            g_pExoResMan->ReleaseResObject(this);
            m_bHoldingResObject = 0;
        }
        m_bResourceBacked = 0;
#if KOTOR1
        m_pHeader = 0;
        m_paStructs = 0;
        m_nStructCapacity = 0;
#endif
        m_paFields = 0;
        m_nFieldCapacity = 0;
        m_paLabels = 0;
        m_nLabelCapacity = 0;
        m_pFieldData = 0;
        m_nFieldDataCapacity = 0;
#if !KOTOR1
        m_pHeader = 0;
        m_paStructs = 0;
        m_nStructCapacity = 0;
#endif
        m_pFieldIndices = 0;
        m_nFieldIndicesCapacity = 0;
        m_nFieldIndicesUsed = 0;
        m_pListIndices = 0;
        m_nListIndicesCapacity = 0;
        m_nListIndicesUsed = 0;
    }
}

void CResGFF::ReleaseResource()
{
    if (m_bResourceBacked != 0) {
        if (m_bHoldingResObject != 0) {
            Release();
            g_pExoResMan->ReleaseResObject(this);
            m_bHoldingResObject = 0;
        }
        m_bResourceBacked = 0;
    }
#if KOTOR1
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_paFields = 0;
    m_nFieldCapacity = 0;
    m_paLabels = 0;
    m_nLabelCapacity = 0;
    m_pFieldData = 0;
    m_nFieldDataCapacity = 0;
#if !KOTOR1
    m_pHeader = 0;
    m_paStructs = 0;
    m_nStructCapacity = 0;
#endif
    m_pFieldIndices = 0;
    m_nFieldIndicesCapacity = 0;
    m_nFieldIndicesUsed = 0;
    m_pListIndices = 0;
    m_nListIndicesCapacity = 0;
    m_nListIndicesUsed = 0;
}

int CResGFF::OnResourceFreed()
{
    if (m_bResourceBacked != 0) {
        m_pListIndices = 0;
        m_pFieldIndices = 0;
        m_pFieldData = 0;
        m_paLabels = 0;
        m_paFields = 0;
#if KOTOR1
        m_pHeader = 0;
        m_paStructs = 0;
        m_bResourceBacked = 0;
#else
        m_bResourceBacked = 0;
        m_pHeader = 0;
        m_paStructs = 0;
#endif
    }
    m_nValid = 0;
    return 1;
}

const char* CResGFF::VERSION = "V3.2";

int CResGFF::OnResourceServiced()
{
    if (m_pFileData == 0) {
        return 0;
    }
    if (m_pHeader != 0 && !m_bResourceBacked) {
        delete[] m_pListIndices;
        m_pListIndices = 0;
        delete[] m_pFieldIndices;
        m_pFieldIndices = 0;
        delete[] m_pFieldData;
        m_pFieldData = 0;
        delete[] m_paLabels;
        m_paLabels = 0;
#if KOTOR1
        m_paFields.Delete();
        m_paStructs.Delete();
#else
        delete[] m_paFields;
        m_paFields = 0;
        delete[] m_paStructs;
        m_paStructs = 0;
#endif
        delete m_pHeader;
        m_pHeader = 0;
    }
    m_pHeader = static_cast<GFFHeader*>(m_pFileData);

    unsigned int tmp = *reinterpret_cast<unsigned int*>(m_pHeader->sFileType);  // raw-offset: GFF file bytes
    ::ByteSwap(&tmp);
    if (tmp != *reinterpret_cast<unsigned int*>(m_sFileType)) {  // raw-offset: GFF file bytes
        m_pHeader = 0;
        return 0;
    }
    tmp = *reinterpret_cast<unsigned int*>(m_pHeader->sFileVersion);  // raw-offset: GFF file bytes
    ::ByteSwap(&tmp);
    if (tmp != *reinterpret_cast<const unsigned int*>(VERSION)) {
        m_pHeader = 0;
        return 0;
    }

    if (m_pHeader->nStructCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nStructOffset;
        ::ByteSwap(&tmp);
        m_paStructs = reinterpret_cast<CResGFFStruct*>(pBase + tmp);
    }
    if (m_pHeader->nFieldCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nFieldOffset;
        ::ByteSwap(&tmp);
        m_paFields = reinterpret_cast<CResGFFField*>(pBase + tmp);
    }
    if (m_pHeader->nLabelCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nLabelOffset;
        ::ByteSwap(&tmp);
        m_paLabels = reinterpret_cast<GFFLabel*>(pBase + tmp);
    }
    if (m_pHeader->nFieldDataCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nFieldDataOffset;
        ::ByteSwap(&tmp);
        m_pFieldData = pBase + tmp;
    }
    if (m_pHeader->nFieldIndicesCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nFieldIndicesOffset;
        ::ByteSwap(&tmp);
        m_pFieldIndices = pBase + tmp;
    }
    if (m_pHeader->nListIndicesCount != 0) {
        unsigned char* pBase = static_cast<unsigned char*>(m_pFileData);
        tmp = m_pHeader->nListIndicesOffset;
        ::ByteSwap(&tmp);
        m_pListIndices = pBase + tmp;
    }

    m_nValid = 1;
    m_bResourceBacked = 1;
    return 1;
}

unsigned int CResGFF::ByteSwap(unsigned int n)
{
    ::ByteSwap(&n);
    return n;
}

unsigned long long CResGFF::ByteSwap64(unsigned long long n)
{
    ::ByteSwap(&n);
    return n;
}

int CResGFF::GetTopLevelStruct(CResStruct* pStruct)
{
    if (pStruct != 0) {
        pStruct->nStructIndex = 0;
        return 1;
    }
    return 0;
}

static const int g_anIsDataInPlace[10] = {0, 0, 1, 0, 0, 0, 0, 0, 1, 0};

int CResGFF::IsDataInPlace(unsigned int nType)
{
    if (nType - 6 < 10) {
        return g_anIsDataInPlace[static_cast<int>(nType - 6)];
    }
    return 1;
}

unsigned int CResGFF::GetFieldCount(CResGFFStruct* pStruct)
{
    if (pStruct == 0) {
        return INVALID_INDEX;
    }
    unsigned int n = pStruct->nFieldCount;
    ::ByteSwap(&n);
    return n;
}

#if KOTOR1
CResGFFAlignedPtr<CResGFFStruct> CResGFF::GetStruct(CResStruct* pStruct)
{
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int idx = pStruct->nStructIndex;
        unsigned int n = m_pHeader->nStructCount;
        ::ByteSwap(&n);
        if (idx < n) {
            return m_paStructs + pStruct->nStructIndex;
        }
    }
    return CResGFFAlignedPtr<CResGFFStruct>();
}
#else
CResGFFStruct* CResGFF::GetStruct(CResStruct* pStruct)
{
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int idx = pStruct->nStructIndex;
        unsigned int n = m_pHeader->nStructCount;
        ::ByteSwap(&n);
        if (idx < n) {
            return &m_paStructs[pStruct->nStructIndex];
        }
    }
    return 0;
}
#endif

unsigned int CResGFF::GetElementType(CResStruct* pStruct)
{
    unsigned int tmp;
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int idx = pStruct->nStructIndex;
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        if (idx < tmp) {
            CResGFFStruct* p = &m_paStructs[pStruct->nStructIndex];
            if (p != 0) {
                tmp = p->nType;
                ::ByteSwap(&tmp);
                return tmp;
            }
        }
    }
    return INVALID_INDEX;
}

unsigned int CResGFF::GetFieldCount(CResStruct* pStruct)
{
    unsigned int tmp;
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int idx = pStruct->nStructIndex;
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        if (idx < tmp) {
            if (&m_paStructs[pStruct->nStructIndex] != 0) {
                tmp = m_paStructs[pStruct->nStructIndex].nFieldCount;
                ::ByteSwap(&tmp);
                return tmp;
            }
        }
    }
    return INVALID_INDEX;
}

#if KOTOR1
CResGFFAlignedPtr<CResGFFField> CResGFF::GetField(CResGFFStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
    if (pStruct != 0 && m_paFields != 0) {
        tmp = pStruct->nFieldCount;
        ::ByteSwap(&tmp);
        if (nIndex < tmp) {
            tmp = pStruct->nFieldCount;
            ::ByteSwap(&tmp);
            unsigned int nFC = tmp;
            tmp = pStruct->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            unsigned int nData = tmp;
            if (nFC == 1) {
                tmp = m_pHeader->nFieldCount;
                ::ByteSwap(&tmp);
                if (nData < tmp) {
                    tmp = pStruct->nDataOrDataOffset;
                    ::ByteSwap(&tmp);
                    return CResGFFAlignedPtr<CResGFFField>(&m_paFields.m_p[tmp]);
                }
            } else if (m_pFieldIndices != 0) {
                unsigned int byteOff = nData + nIndex * 4;
                unsigned int nEnd = byteOff + 4;
                tmp = m_pHeader->nFieldIndicesCount;
                ::ByteSwap(&tmp);
                if (nEnd <= tmp) {
                    tmp = *reinterpret_cast<unsigned int*>(m_pFieldIndices + byteOff);
                    ::ByteSwap(&tmp);
                    return CResGFFAlignedPtr<CResGFFField>(&m_paFields.m_p[tmp]);
                }
            }
        }
    }
    return CResGFFAlignedPtr<CResGFFField>();
}
#else
CResGFFField* CResGFF::GetField(CResGFFStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
    if (pStruct != 0 && m_paFields != 0) {
        tmp = pStruct->nFieldCount;
        ::ByteSwap(&tmp);
        if (nIndex < tmp) {
            tmp = pStruct->nFieldCount;
            ::ByteSwap(&tmp);
            unsigned int nFC = tmp;
            tmp = pStruct->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            unsigned int nData = tmp;
            if (nFC == 1) {
                tmp = m_pHeader->nFieldCount;
                ::ByteSwap(&tmp);
                if (nData < tmp) {
                    CResGFFField* paFields = m_paFields;
                    tmp = pStruct->nDataOrDataOffset;
                    ::ByteSwap(&tmp);
                    return &paFields[tmp];
                }
            } else if (m_pFieldIndices != 0) {
                unsigned int byteOff = nData + nIndex * 4;
                unsigned int nEnd = byteOff + 4;
                tmp = m_pHeader->nFieldIndicesCount;
                ::ByteSwap(&tmp);
                if (nEnd <= tmp) {
                    CResGFFField* paFields = m_paFields;
                    tmp = *reinterpret_cast<unsigned int*>(m_pFieldIndices + byteOff);
                    ::ByteSwap(&tmp);
                    return &paFields[tmp];
                }
            }
        }
    }
    return 0;
}
#endif

#if KOTOR1
CResGFFAlignedPtr<CResGFFField> CResGFF::GetField(CResStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int nStructIndex = pStruct->nStructIndex;
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        if (nStructIndex < tmp) {
            CResGFFStruct* pRec = &m_paStructs.m_p[pStruct->nStructIndex];
            if (pRec != 0) {
                CResGFFStruct rec = CResGFFAlignedPtr<CResGFFStruct>(pRec).Get();
                if (m_paFields != 0) {
                    tmp = rec.nFieldCount;
                    ::ByteSwap(&tmp);
                    if (nIndex < tmp) {
                        tmp = rec.nFieldCount;
                        ::ByteSwap(&tmp);
                        if (tmp == 1) {
                            tmp = rec.nDataOrDataOffset;
                            ::ByteSwap(&tmp);
                            unsigned int dataIdx = tmp;
                            tmp = m_pHeader->nFieldCount;
                            ::ByteSwap(&tmp);
                            if (dataIdx < tmp) {
                                tmp = rec.nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                return CResGFFAlignedPtr<CResGFFField>(&m_paFields.m_p[tmp]);
                            }
                        } else if (m_pFieldIndices != 0) {
                            tmp = rec.nDataOrDataOffset;
                            ::ByteSwap(&tmp);
                            unsigned int byteOff = nIndex * 4 + tmp;
                            unsigned int nEnd = byteOff + 4;
                            tmp = m_pHeader->nFieldIndicesCount;
                            ::ByteSwap(&tmp);
                            if (nEnd <= tmp) {
                                unsigned char* paFieldIndices = m_pFieldIndices;
                                tmp = rec.nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                tmp = *reinterpret_cast<unsigned int*>(paFieldIndices + tmp + nIndex * 4);
                                ::ByteSwap(&tmp);
                                return CResGFFAlignedPtr<CResGFFField>(&m_paFields.m_p[tmp]);
                            }
                        }
                    }
                }
            }
        }
    }
    return CResGFFAlignedPtr<CResGFFField>();
}
#else
CResGFFField* CResGFF::GetField(CResStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
    if (pStruct != 0 && m_pHeader != 0 && m_paStructs != 0) {
        unsigned int nStructIndex = pStruct->nStructIndex;
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        if (nStructIndex < tmp) {
            CResGFFStruct* pRec = &m_paStructs[pStruct->nStructIndex];
            if (pRec != 0 && m_paFields != 0) {
                tmp = pRec->nFieldCount;
                ::ByteSwap(&tmp);
                if (nIndex < tmp) {
                    tmp = pRec->nFieldCount;
                    ::ByteSwap(&tmp);
                    if (tmp == 1) {
                        tmp = pRec->nDataOrDataOffset;
                        ::ByteSwap(&tmp);
                        unsigned int dataIdx = tmp;
                        tmp = m_pHeader->nFieldCount;
                        ::ByteSwap(&tmp);
                        if (dataIdx < tmp) {
                            CResGFFField* paFields = m_paFields;
                            tmp = pRec->nDataOrDataOffset;
                            ::ByteSwap(&tmp);
                            return &paFields[tmp];
                        }
                    } else if (m_pFieldIndices != 0) {
                        tmp = pRec->nDataOrDataOffset;
                        ::ByteSwap(&tmp);
                        unsigned int byteOff = nIndex * 4 + tmp;
                        unsigned int nEnd = byteOff + 4;
                        tmp = m_pHeader->nFieldIndicesCount;
                        ::ByteSwap(&tmp);
                        if (nEnd <= tmp) {
                            CResGFFField* paFields = m_paFields;
                            unsigned char* paFieldIndices = m_pFieldIndices;
                            tmp = pRec->nDataOrDataOffset;
                            ::ByteSwap(&tmp);
                            tmp = *reinterpret_cast<unsigned int*>(paFieldIndices + tmp + nIndex * 4);
                            ::ByteSwap(&tmp);
                            return &paFields[tmp];
                        }
                    }
                }
            }
        }
    }
    return 0;
}
#endif

#if KOTOR1
// K1: the struct record is copied out and the labels are compared with strncmp(..., 16).
unsigned int CResGFF::GetFieldByLabel(CResStruct* pStruct, char* sLabel)
{
    if (sLabel == 0 || m_paLabels == 0) {
        return INVALID_INDEX;
    }
    GFFLabel sLabelBuf = {};
    strncpy(sLabelBuf.s, sLabel, 16);

    unsigned int tmp;
    CResGFFStruct rec;
    CResGFFStruct* pRec;
    unsigned int nFieldCount;
    if (pStruct == 0 || m_pHeader == 0 || m_paStructs == 0) {
        pRec = 0;
        nFieldCount = INVALID_INDEX;
    } else {
        unsigned int nStructIndex = pStruct->nStructIndex;
        unsigned int nCount = m_pHeader->nStructCount;
        ::ByteSwap(&nCount);
        if (nStructIndex >= nCount) {
            pRec = 0;
            nFieldCount = INVALID_INDEX;
        } else {
            CResGFFAlignedPtr<CResGFFStruct> pRecRef = m_paStructs + pStruct->nStructIndex;
            if (pRecRef.m_p == 0) {
                pRec = 0;
                nFieldCount = INVALID_INDEX;
            } else {
                rec = *pRecRef.m_p;
                pRec = &rec;
                nCount = rec.nFieldCount;
                ::ByteSwap(&nCount);
                nFieldCount = nCount;
                if (nCount == 0) {
                    return INVALID_INDEX;
                }
            }
        }
    }

    unsigned int i = 0;
    do {
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pRec, i);
        if (pFieldRef.m_p == 0) {
            break;
        }
        CResGFFField field = pFieldRef.Get();
        tmp = field.nLabelIndex;
        ::ByteSwap(&tmp);
        unsigned int nLabelIdx = tmp;
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        if (nLabelIdx >= tmp) {
            break;
        }
        GFFLabel* paLabels = m_paLabels;
        tmp = field.nLabelIndex;
        ::ByteSwap(&tmp);
        if (strncmp(paLabels[tmp].s, sLabelBuf.s, 16) == 0) {
            return i;
        }
        i = i + 1;
    } while (i != nFieldCount);
    return INVALID_INDEX;
}
#else
unsigned int CResGFF::GetFieldByLabel(CResStruct* pStruct, char* sLabel)
{
    unsigned int tmp;
    if (sLabel == 0 || m_paLabels == 0) {
        return INVALID_INDEX;
    }
    int sLabelBuf[4] = {0, 0, 0, 0};
    strncpy(reinterpret_cast<char*>(sLabelBuf), sLabel, 16);

    CResGFFStruct* pRec;
    unsigned int nFieldCount;
    if (pStruct == 0 || m_pHeader == 0 || m_paStructs == 0) {
        pRec = 0;
        nFieldCount = INVALID_INDEX;
    } else {
        unsigned int nStructIndex = pStruct->nStructIndex;
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        if (nStructIndex >= tmp) {
            pRec = 0;
            nFieldCount = INVALID_INDEX;
        } else {
            pRec = &m_paStructs[pStruct->nStructIndex];
            if (pRec == 0) {
                pRec = 0;
                nFieldCount = INVALID_INDEX;
            } else {
                tmp = pRec->nFieldCount;
                ::ByteSwap(&tmp);
                nFieldCount = tmp;
                if (tmp == 0) {
                    return INVALID_INDEX;
                }
            }
        }
    }

    unsigned int i = 0;
    for (;;) {
        CResGFFField* pField = GetField(pRec, i);
        if (pField == 0) {
            break;
        }
        tmp = pField->nLabelIndex;
        ::ByteSwap(&tmp);
        unsigned int nLabelIdx = tmp;
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        if (nLabelIdx >= tmp) {
            break;
        }
        GFFLabel* paLabels = m_paLabels;
        tmp = pField->nLabelIndex;
        ::ByteSwap(&tmp);
        int* pLabel = reinterpret_cast<int*>(&paLabels[tmp]);
        if (sLabelBuf[0] == pLabel[0] && sLabelBuf[1] == pLabel[1] &&  // raw-offset: GFF file bytes
            sLabelBuf[2] == pLabel[2] && sLabelBuf[3] == pLabel[3]) {  // raw-offset: GFF file bytes
            return i;
        }
        i = i + 1;
        if (i == nFieldCount) {
            break;
        }
    }
    return INVALID_INDEX;
}
#endif

GFFLabel* CResGFF::GetLabel(CResStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
#if KOTOR1
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nIndex);
    if (pFieldRef.m_p != 0 && m_paLabels != 0) {
        CResGFFField field = pFieldRef.Get();
        CResGFFField* pField = &field;
#else
    CResGFFField* pField = GetField(pStruct, nIndex);
    if (pField != 0 && m_paLabels != 0) {
#endif
        tmp = pField->nLabelIndex;
        ::ByteSwap(&tmp);
        unsigned int nLabelIdx = tmp;
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        if (nLabelIdx < tmp) {
            GFFLabel* pLabels = m_paLabels;
            tmp = pField->nLabelIndex;
            ::ByteSwap(&tmp);
            return &pLabels[tmp];
        }
    }
    return 0;
}

GFFLabel* CResGFF::GetFieldLabel(CResStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
#if KOTOR1
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nIndex);
    if (pFieldRef.m_p != 0 && m_paLabels != 0) {
        CResGFFField field = pFieldRef.Get();
        CResGFFField* pField = &field;
#else
    CResGFFField* pField = GetField(pStruct, nIndex);
    if (pField != 0 && m_paLabels != 0) {
#endif
        tmp = pField->nLabelIndex;
        ::ByteSwap(&tmp);
        unsigned int nLabelIdx = tmp;
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        if (nLabelIdx < tmp) {
            GFFLabel* pLabels = m_paLabels;
            tmp = pField->nLabelIndex;
            ::ByteSwap(&tmp);
            return &pLabels[tmp];
        }
    }
    return 0;
}

int CResGFF::GetStructFromStruct(CResStruct* pOut, CResStruct* pStruct, char* sLabel)
{
    unsigned int tmp;
    if (pOut != 0 && pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
            tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == FIELD_STRUCT) {
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                pOut->nStructIndex = tmp;
                return 1;
            }
        }
    }
    return 0;
}

unsigned int CResGFF::GetListCount(CResList* pList)
{
    if (pList != 0) {
        unsigned int nField = GetFieldByLabel(&pList->m_struct, pList->m_label);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(&pList->m_struct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(&pList->m_struct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == FIELD_LIST) {
                unsigned int nSize;
                unsigned int* pList2 = static_cast<unsigned int*>(GetDataLayoutList(pField, nSize));
                if (pList2 == 0)
                    return 0;
                if (nSize < 4)
                    return 0;
                tmp = *pList2;
                ::ByteSwap(&tmp);
                return tmp;
            }
        }
    }
    return 0;
}

int CResGFF::GetListElement(CResStruct* pOut, CResList* pList, unsigned int nIndex)
{
    unsigned int tmp;
    if (pOut != 0 && pList != 0) {
        unsigned int nField = GetFieldByLabel(&pList->m_struct, pList->m_label);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(&pList->m_struct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(&pList->m_struct, nField);
        if (pField != 0) {
#endif
            tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == FIELD_LIST) {
                unsigned int nSize;
                unsigned int* pElems = static_cast<unsigned int*>(GetDataLayoutList(pField, nSize));
                if (pElems != 0 && static_cast<uint64_t>(nIndex + 2) * 4 <= static_cast<uint64_t>(nSize)) {
                    tmp = pElems[0];
                    ::ByteSwap(&tmp);
                    unsigned int nCount = tmp;
                    tmp = pElems[nIndex + 1];
                    ::ByteSwap(&tmp);
                    if (nIndex < nCount) {
                        unsigned int nStructIndex = tmp;
                        tmp = m_pHeader->nStructCount;
                        ::ByteSwap(&tmp);
                        if (nStructIndex < tmp) {
                            pOut->nStructIndex = nStructIndex;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int CResGFF::GetList(CResList* pList, CResStruct* pStruct, char* sLabel)
{
    if (pList != 0 && pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == FIELD_LIST) {
                void* pData = GetDataLayoutList(pField, tmp);
                if (pData == 0)
                    return 0;
                if (tmp < 4)
                    return 0;
                pList->m_struct = *pStruct;
                strncpy(pList->m_label, sLabel, 16);
                return 1;
            }
        }
    }
    return 0;
}

unsigned int CResGFF::GetFieldType(CResStruct* pStruct, char* sLabel, unsigned int nIndex)
{
    if (sLabel != 0) {
        nIndex = GetFieldByLabel(pStruct, sLabel);
    }
#if KOTOR1
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nIndex);
    if (pFieldRef.m_p == 0) {
        return INVALID_INDEX;
    }
    CResGFFField field = pFieldRef.Get();
    CResGFFField* pField = &field;
#else
    CResGFFField* pField = GetField(pStruct, nIndex);
    if (pField == 0) {
        return INVALID_INDEX;
    }
#endif
    unsigned int t = pField->nType;
    ::ByteSwap(&t);
    return t;
}

unsigned int CResGFF::GetFieldSize(CResStruct* pStruct, char* sLabel, unsigned int nIndex)
{
    if (sLabel != 0) {
        nIndex = GetFieldByLabel(pStruct, sLabel);
    }
#if KOTOR1
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nIndex);
    if (pFieldRef.m_p == 0) {
        return 0;
    }
    unsigned int nType;
    CResGFFField field = pFieldRef.Get();
    CResGFFField* pField = &field;
#else
    CResGFFField* pField = GetField(pStruct, nIndex);
    if (pField == 0) {
        return 0;
    }
#endif
#if KOTOR1
    nType = pField->nType;
#else
    unsigned int nType = pField->nType;
#endif
    ::ByteSwap(&nType);
    unsigned int nSize;
    switch (nType) {
    case FIELD_BYTE:
    case FIELD_CHAR:
        return 1;
    case FIELD_WORD:
    case FIELD_SHORT:
        return 2;
    case FIELD_DWORD:
    case FIELD_INT:
    case FIELD_FLOAT:
        return 4;
    case FIELD_DWORD64:
    case FIELD_INT64:
    case FIELD_DOUBLE:
        return 8;
    case FIELD_CEXOSTRING:
    case FIELD_VOID: {
        unsigned int* pData = static_cast<unsigned int*>(GetDataField(pField, nSize));
        if (pData != 0 && nSize > 3) {
            nType = *pData;
            ::ByteSwap(&nType);
            return nType;
        }
        return 0;
    }
    case FIELD_CRESREF: {
        unsigned char* pData = static_cast<unsigned char*>(GetDataField(pField, nSize));
        if (pData != 0 && nSize != 0) {
            return *pData;
        }
        return 0;
    }
    default:
        return 0;
    case FIELD_CEXOLOCSTRING:
    case FIELD_STRUCT:
    case FIELD_LIST:
        return 0;
    }
}

void* CResGFF::GetDataField(CResGFFField* pField, unsigned int& nSize)
{
    unsigned int tmp;
    if (pField != 0 && m_pFieldData != 0) {
        tmp = pField->nDataOrDataOffset;
        ::ByteSwap(&tmp);
        unsigned int nOffset = tmp;
        tmp = m_pHeader->nFieldDataCount;
        ::ByteSwap(&tmp);
        if (nOffset < tmp) {
            tmp = m_pHeader->nFieldDataCount;
            ::ByteSwap(&tmp);
            unsigned int total = tmp;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            nSize = total - tmp;
            unsigned char* pData = m_pFieldData;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            return pData + tmp;
        }
    }
    return 0;
}

void* CResGFF::GetDataLayoutField(CResGFFField* pField, unsigned int& nSize)
{
    unsigned int tmp;
    if (pField != 0 && m_pFieldIndices != 0) {
        tmp = pField->nDataOrDataOffset;
        ::ByteSwap(&tmp);
        unsigned int nOffset = tmp;
        tmp = m_pHeader->nFieldIndicesCount;
        ::ByteSwap(&tmp);
        if (nOffset < tmp) {
            tmp = m_pHeader->nFieldIndicesCount;
            ::ByteSwap(&tmp);
            unsigned int total = tmp;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            nSize = total - tmp;
            unsigned char* pData = m_pFieldIndices;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            return pData + tmp;
        }
    }
    return 0;
}

void* CResGFF::GetDataLayoutList(CResGFFField* pField, unsigned int& nSize)
{
    unsigned int tmp;
    if (pField != 0 && m_pListIndices != 0) {
        tmp = pField->nDataOrDataOffset;
        ::ByteSwap(&tmp);
        unsigned int nOffset = tmp;
        tmp = m_pHeader->nListIndicesCount;
        ::ByteSwap(&tmp);
        if (nOffset < tmp) {
            tmp = m_pHeader->nListIndicesCount;
            ::ByteSwap(&tmp);
            unsigned int total = tmp;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            nSize = total - tmp;
            unsigned char* pData = m_pListIndices;
            tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            return pData + tmp;
        }
    }
    return 0;
}

void* CResGFF::GetFieldStringID(CResStruct* pStruct, unsigned int nIndex)
{
    unsigned int tmp;
#if KOTOR1
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nIndex);
    if (pFieldRef.m_p != 0) {
        CResGFFField field = pFieldRef.Get();
        CResGFFField* pField = &field;
#else
    CResGFFField* pField = GetField(pStruct, nIndex);
    if (pField != 0) {
#endif
        if (m_paLabels != 0) {
            tmp = pField->nLabelIndex;
            ::ByteSwap(&tmp);
            unsigned int nLabelIdx = tmp;
            tmp = m_pHeader->nLabelCount;
            ::ByteSwap(&tmp);
            if (nLabelIdx < tmp) {
                GFFLabel* pLabels = m_paLabels;
                tmp = pField->nLabelIndex;
                ::ByteSwap(&tmp);
                GFFLabel* pLabel = &pLabels[tmp];
                if (pLabel != 0) {
                    __builtin_memcpy(m_sLabelCache, pLabel, sizeof(m_sLabelCache));
                    return m_sLabelCache;
                }
            }
        }
    }
    return 0;
}

void CResGFF::GetGFFFileInfo(CExoString* pFileType, CExoString* pFileVersion)
{
    if (m_pHeader != 0 && pFileType != 0 && pFileVersion != 0) {
        pFileType->Format(const_cast<char*>("%.4s"), m_pHeader->sFileType);
        pFileVersion->Format(const_cast<char*>("%.4s"), m_pHeader->sFileVersion);
    }
}

unsigned int CResGFF::GetTotalSize()
{
    if (m_pHeader == 0) {
        return 0;
    }
    unsigned int tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    unsigned int nStructCount = tmp;
    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    unsigned int nFieldCount = tmp;
    tmp = m_pHeader->nLabelCount;
    ::ByteSwap(&tmp);
    unsigned int nLabelCount = tmp;
    tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    unsigned int nFieldDataCount = tmp;
    tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    unsigned int nFieldIndicesCount = tmp;
    tmp = m_pHeader->nListIndicesCount;
    ::ByteSwap(&tmp);
    unsigned int nListIndicesCount = tmp;
    return (nFieldCount + nStructCount) * GFF_RECORD_SIZE + nLabelCount * GFF_LABEL_SIZE + nFieldDataCount +
           nFieldIndicesCount + nListIndicesCount + GFF_HEADER_SIZE;
}

int CResGFF::GetFileData(char* sPath)
{
    int nResult = 0;
    if (m_pHeader == 0) {
#if KOTOR1
        FILE* f = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exoresman/resgff.cpp",
                         "GetFileData", 1280, sPath, "rb");
#else
        FILE* f = fopen(sPath, "rb");
#endif
        if (f != 0) {
            fseek(f, 0, SEEK_END);
            unsigned int nSize = static_cast<unsigned int>(ftell(f));
            unsigned char* pBuf = new unsigned char[nSize];
            fseek(f, 0, SEEK_SET);
            size_t nRead = fread(pBuf, nSize, 1, f);
            if (nRead == 1) {
                fclose(f);
                m_pHeader = reinterpret_cast<GFFHeader*>(pBuf);
                unsigned int tmp = *reinterpret_cast<unsigned int*>(m_pHeader->sFileVersion);  // raw-offset: GFF file bytes
                ::ByteSwap(&tmp);
                if (tmp == *reinterpret_cast<const unsigned int*>(VERSION)) {
                    if (m_pHeader->nStructCount != 0) {
                        tmp = m_pHeader->nStructOffset;
                        ::ByteSwap(&tmp);
                        m_paStructs = reinterpret_cast<CResGFFStruct*>(pBuf + tmp);
                    }
                    if (m_pHeader->nFieldCount != 0) {
                        tmp = m_pHeader->nFieldOffset;
                        ::ByteSwap(&tmp);
                        m_paFields = reinterpret_cast<CResGFFField*>(pBuf + tmp);
                    }
                    if (m_pHeader->nLabelCount != 0) {
                        tmp = m_pHeader->nLabelOffset;
                        ::ByteSwap(&tmp);
                        m_paLabels = reinterpret_cast<GFFLabel*>(pBuf + tmp);
                    }
                    if (m_pHeader->nFieldDataCount != 0) {
                        tmp = m_pHeader->nFieldDataOffset;
                        ::ByteSwap(&tmp);
                        m_pFieldData = pBuf + tmp;
                    }
                    if (m_pHeader->nFieldIndicesCount != 0) {
                        tmp = m_pHeader->nFieldIndicesOffset;
                        ::ByteSwap(&tmp);
                        m_pFieldIndices = pBuf + tmp;
                    }
                    if (m_pHeader->nListIndicesCount != 0) {
                        tmp = m_pHeader->nListIndicesOffset;
                        ::ByteSwap(&tmp);
                        m_pListIndices = pBuf + tmp;
                    }
                    nResult = 1;
                    m_bResourceBacked = 1;
                    InitializeForWriting();
                    delete[] pBuf;
                    return nResult;
                }
                m_pHeader = 0;
            }
            delete[] pBuf;
        }
    }
    return nResult;
}

unsigned char CResGFF::ReadFieldBYTE(CResStruct* pStruct, char* sLabel, int& bSuccess, unsigned char defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
            unsigned int tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == 0) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return static_cast<unsigned char>(tmp);
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

char CResGFF::ReadFieldCHAR(CResStruct* pStruct, char* sLabel, int& bSuccess, char defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 1) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return static_cast<char>(tmp);
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

unsigned short CResGFF::ReadFieldWORD(CResStruct* pStruct, char* sLabel, int& bSuccess, unsigned short defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 2) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return static_cast<unsigned short>(tmp);
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

short CResGFF::ReadFieldSHORT(CResStruct* pStruct, char* sLabel, int& bSuccess, short defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 3) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return static_cast<short>(tmp);
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

unsigned int CResGFF::ReadFieldDWORD(CResStruct* pStruct, char* sLabel, int& bSuccess, unsigned int defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 4) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return tmp;
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

int CResGFF::ReadFieldINT(CResStruct* pStruct, char* sLabel, int& bSuccess, int defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 5) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return tmp;
            }
        }
    }
    bSuccess = 0;
    return static_cast<unsigned int>(defValue);
}

unsigned int CResGFF::SetFieldINT(CResStruct* pStruct, char* sLabel, int& bSuccess, int value)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
            int bIsINT;
            {
                unsigned int tmp = pField->nType;
                ::ByteSwap(&tmp);
                bIsINT = (tmp == 5);
            }
            if (bIsINT) {
                bSuccess = 1;
                {
                    unsigned int tmp = static_cast<unsigned int>(value);
                    ::ByteSwap(&tmp);
                    pField->nDataOrDataOffset = tmp;
                }
#if KOTOR1
                pFieldRef.Set(field);
#endif
                unsigned int nOld = pField->nDataOrDataOffset;
                ::ByteSwap(&nOld);
                return nOld;
            }
        }
    }
    bSuccess = 0;
    return static_cast<unsigned int>(value);
}

float CResGFF::ReadFieldFLOAT(CResStruct* pStruct, char* sLabel, int& bSuccess, float defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 8) {
                bSuccess = 1;
                tmp = pField->nDataOrDataOffset;
                ::ByteSwap(&tmp);
                return *reinterpret_cast<float*>(&tmp);
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

unsigned long long CResGFF::ReadFieldDWORD64(CResStruct* pStruct, char* sLabel, int& bSuccess, unsigned long long defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            union { unsigned int nType; unsigned long long v; } tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if !KOTOR1
            union { unsigned int nType; unsigned long long v; } tmp;
#endif
            tmp.nType = pField->nType;
            ::ByteSwap(&tmp.nType);
            if (tmp.nType == 6) {
                unsigned int nSize;
                unsigned long long* pData = static_cast<unsigned long long*>(GetDataField(pField, nSize));
                if (pData != 0 && nSize > 7) {
                    bSuccess = 1;
                    tmp.v = *pData;
                    ::ByteSwap(&tmp.v);
                    return tmp.v;
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

#if KOTOR1
int64_t CResGFF::ReadFieldINT64(CResStruct* pStruct, char* sLabel, int& bSuccess, int64_t defValue)
#else
long long CResGFF::ReadFieldINT64(CResStruct* pStruct, char* sLabel, int& bSuccess, long long defValue)
#endif
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            union { unsigned int nType; unsigned long long v; } tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if !KOTOR1
            union { unsigned int nType; unsigned long long v; } tmp;
#endif
            tmp.nType = pField->nType;
            ::ByteSwap(&tmp.nType);
            if (tmp.nType == 7) {
                unsigned int nSize;
                unsigned long long* pData = static_cast<unsigned long long*>(GetDataField(pField, nSize));
                if (pData != 0 && nSize > 7) {
                    bSuccess = 1;
                    tmp.v = *pData;
                    ::ByteSwap(&tmp.v);
                    return static_cast<long long>(tmp.v);
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

double CResGFF::ReadFieldDOUBLE(CResStruct* pStruct, char* sLabel, int& bSuccess, double defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            union { unsigned int nType; unsigned long long v; double d; } tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if !KOTOR1
            union { unsigned int nType; unsigned long long v; double d; } tmp;
#endif
            tmp.nType = pField->nType;
            ::ByteSwap(&tmp.nType);
            if (tmp.nType == 9) {
                unsigned int nSize;
                unsigned long long* pData = static_cast<unsigned long long*>(GetDataField(pField, nSize));
                if (pData != 0 && nSize > 7) {
                    bSuccess = 1;
                    tmp.v = *pData;
                    ::ByteSwap(&tmp.v);
                    return tmp.d;
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

CResRef CResGFF::ReadFieldCResRef(CResStruct* pStruct, char* sLabel, int& bSuccess, const CResRef& defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == FIELD_CRESREF) {
                unsigned char* pData = static_cast<unsigned char*>(GetDataField(pField, tmp));
                if (pData != 0 && tmp != 0) {
                    uint64_t nLen = pData[0];
                    if (nLen < tmp) {
                        bSuccess = 1;
                        return CResRef(pData + 1, static_cast<unsigned char>(nLen));
                    }
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

CExoString CResGFF::ReadFieldCExoString(CResStruct* pStruct, char* sLabel, int& bSuccess, const CExoString& defValue)
{
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == 10) {
                unsigned int nSize;
                unsigned int* pData = static_cast<unsigned int*>(GetDataField(pField, nSize));
                if (pData == 0 || nSize <= 3) {
                    bSuccess = 0;
                    return defValue;
                }
                {
                    tmp = *pData;
                    ::ByteSwap(&tmp);
                    if (static_cast<uint64_t>(tmp) + 4 <= static_cast<uint64_t>(nSize)) {
                        bSuccess = 1;
                        return CExoString(reinterpret_cast<const char*>(pData + 1), tmp);
                    }
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

Quaternion CResGFF::ReadFieldQuaternion(CResStruct* pStruct, char* sLabel, int& bSuccess, const Quaternion& defValue)
{
    unsigned int tmp;
    Quaternion out;  // identity by default constructor
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
            tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == FIELD_QUATERNION) {
                float* pData = static_cast<float*>(GetDataField(pField, tmp));
                if (pData == 0 || tmp <= 15) {
                    bSuccess = 0;
                    return defValue;
                }
                {
                    bSuccess = 1;
                    out.w = pData[0];
                    out.x = pData[1];
                    out.y = pData[2];
                    out.z = pData[3];
                    ::ByteSwap(&out.w);
                    ::ByteSwap(&out.x);
                    ::ByteSwap(&out.y);
                    ::ByteSwap(&out.z);
                    return out;
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

Vector CResGFF::ReadFieldVector(CResStruct* pStruct, char* sLabel, int& bSuccess, const Vector& defValue)
{
    unsigned int tmp;
    Vector out;  // zeroed by Vector's default ctor
    if (pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
            tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == FIELD_VECTOR) {
                float* pData = static_cast<float*>(GetDataField(pField, tmp));
#if KOTOR1
                if (pData != 0 && tmp > 11) {
#else
                if (pData == 0 || tmp <= 11) {
                    bSuccess = 0;
                    return defValue;
                }
                {
#endif
                    bSuccess = 1;
                    out.x = pData[0];
                    out.y = pData[1];
                    out.z = pData[2];
                    ::ByteSwap(&out.x);
                    ::ByteSwap(&out.y);
                    ::ByteSwap(&out.z);
                    return out;
                }
            }
        }
    }
    bSuccess = 0;
    return defValue;
}

CExoLocString CResGFF::ReadFieldCExoLocString(CResStruct* pStruct, char* sLabel, int& bSuccess, const CExoLocString& defValue)
{
    CExoLocString tmpLoc;
    unsigned int tmp K2_UNSET;
    if (pStruct == 0 || sLabel == 0) {
        bSuccess = 0;
        return defValue;
    }
    unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
    // K1: a missing field and a wrong type share one failure exit.
    CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
    CResGFFField field;
    CResGFFField* pField = &field;
    if (pFieldRef.m_p != 0) {
        field = pFieldRef.Get();
        tmp = pField->nType;
        ::ByteSwap(&tmp);
    }
    if (pFieldRef.m_p == 0 || tmp != FIELD_CEXOLOCSTRING) {
        bSuccess = 0;
        return defValue;
    }
#else
    CResGFFField* pField = GetField(pStruct, nField);
    if (pField == 0) {
        bSuccess = 0;
        return defValue;
    }
    tmp = pField->nType;
    ::ByteSwap(&tmp);
    if (tmp != FIELD_CEXOLOCSTRING) {
        bSuccess = 0;
        return defValue;
    }
#endif
    unsigned int nSize;
    unsigned char* pData = static_cast<unsigned char*>(GetDataField(pField, nSize));
    if (pData == 0 || nSize < 4) {
        bSuccess = 0;
        return defValue;
    }
    ExoLocStringHeader* pHdr = reinterpret_cast<ExoLocStringHeader*>(pData);
    tmp = pHdr->nTotalSize;
    ::ByteSwap(&tmp);
    unsigned int nTotal = tmp;
    if (nSize < static_cast<uint64_t>(nTotal) + 4) {
        bSuccess = 0;
        return defValue;
    }
    tmp = pHdr->nStringRef;
    ::ByteSwap(&tmp);
    unsigned int nStrRef = tmp;
    tmp = pHdr->nStringCount;
    ::ByteSwap(&tmp);
    unsigned int nCount = tmp;
    tmpLoc.m_nStrRef = nStrRef;
    unsigned char* p = pData + 12;
    int nRemaining = static_cast<int>(nTotal) - 8;
    unsigned int i;
    for (i = 0; i < nCount; i++) {
        ExoLocSubString* pSub = reinterpret_cast<ExoLocSubString*>(p);
        tmp = pSub->nLanguageID;
        ::ByteSwap(&tmp);
        int nLang = static_cast<int>(tmp);
        tmp = pSub->nStringLength;
        ::ByteSwap(&tmp);
        int nStrLen = static_cast<int>(tmp);
        CExoString str(reinterpret_cast<const char*>(p + 8), nStrLen);
        tmpLoc.AddString(nLang / 2, str, nLang % 2);
        p = p + 8 + nStrLen;
        nRemaining = nRemaining - 8 - nStrLen;
    }
    if (nRemaining != 0) {
        bSuccess = 0;
        return defValue;
    }
    bSuccess = 1;
    return tmpLoc;
}

void* CResGFF::ReadFieldVOID(CResStruct* pStruct, void* pBuffer, unsigned int nBufSize, char* sLabel, int& bSuccess, void* pDefault)
{
    if (pBuffer != 0 && pStruct != 0 && sLabel != 0) {
        unsigned int nField = GetFieldByLabel(pStruct, sLabel);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(pStruct, nField);
        if (pFieldRef.m_p != 0) {
            unsigned int tmp;
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(pStruct, nField);
        if (pField != 0) {
#endif
#if KOTOR1
            tmp = pField->nType;
#else
            unsigned int tmp = pField->nType;
#endif
            ::ByteSwap(&tmp);
            if (tmp == FIELD_VOID) {
                unsigned int nSize;
                unsigned int* pData = static_cast<unsigned int*>(GetDataField(pField, nSize));
#if KOTOR1
                if (pData == 0 || nSize <= 3) {
                    bSuccess = 0;
                    return pDefault;
                }
                {
#else
                if (pData != 0 && nSize > 3) {
#endif
                    tmp = *pData;
                    ::ByteSwap(&tmp);
                    if (static_cast<uint64_t>(tmp) + 4 <= static_cast<uint64_t>(nSize)) {
                        unsigned int nCopy = tmp < nBufSize ? tmp : nBufSize;
                        bSuccess = 1;
                        memcpy(pBuffer, pData + 1, nCopy);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                        return pBuffer;
                    }
                }
            }
        }
    }
    bSuccess = 0;
    return pDefault;
}

void CResGFF::InitializeForWriting()
{
    GFFHeader* pNewHeader = static_cast<GFFHeader*>(::operator new(sizeof(GFFHeader)));
    pNewHeader->nStructOffset = 0;
    pNewHeader->nFieldOffset = 0;
    pNewHeader->nLabelOffset = 0;
    pNewHeader->nFieldDataOffset = 0;
    pNewHeader->nFieldIndicesOffset = 0;
    pNewHeader->nListIndicesOffset = 0;

    unsigned int tmp;
    GFFHeader* pOldHeader = m_pHeader;
    if (pOldHeader == 0) {
        *reinterpret_cast<unsigned int*>(pNewHeader->sFileType) = 0;  // raw-offset: GFF file bytes
        *reinterpret_cast<unsigned int*>(pNewHeader->sFileVersion) = 0;  // raw-offset: GFF file bytes
        pNewHeader->nStructCount = 0;
        m_nStructCapacity = GFF_STRUCT_GROW;
        pNewHeader->nFieldCount = 0;
        m_nFieldCapacity = GFF_FIELD_GROW;
        pNewHeader->nLabelCount = 0;
        m_nLabelCapacity = GFF_LABEL_GROW;
        pNewHeader->nFieldDataCount = 0;
        m_nFieldDataCapacity = GFF_FIELD_DATA_GROW;
        pNewHeader->nFieldIndicesCount = 0;
        m_nFieldIndicesCapacity = GFF_INDICES_GROW;
        pNewHeader->nListIndicesCount = 0;
        m_nListIndicesCapacity = GFF_INDICES_GROW;
    } else {
        *reinterpret_cast<unsigned int*>(pNewHeader->sFileType) = *reinterpret_cast<unsigned int*>(pOldHeader->sFileType);  // raw-offset: GFF file bytes
        *reinterpret_cast<unsigned int*>(pNewHeader->sFileVersion) = *reinterpret_cast<unsigned int*>(pOldHeader->sFileVersion);  // raw-offset: GFF file bytes
        pNewHeader->nStructCount = pOldHeader->nStructCount;
        tmp = pOldHeader->nStructCount;
        ::ByteSwap(&tmp);
        m_nStructCapacity = m_nStructGrow + tmp;

        pNewHeader->nFieldCount = m_pHeader->nFieldCount;
        tmp = m_pHeader->nFieldCount;
        ::ByteSwap(&tmp);
        m_nFieldCapacity = m_nFieldGrow + tmp;

        pNewHeader->nLabelCount = m_pHeader->nLabelCount;
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        m_nLabelCapacity = m_nLabelGrow + tmp;

        pNewHeader->nFieldDataCount = m_pHeader->nFieldDataCount;
        tmp = m_pHeader->nFieldDataCount;
        ::ByteSwap(&tmp);
        m_nFieldDataCapacity = m_nFieldDataGrow + tmp;

        pNewHeader->nFieldIndicesCount = m_pHeader->nFieldIndicesCount;
        tmp = m_pHeader->nFieldIndicesCount;
        ::ByteSwap(&tmp);
        m_nFieldIndicesCapacity = m_nFieldIndicesGrow + tmp;

        pNewHeader->nListIndicesCount = m_pHeader->nListIndicesCount;
        tmp = m_pHeader->nListIndicesCount;
        ::ByteSwap(&tmp);
        m_nListIndicesCapacity = m_nListIndicesGrow + tmp;
    }

#if KOTOR1
    // K1: each old buffer pointer is read once, before its count is swapped.
    CResGFFStruct* pNewStructs = new CResGFFStruct[m_nStructCapacity];
    CResGFFStruct* pOldStructs = m_paStructs;
    if (pOldStructs != 0) {
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        memcpy(pNewStructs, pOldStructs, static_cast<uint64_t>(tmp) * sizeof(CResGFFStruct));
    }

    CResGFFField* pNewFields = new CResGFFField[m_nFieldCapacity];
    CResGFFField* pOldFields = m_paFields;
    if (pOldFields != 0) {
        tmp = m_pHeader->nFieldCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFields, pOldFields, static_cast<uint64_t>(tmp) * sizeof(CResGFFField));
    }

    GFFLabel* pNewLabels = new GFFLabel[m_nLabelCapacity];
    GFFLabel* pOldLabels = m_paLabels;
    if (pOldLabels != 0) {
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        memcpy(pNewLabels, pOldLabels, static_cast<uint64_t>(tmp) * sizeof(GFFLabel));
    }

    unsigned char* pNewFieldData = new unsigned char[m_nFieldDataCapacity];
    unsigned char* pOldFieldData = m_pFieldData;
    if (pOldFieldData != 0) {
        tmp = m_pHeader->nFieldDataCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFieldData, pOldFieldData, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

    unsigned char* pNewFieldIndices = new unsigned char[m_nFieldIndicesCapacity];
    unsigned char* pOldFieldIndices = m_pFieldIndices;
    if (pOldFieldIndices != 0) {
        tmp = m_pHeader->nFieldIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFieldIndices, pOldFieldIndices, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

    unsigned char* pNewListIndices = new unsigned char[m_nListIndicesCapacity];
    unsigned char* pOldListIndices = m_pListIndices;
    if (pOldListIndices != 0) {
        tmp = m_pHeader->nListIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNewListIndices, pOldListIndices, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

#else
    CResGFFStruct* pNewStructs = new CResGFFStruct[m_nStructCapacity];
    if (m_paStructs != 0) {
        tmp = m_pHeader->nStructCount;
        ::ByteSwap(&tmp);
        memcpy(pNewStructs, m_paStructs, static_cast<uint64_t>(tmp) * sizeof(CResGFFStruct));
    }

    CResGFFField* pNewFields = new CResGFFField[m_nFieldCapacity];
    if (m_paFields != 0) {
        tmp = m_pHeader->nFieldCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFields, m_paFields, static_cast<uint64_t>(tmp) * sizeof(CResGFFField));
    }

    GFFLabel* pNewLabels = new GFFLabel[m_nLabelCapacity];
    if (m_paLabels != 0) {
        tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        memcpy(pNewLabels, m_paLabels, static_cast<uint64_t>(tmp) * sizeof(GFFLabel));
    }

    unsigned char* pNewFieldData = new unsigned char[m_nFieldDataCapacity];
    if (m_pFieldData != 0) {
        tmp = m_pHeader->nFieldDataCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFieldData, m_pFieldData, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

    unsigned char* pNewFieldIndices = new unsigned char[m_nFieldIndicesCapacity];
    if (m_pFieldIndices != 0) {
        tmp = m_pHeader->nFieldIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNewFieldIndices, m_pFieldIndices, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

    unsigned char* pNewListIndices = new unsigned char[m_nListIndicesCapacity];
    if (m_pListIndices != 0) {
        tmp = m_pHeader->nListIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNewListIndices, m_pListIndices, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }

#endif
    if (m_bResourceBacked == 0) {
        if (m_pListIndices != 0) {
            delete[] m_pListIndices;
        }
        if (m_pFieldIndices != 0) {
            delete[] m_pFieldIndices;
        }
        if (m_pFieldData != 0) {
            delete[] m_pFieldData;
        }
        if (m_paLabels != 0) {
            delete[] m_paLabels;
        }
#if KOTOR1
        m_paFields.Delete();
        m_paStructs.Delete();
#else
        if (m_paFields != 0) {
            delete[] m_paFields;
        }
        if (m_paStructs != 0) {
            delete[] m_paStructs;
        }
#endif
        if (m_pHeader != 0) {
            operator delete(m_pHeader);
        }
    } else {
        ReleaseResource();
    }

    m_pHeader = pNewHeader;
    m_paStructs = pNewStructs;
    m_pListIndices = pNewListIndices;
    m_pFieldIndices = pNewFieldIndices;
    m_paFields = pNewFields;
    m_pFieldData = pNewFieldData;
    m_paLabels = pNewLabels;
}

#if KOTOR1
unsigned int CResGFF::AddStruct(unsigned int nType)
{
    if (m_pHeader == 0 || m_paStructs == 0) {
        return INVALID_INDEX;
    }
    unsigned int tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    unsigned int nIndex = tmp;
    if (nIndex >= m_nStructCapacity) {
        unsigned int nNewCap = m_nStructGrow + nIndex;
        m_nStructCapacity = nNewCap;
        m_nStructGrow *= 2;
        CResGFFStruct* pNew = new CResGFFStruct[nNewCap];
        memcpy(pNew, m_paStructs.m_p, static_cast<uint64_t>(nIndex) * sizeof(CResGFFStruct));
        m_paStructs.Delete();
        m_paStructs = pNew;
    }
    tmp = nIndex + 1;
    ::ByteSwap(&tmp);
    m_pHeader->nStructCount = tmp;
    tmp = nType;
    ::ByteSwap(&tmp);
    CResGFFStruct rec;
    rec.nType = tmp;
    rec.nDataOrDataOffset = INVALID_INDEX;
    rec.nFieldCount = 0;
    m_paStructs.SetAt(nIndex, rec);
    return nIndex;
}
#else
unsigned int CResGFF::AddStruct(unsigned int nType)
{
    if (m_pHeader == 0 || m_paStructs == 0) {
        return INVALID_INDEX;
    }
    unsigned int tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    unsigned int nIndex = tmp;
    if (nIndex >= m_nStructCapacity) {
        unsigned int nNewCap = m_nStructGrow + nIndex;
        m_nStructCapacity = nNewCap;
        CResGFFStruct* pNew = new CResGFFStruct[nNewCap];
        CResGFFStruct* pOld = m_paStructs;
        memcpy(pNew, pOld, static_cast<uint64_t>(nIndex) * sizeof(CResGFFStruct));
        if (pOld != 0) {
            delete[] pOld;
        }
        m_paStructs = pNew;
    }
    tmp = nIndex + 1;
    ::ByteSwap(&tmp);
    m_pHeader->nStructCount = tmp;
    tmp = nType;
    ::ByteSwap(&tmp);
    CResGFFStruct* pRec = &m_paStructs[nIndex];
    pRec->nType = tmp;
    pRec->nDataOrDataOffset = INVALID_INDEX;
    pRec->nFieldCount = 0;
    return nIndex;
}
#endif

void* CResGFF::AddDataLayoutField(unsigned int nOffset, unsigned int nInsertPos, unsigned int nInsertLen)
{
    if (m_pHeader == 0 || m_pFieldIndices == 0) {
        return 0;
    }
    unsigned int tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    unsigned int nBase = (tmp != nInsertPos + nOffset) ? tmp : nOffset;
    unsigned int nNewTotal = nBase + nInsertLen;
    if (nNewTotal >= m_nFieldIndicesCapacity) {
        m_nFieldIndicesCapacity = m_nFieldIndicesGrow + nNewTotal;
#if KOTOR1
        m_nFieldIndicesGrow *= 2;
#endif
        unsigned char* pNew = new unsigned char[m_nFieldIndicesCapacity];
        unsigned char* pOld = m_pFieldIndices;
        tmp = m_pHeader->nFieldIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNew, pOld, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
        if (m_pFieldIndices != 0) {
            delete[] m_pFieldIndices;
        }
        m_pFieldIndices = pNew;
    }
    if (nBase != nOffset) {
        memcpy(m_pFieldIndices + nBase, m_pFieldIndices + nOffset, nInsertPos);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
        m_nFieldIndicesUsed = m_nFieldIndicesUsed + nInsertPos;
    }
    tmp = nNewTotal;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldIndicesCount = tmp;
    return m_pFieldIndices + nBase;
}

void* CResGFF::AddDataField(unsigned int nOffset, unsigned int nInsertPos, unsigned int nInsertLen)
{
    if (m_pHeader == 0 || m_pFieldData == 0) {
        return 0;
    }
    unsigned int tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    unsigned int nBase = (tmp != nInsertPos + nOffset) ? tmp : nOffset;
    unsigned int nNewTotal = nBase + nInsertLen;
    if (nNewTotal >= m_nFieldDataCapacity) {
        m_nFieldDataCapacity = m_nFieldDataGrow + nNewTotal;
#if KOTOR1
        m_nFieldDataGrow *= 2;
#endif
        unsigned char* pNew = new unsigned char[m_nFieldDataCapacity];
        unsigned char* pOld = m_pFieldData;
        tmp = m_pHeader->nFieldDataCount;
        ::ByteSwap(&tmp);
        memcpy(pNew, pOld, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
        if (m_pFieldData != 0) {
            delete[] m_pFieldData;
        }
        m_pFieldData = pNew;
    }
    if (nBase != nOffset) {
        memcpy(m_pFieldData + nBase, m_pFieldData + nOffset, nInsertPos);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
    }
    tmp = nNewTotal;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldDataCount = tmp;
    return m_pFieldData + nBase;
}

unsigned int CResGFF::AddLabel(char* sLabel)
{
    if (m_pHeader == 0 || m_paLabels == 0) {
        return INVALID_INDEX;
    }
    unsigned int n;
    {
        unsigned int tmp = m_pHeader->nLabelCount;
        ::ByteSwap(&tmp);
        n = tmp;
    }
    GFFLabel newLabel = {};
    strncpy(newLabel.s, sLabel, 16);
#if KOTOR1
    // K1: the existing labels are searched with strncmp(..., 16).
    unsigned int i = n;
    while (i != 0) {
        i--;
        if (strncmp(m_paLabels[i].s, newLabel.s, 16) == 0) {
            return i;
        }
    }
#else
    if (n != 0) {
        const unsigned int* pWant = reinterpret_cast<const unsigned int*>(&newLabel);
        unsigned int i = n;
        do {
            i--;
            const unsigned int* p = reinterpret_cast<const unsigned int*>(&m_paLabels[i]);
            if (pWant[0] == p[0] && pWant[1] == p[1] && pWant[2] == p[2] && pWant[3] == p[3]) {  // raw-offset: GFF file bytes
                return i;
            }
        } while (i != 0);
    }
#endif
    if (n >= m_nLabelCapacity) {
        m_nLabelCapacity = m_nLabelGrow + n;
#if KOTOR1
        m_nLabelGrow *= 2;
#endif
        K2_ASSUME(n < m_nLabelCapacity);   // the grow step starts at 0x62 and only doubles
        GFFLabel* pNew = new GFFLabel[m_nLabelCapacity];
        GFFLabel* pOld = m_paLabels;
        memcpy(pNew, pOld, static_cast<uint64_t>(n) * sizeof(GFFLabel));
        if (pOld != 0) {
            delete[] pOld;
        }
        m_paLabels = pNew;
    }
    unsigned int nNewCount = n + 1;
    ::ByteSwap(&nNewCount);
    m_pHeader->nLabelCount = nNewCount;
    m_paLabels[n] = newLabel;
    return n;
}

void* CResGFF::AddDataLayoutList(unsigned int nOffset, unsigned int nInsertPos, unsigned int nInsertLen)
{
    if (m_pHeader == 0 || m_pListIndices == 0) {
        return 0;
    }
    unsigned int tmp = m_pHeader->nListIndicesCount;
    ::ByteSwap(&tmp);
    unsigned int nBase = (tmp != nInsertPos + nOffset) ? tmp : nOffset;
    unsigned int nNewTotal = nBase + nInsertLen;
    if (nNewTotal >= m_nListIndicesCapacity) {
        m_nListIndicesCapacity = m_nListIndicesGrow + nNewTotal;
#if KOTOR1
        m_nListIndicesGrow *= 2;
#endif
        unsigned char* pNew = new unsigned char[m_nListIndicesCapacity];
        unsigned char* pOld = m_pListIndices;
        tmp = m_pHeader->nListIndicesCount;
        ::ByteSwap(&tmp);
        memcpy(pNew, pOld, tmp);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
        if (m_pListIndices != 0) {
            delete[] m_pListIndices;
        }
        m_pListIndices = pNew;
    }
    if (nBase != nOffset) {
        memcpy(m_pListIndices + nBase, m_pListIndices + nOffset, nInsertPos);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
        m_nListIndicesUsed = m_nListIndicesUsed + nInsertPos;
    }
    tmp = nNewTotal;
    ::ByteSwap(&tmp);
    m_pHeader->nListIndicesCount = tmp;
    return m_pListIndices + nBase;
}

#if KOTOR1
CResGFFAlignedPtr<CResGFFField> CResGFF::AddField(CResStruct* pStruct, char* sLabel, unsigned int nType)
{
    if (pStruct == 0 || sLabel == 0) {
        return CResGFFAlignedPtr<CResGFFField>();
    }
    if (m_pHeader == 0 || m_paStructs == 0) {
        return CResGFFAlignedPtr<CResGFFField>();
    }
    unsigned int idx = pStruct->nStructIndex;
    unsigned int tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    if (idx >= tmp) {
        return CResGFFAlignedPtr<CResGFFField>();
    }
    CResGFFAlignedPtr<CResGFFStruct> pRec(&m_paStructs.m_p[pStruct->nStructIndex]);
    if (pRec.m_p == 0) {
        return CResGFFAlignedPtr<CResGFFField>();
    }
    CResGFFStruct rec = pRec.Get();
    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    unsigned int nFieldIndex = tmp;
    if (nFieldIndex >= m_nFieldCapacity) {
        m_nFieldCapacity = m_nFieldGrow + nFieldIndex;
        m_nFieldGrow *= 2;
        CResGFFField* pNew = new CResGFFField[m_nFieldCapacity];
        memcpy(pNew, m_paFields.m_p, static_cast<uint64_t>(nFieldIndex) * sizeof(CResGFFField));
        m_paFields.Delete();
        m_paFields = pNew;
    }
    tmp = nFieldIndex + 1;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldCount = tmp;

    tmp = rec.nFieldCount;
    ::ByteSwap(&tmp);
    unsigned int nStructFieldCount = tmp;
    // nDataOrDataOffset is the (byte-swapped) field index at 0 fields, the head of a promoted
    // FieldIndices array at 1 field, and a growable array at 2 or more.
    if (nStructFieldCount != 0) {
        CResGFFAlignedPtr<unsigned int> pSlot;
        if (nStructFieldCount == 1) {
            pSlot = static_cast<unsigned int*>(AddDataLayoutField(NEW_DATA_OFFSET, 0, 8));
            if (pSlot.m_p == 0) {
                return CResGFFAlignedPtr<CResGFFField>();
            }
            pSlot.Set(rec.nDataOrDataOffset);
        } else {
            tmp = rec.nDataOrDataOffset;
            ::ByteSwap(&tmp);
            pSlot = static_cast<unsigned int*>(AddDataLayoutField(
                tmp, nStructFieldCount * 4, nStructFieldCount * 4 + 4));
            if (pSlot.m_p == 0) {
                return CResGFFAlignedPtr<CResGFFField>();
            }
        }
        tmp = nFieldIndex;
        ::ByteSwap(&tmp);
        CResGFFAlignedPtr<unsigned int>(pSlot.m_p + nStructFieldCount).Set(tmp);
        tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pSlot.m_p) - m_pFieldIndices);
        ::ByteSwap(&tmp);
        rec.nDataOrDataOffset = tmp;
    } else {
        tmp = nFieldIndex;
        ::ByteSwap(&tmp);
        rec.nDataOrDataOffset = tmp;
    }
    tmp = nStructFieldCount + 1;
    ::ByteSwap(&tmp);
    rec.nFieldCount = tmp;
    pRec.Set(rec);

    CResGFFField field;
    tmp = AddLabel(sLabel);
    ::ByteSwap(&tmp);
    field.nLabelIndex = tmp;
    tmp = nType;
    ::ByteSwap(&tmp);
    field.nType = tmp;
    field.nDataOrDataOffset = INVALID_INDEX;
    m_paFields.SetAt(nFieldIndex, field);
    return CResGFFAlignedPtr<CResGFFField>(&m_paFields.m_p[nFieldIndex]);
}
#else
void* CResGFF::AddField(CResStruct* pStruct, char* sLabel, unsigned int nType)
{
    if (pStruct == 0 || sLabel == 0) {
        return 0;
    }
    if (m_pHeader == 0 || m_paStructs == 0) {
        return 0;
    }
    unsigned int idx = pStruct->nStructIndex;
    unsigned int tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    if (idx >= tmp) {
        return 0;
    }
    CResGFFStruct* pRec = &m_paStructs[pStruct->nStructIndex];
    if (pRec == 0) {
        return 0;
    }
    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    unsigned int nFieldIndex = tmp;
    if (nFieldIndex >= m_nFieldCapacity) {
        m_nFieldCapacity = m_nFieldGrow + nFieldIndex;
        CResGFFField* pNew = new CResGFFField[m_nFieldCapacity];
        CResGFFField* pOld = m_paFields;
        memcpy(pNew, pOld, static_cast<uint64_t>(nFieldIndex) * sizeof(CResGFFField));
        if (pOld != 0) {
            delete[] pOld;
        }
        m_paFields = pNew;
    }
    tmp = nFieldIndex + 1;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldCount = tmp;

    tmp = pRec->nFieldCount;
    ::ByteSwap(&tmp);
    unsigned int nStructFieldCount = tmp;
    // nDataOrDataOffset is the (byte-swapped) field index at 0 fields, the head of a promoted
    // FieldIndices array at 1 field, and a growable array at 2 or more.
    if (nStructFieldCount != 0) {
        unsigned int* pSlot;
        if (nStructFieldCount == 1) {
            pSlot = static_cast<unsigned int*>(AddDataLayoutField(NEW_DATA_OFFSET, 0, 8));
            if (pSlot == 0) {
                return 0;
            }
            pSlot[0] = pRec->nDataOrDataOffset;
        } else {
            tmp = pRec->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            pSlot = static_cast<unsigned int*>(AddDataLayoutField(
                tmp, nStructFieldCount * 4, nStructFieldCount * 4 + 4));
            if (pSlot == 0) {
                return 0;
            }
        }
        tmp = nFieldIndex;
        ::ByteSwap(&tmp);
        pSlot[nStructFieldCount] = tmp;
        tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pSlot) - m_pFieldIndices);
        ::ByteSwap(&tmp);
        pRec->nDataOrDataOffset = tmp;
    } else {
        tmp = nFieldIndex;
        ::ByteSwap(&tmp);
        pRec->nDataOrDataOffset = tmp;
    }
    tmp = nStructFieldCount + 1;
    ::ByteSwap(&tmp);
    pRec->nFieldCount = tmp;

    tmp = AddLabel(sLabel);
    ::ByteSwap(&tmp);
    m_paFields[nFieldIndex].nLabelIndex = tmp;
    tmp = nType;
    ::ByteSwap(&tmp);
    m_paFields[nFieldIndex].nType = tmp;
    m_paFields[nFieldIndex].nDataOrDataOffset = INVALID_INDEX;
    return &m_paFields[nFieldIndex];
}
#endif

int CResGFF::AddListElement(CResStruct* pOut, CResList* pList, unsigned int nType)
{
    unsigned int nSize;
    unsigned int tmp;
    if (pOut != 0 && pList != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
        unsigned int nField = GetFieldByLabel(&pList->m_struct, pList->m_label);
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = GetField(&pList->m_struct, nField);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = GetField(&pList->m_struct, nField);
        if (pField != 0) {
#endif
            tmp = pField->nType;
            ::ByteSwap(&tmp);
            if (tmp == FIELD_LIST) {
                unsigned int* pList2 = static_cast<unsigned int*>(GetDataLayoutList(pField, nSize));
                if (pList2 != 0 && nSize >= 4) {
                    tmp = pList2[0];
                    ::ByteSwap(&tmp);
                    unsigned int nCount = tmp;
                    unsigned int nNewCount = nCount + 1;
                    uint64_t nNewSize = static_cast<uint64_t>(nNewCount) * 4;
                    if (nNewSize <= nSize) {
                        tmp = pField->nDataOrDataOffset;
                        ::ByteSwap(&tmp);
                        unsigned int* pNew = static_cast<unsigned int*>(AddDataLayoutList(tmp, static_cast<unsigned int>(nNewSize), nCount * 4 + 8));
                        if (pNew != 0) {
                            tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pNew) - m_pListIndices);
                            ::ByteSwap(&tmp);
                            pField->nDataOrDataOffset = tmp;
#if KOTOR1
                            pFieldRef.Set(field);
#endif
                            unsigned int nNewStructIdx = AddStruct(nType);
                            pOut->nStructIndex = nNewStructIdx;
                            if (nNewStructIdx != INVALID_INDEX) {
                                tmp = nNewCount;
                                ::ByteSwap(&tmp);
#if KOTOR1
                                CResGFFAlignedPtr<unsigned int>(pNew).Set(tmp);
#else
                                pNew[0] = tmp;
#endif
                                tmp = pOut->nStructIndex;
                                ::ByteSwap(&tmp);
#if KOTOR1
                                CResGFFAlignedPtr<unsigned int>(reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(pNew) + nNewSize)).Set(tmp);
#else
                                *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(pNew) + nNewSize) = tmp;
#endif
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int CResGFF::AddList(CResList* pList, CResStruct* pStruct, char* sLabel)
{
    if (pList != 0 && pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_LIST);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_LIST));
        if (pField != 0) {
#endif
            unsigned int tmp = pField->nDataOrDataOffset;
            ::ByteSwap(&tmp);
            unsigned int* pNew = static_cast<unsigned int*>(AddDataLayoutList(tmp, 0, 4));
            if (pNew != 0) {
                tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pNew) - m_pListIndices);
                ::ByteSwap(&tmp);
                pField->nDataOrDataOffset = tmp;
#if KOTOR1
                pFieldRef.Set(field);
                tmp = 0;
                CResGFFAlignedPtr<unsigned int>(pNew).Set(tmp);
#else
                pNew[0] = 0;
#endif
                pList->m_struct = *pStruct;
                strncpy(pList->m_label, sLabel, 16);
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::AddStructToStruct(CResStruct* pOut, CResStruct* pStruct, char* sLabel, unsigned int nType)
{
    if (pOut != 0 && pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_STRUCT);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_STRUCT));
        if (pField != 0) {
#endif
            unsigned int nNewStructIdx = AddStruct(nType);
            if (nNewStructIdx != INVALID_INDEX) {
                unsigned int nIdxSwapped = nNewStructIdx;
                ::ByteSwap(&nIdxSwapped);
                pField->nDataOrDataOffset = nIdxSwapped;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                pOut->nStructIndex = nNewStructIdx;
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldBYTE(CResStruct* pStruct, unsigned char value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 0);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 0));
        if (pField != 0) {
#endif
            unsigned int v = value & 0xff;
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldCHAR(CResStruct* pStruct, char value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 1);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 1));
        if (pField != 0) {
#endif
            unsigned int v = static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(value)));
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldWORD(CResStruct* pStruct, unsigned short value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 2);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 2));
        if (pField != 0) {
#endif
            unsigned int v = value & 0xffff;
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldSHORT(CResStruct* pStruct, short value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 3);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 3));
        if (pField != 0) {
#endif
            unsigned int v = static_cast<unsigned int>(value);
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldDWORD(CResStruct* pStruct, unsigned int value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 4);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 4));
        if (pField != 0) {
#endif
            unsigned int v = value;
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldINT(CResStruct* pStruct, int value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 5);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 5));
        if (pField != 0) {
#endif
            unsigned int v = static_cast<unsigned int>(value);
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldFLOAT(CResStruct* pStruct, float value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 8);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 8));
        if (pField != 0) {
#endif
            unsigned int v = *reinterpret_cast<unsigned int*>(&value);
            ::ByteSwap(&v);
            pField->nDataOrDataOffset = v;
#if KOTOR1
            pFieldRef.Set(field);
#endif
            return 1;
        }
    }
    return 0;
}

int CResGFF::WriteFieldDWORD64(CResStruct* pStruct, unsigned long long value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 6);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 6));
        if (pField != 0) {
#endif
            unsigned long long* pData = static_cast<unsigned long long*>(AddDataField(NEW_DATA_OFFSET, 0, 8));
            if (pData != 0) {
                union {
                    unsigned int nOff;
                    unsigned long long v;
                    volatile unsigned long long vv;
                } tmp;
                tmp.nOff = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp.nOff);
                pField->nDataOrDataOffset = tmp.nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                tmp.v = value;
                ::ByteSwap(&tmp.v);
                tmp.vv = tmp.v;
                *pData = tmp.v;
                return 1;
            }
        }
    }
    return 0;
}

#if KOTOR1
int CResGFF::WriteFieldINT64(CResStruct* pStruct, int64_t value, char* sLabel)
#else
int CResGFF::WriteFieldINT64(CResStruct* pStruct, long long value, char* sLabel)
#endif
{
    union { unsigned int nOff; unsigned long long v; } tmp;
    volatile long long nData __attribute__((unused)) = value;   // never read; the store is intentional
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 7);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 7));
        if (pField != 0) {
#endif
            unsigned long long* pData = static_cast<unsigned long long*>(AddDataField(NEW_DATA_OFFSET, 0, 8));
            if (pData != 0) {
                tmp.nOff = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp.nOff);
                pField->nDataOrDataOffset = tmp.nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                tmp.v = static_cast<unsigned long long>(value);
                ::ByteSwap(&tmp.v);
                nData = static_cast<long long>(tmp.v);
                *pData = tmp.v;
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldDOUBLE(CResStruct* pStruct, double value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 9);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 9));
        if (pField != 0) {
#endif
            unsigned long long* pData = static_cast<unsigned long long*>(AddDataField(NEW_DATA_OFFSET, 0, 8));
            if (pData != 0) {
                union { unsigned int nOff; unsigned long long v; double d; } tmp;
                tmp.nOff = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp.nOff);
                pField->nDataOrDataOffset = tmp.nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                tmp.d = value;
                ::ByteSwap(&tmp.v);
                *pData = tmp.v;
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldCResRef(CResStruct* pStruct, const CResRef& value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_CRESREF);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_CRESREF));
        if (pField != 0) {
#endif
            unsigned char nLen = value.GetLength();
            unsigned char* pData = static_cast<unsigned char*>(AddDataField(NEW_DATA_OFFSET, 0, nLen + 1));
            if (pData != 0) {
                unsigned int nOff = static_cast<unsigned int>(pData - m_pFieldData);  // raw-offset: GFF file bytes
                ::ByteSwap(&nOff);
                pField->nDataOrDataOffset = nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                *pData = nLen;
                const unsigned char* pSrc = reinterpret_cast<const unsigned char*>(value.GetResRef());
                unsigned char* pDst = pData + 1;
                for (unsigned int i = 0; i < nLen; i++) {
                    unsigned char c = pSrc[i];
                    if (static_cast<unsigned char>(c - 'A') < 26) {
                        c = c + ('a' - 'A');
                    }
                    pDst[i] = c;
                }
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldCExoString(CResStruct* pStruct, const CExoString& value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, 10);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, 10));
        if (pField != 0) {
#endif
#if KOTOR1
            unsigned int nLen = static_cast<unsigned int>(value.GetLength());
#else
            unsigned int nLen = value.m_sString != 0 ? static_cast<unsigned int>(strlen(value.m_sString)) : 0;
#endif
            unsigned int* pData = static_cast<unsigned int*>(AddDataField(NEW_DATA_OFFSET, 0, nLen + 4));
            if (pData != 0) {
                uint64_t nCopyLen = nLen;
                unsigned int tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp);
                pField->nDataOrDataOffset = tmp;
#if KOTOR1
                pFieldRef.Set(field);
                // K1: the length prefix is written for an empty string too.
                tmp = nLen;
                ::ByteSwap(&tmp);
                *pData = tmp;
#else
                if (nLen != 0) {
                    tmp = nLen;
                    ::ByteSwap(&tmp);
                    *pData = tmp;
                }
#ifdef K2_FIX_ENGINE_BUGS
                // An empty string's 0 length is never written and AddDataField's buffer is not
                // zeroed, so the field reads back as garbage.
                else {
                    *pData = 0;
                }
#endif
#endif
                memcpy(pData + 1, value.CStr(), nCopyLen);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                return 1;
            }
        }
    }
    return 0;
}

#if KOTOR1
// K1: the length prefix is written unconditionally, every header word goes through the
// aligned-pointer store, and a string's length is CExoString::GetLength().
int CResGFF::WriteFieldCExoLocString(CResStruct* pStruct, const CExoLocString& value, char* sLabel)
{
    CExoString str;
    int bSuccess = 0;
    unsigned int nTotal;
    unsigned int* pData;
    unsigned int tmp;
    int nLang;
    unsigned char nGender;
    unsigned int nLen;
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_CEXOLOCSTRING);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            nTotal = 8;
            for (unsigned int i = 0; i < static_cast<unsigned int>(value.GetStringCount()); i++) {
                nTotal = nTotal + static_cast<unsigned int>(value.GetStringLength(i)) + 8;
            }
            pData = static_cast<unsigned int*>(AddDataField(NEW_DATA_OFFSET, 0, nTotal + 4));
            if (pData != 0) {
                tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp);
                field.nDataOrDataOffset = tmp;
                pFieldRef.Set(field);
                tmp = nTotal;
                ::ByteSwap(&tmp);
                CResGFFAlignedPtr<unsigned int>(pData).Set(tmp);
                tmp = static_cast<unsigned int>(value.m_nStrRef);
                ::ByteSwap(&tmp);
                CResGFFAlignedPtr<unsigned int>(pData + 1).Set(tmp);
                tmp = static_cast<unsigned int>(value.GetStringCount());
                ::ByteSwap(&tmp);
                bSuccess = 1;
                CResGFFAlignedPtr<unsigned int>(pData + 2).Set(tmp);
                unsigned int* pFirstRecord = pData + 3;
                unsigned char* p = reinterpret_cast<unsigned char*>(pFirstRecord);
                for (unsigned int i = 0; i < static_cast<unsigned int>(value.GetStringCount()); i++) {
                    if (value.GetString(i, &nLang, &str, &nGender)) {
                        nLen = str.GetLength();
                    } else {
                        nLen = 0;
                        bSuccess = 0;
                        nLang = -1;
                        nGender = 0;
                    }
                    tmp = static_cast<unsigned int>(nLang * 2) + static_cast<unsigned int>(nGender);
                    ::ByteSwap(&tmp);
                    CResGFFAlignedPtr<unsigned int>(reinterpret_cast<unsigned int*>(p)).Set(tmp);
                    tmp = nLen;
                    ::ByteSwap(&tmp);
                    CResGFFAlignedPtr<unsigned int>(reinterpret_cast<unsigned int*>(p) + 1).Set(tmp);  // idiom-exempt: GFF field-data bytes (a substring's length word)
                    memcpy(p + 8, str.CStr(), nLen);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                    p = p + 8 + nLen;
                }
            }
        }
    }
    return bSuccess;
}
#else
int CResGFF::WriteFieldCExoLocString(CResStruct* pStruct, const CExoLocString& value, char* sLabel)
{
    CExoString str;
    int bSuccess = 0;
    CResGFFField* pField;
    unsigned int nTotal;
    unsigned int* pData;
    unsigned int tmp;
    int nLang;
    unsigned char nGender;
    unsigned int nLen;
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
        pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_CEXOLOCSTRING));
        if (pField != 0) {
            nTotal = 8;
            for (unsigned int i = 0; i < static_cast<unsigned int>(value.GetStringCount()); i++) {
                nTotal = nTotal + static_cast<unsigned int>(value.GetStringLength(i)) + 8;
            }
            pData = static_cast<unsigned int*>(AddDataField(NEW_DATA_OFFSET, 0, nTotal + 4));
            if (pData != 0) {
                tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);
                ::ByteSwap(&tmp);
                pField->nDataOrDataOffset = tmp;
                if (nTotal != 0) {
                    tmp = nTotal;
                    ::ByteSwap(&tmp);
                    *pData = tmp;
                }
                tmp = static_cast<unsigned int>(value.m_nStrRef);
                ::ByteSwap(&tmp);
                pData[1] = tmp;
                tmp = static_cast<unsigned int>(value.GetStringCount());
                ::ByteSwap(&tmp);
                bSuccess = 1;
                pData[2] = tmp;
                unsigned int* pFirstRecord = pData + 3;
                unsigned char* p = reinterpret_cast<unsigned char*>(pFirstRecord);
                for (unsigned int i = 0; i < static_cast<unsigned int>(value.GetStringCount()); i++) {
                    nLen = 0;
                    if (value.GetString(i, &nLang, &str, &nGender)) {
                        if (str.m_sString != 0) {
                            nLen = static_cast<unsigned int>(strlen(str.m_sString));
                        }
                    } else {
                        nLen = 0;
                        bSuccess = 0;
                        nLang = -1;
                        nGender = 0;
                    }
                    tmp = static_cast<unsigned int>(nLang * 2) + static_cast<unsigned int>(nGender);
                    ::ByteSwap(&tmp);
                    reinterpret_cast<unsigned int*>(p)[0] = tmp;
                    tmp = nLen;
                    ::ByteSwap(&tmp);
                    reinterpret_cast<unsigned int*>(p)[1] = tmp;  // idiom-exempt: GFF field-data bytes (a substring's length word)
                    memcpy(p + 8, str.CStr(), nLen);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                    p = p + 8 + nLen;
                }
            }
        }
    }
    return bSuccess;
}
#endif

int CResGFF::WriteFieldVOID(CResStruct* pStruct, const void* pData, unsigned int nBytes, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_VOID);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_VOID));
        if (pField != 0) {
#endif
            unsigned int* pDest = static_cast<unsigned int*>(AddDataField(NEW_DATA_OFFSET, 0, nBytes + 4));
            if (pDest != 0) {
                uint64_t nLen = nBytes;
                unsigned int tmp = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pDest) - m_pFieldData);
                ::ByteSwap(&tmp);
                pField->nDataOrDataOffset = tmp;
#if KOTOR1
                pFieldRef.Set(field);
                // K1: the length prefix is written for an empty field too.
                tmp = nBytes;
                ::ByteSwap(&tmp);
                *pDest = tmp;
#else
                if (nBytes != 0) {
                    tmp = nBytes;
                    ::ByteSwap(&tmp);
                    *pDest = tmp;
                }
#endif
                memcpy(pDest + 1, pData, nLen);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldQuaternion(CResStruct* pStruct, const Quaternion& value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_QUATERNION);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_QUATERNION));
        if (pField != 0) {
#endif
            Quaternion* pData = static_cast<Quaternion*>(AddDataField(NEW_DATA_OFFSET, 0, sizeof(Quaternion)));
            if (pData != 0) {
                unsigned int nOff;
                Quaternion tmp = value;
                ::ByteSwap(&tmp.w);
                ::ByteSwap(&tmp.x);
                ::ByteSwap(&tmp.y);
                ::ByteSwap(&tmp.z);
                nOff = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);  // raw-offset: GFF file bytes
                ::ByteSwap(&nOff);
                pField->nDataOrDataOffset = nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                *pData = tmp;
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteFieldVector(CResStruct* pStruct, const Vector& value, char* sLabel)
{
    if (pStruct != 0 && sLabel != 0) {
        if (m_bResourceBacked != 0) {
            InitializeForWriting();
        }
#if KOTOR1
        CResGFFAlignedPtr<CResGFFField> pFieldRef = AddField(pStruct, sLabel, FIELD_VECTOR);
        if (pFieldRef.m_p != 0) {
            CResGFFField field = pFieldRef.Get();
            CResGFFField* pField = &field;
#else
        CResGFFField* pField = static_cast<CResGFFField*>(AddField(pStruct, sLabel, FIELD_VECTOR));
        if (pField != 0) {
#endif
            Vector* pData = static_cast<Vector*>(AddDataField(NEW_DATA_OFFSET, 0, sizeof(Vector)));
            if (pData != 0) {
                unsigned int nOff;
                Vector tmp = value;
                ::ByteSwap(&tmp.x);
                ::ByteSwap(&tmp.y);
                ::ByteSwap(&tmp.z);
                nOff = static_cast<unsigned int>(reinterpret_cast<unsigned char*>(pData) - m_pFieldData);  // raw-offset: GFF file bytes
                ::ByteSwap(&nOff);
                pField->nDataOrDataOffset = nOff;
#if KOTOR1
                pFieldRef.Set(field);
#endif
                *pData = tmp;
                return 1;
            }
        }
    }
    return 0;
}

int CResGFF::WriteGFFData(CExoFile& file, unsigned int& nSize)
{
    if (m_pHeader == 0) {
        return 0;
    }
    nSize = GFF_HEADER_SIZE;
    unsigned int tmp = GFF_HEADER_SIZE;
    ::ByteSwap(&tmp);
    m_pHeader->nStructOffset = tmp;

    unsigned int nNew;
    tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_RECORD_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldOffset = tmp;

    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_RECORD_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nLabelOffset = tmp;

    tmp = m_pHeader->nLabelCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_LABEL_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldDataOffset = tmp;

    tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    nNew = tmp + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldIndicesOffset = tmp;

    tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    nNew = tmp + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nListIndicesOffset = tmp;

    tmp = m_pHeader->nListIndicesCount;
    ::ByteSwap(&tmp);
    nSize = tmp + nSize;

    if (file.Write(m_pHeader, GFF_HEADER_SIZE, 1) != GFF_HEADER_SIZE) {
        return 0;
    }
    tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    unsigned int nBytes = tmp * GFF_RECORD_SIZE;
    if (file.Write(m_paStructs, nBytes, 1) != nBytes) {
        return 0;
    }
    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    nBytes = tmp * GFF_RECORD_SIZE;
    if (file.Write(m_paFields, nBytes, 1) != nBytes) {
        return 0;
    }
    tmp = m_pHeader->nLabelCount;
    ::ByteSwap(&tmp);
    nBytes = tmp * GFF_LABEL_SIZE;
    if (file.Write(m_paLabels, nBytes, 1) != nBytes) {
        return 0;
    }
    tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    nBytes = tmp;
    if (file.Write(m_pFieldData, nBytes, 1) != nBytes) {
        return 0;
    }
    tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    nBytes = tmp;
    if (file.Write(m_pFieldIndices, nBytes, 1) != nBytes) {
        return 0;
    }
    tmp = m_pHeader->nListIndicesCount;
    ::ByteSwap(&tmp);
    nBytes = tmp;
    return file.Write(m_pListIndices, nBytes, 1) == nBytes;
}

int CResGFF::WriteGFFFile(const CExoString& path, unsigned short resType)
{
    unsigned int nSize;
    CExoFile file(path, resType, CExoString("wb"));
    if (file.FileOpened() == 0) {
        return 0;
    }
    Pack(0, 0);
    return WriteGFFData(file, nSize);
}

int CResGFF::WriteGFFFile(char* pDest)
{
    if (m_pHeader == 0) {
        return 0;
    }
#if KOTOR1
    FILE* f = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exoresman/resgff.cpp",
                     "WriteGFFFile", 4300, pDest, "wb");
#else
    FILE* f = fopen(pDest, "wb");
#endif
    if (f == 0) {
        return 0;
    }
    Pack(0, 0);

    unsigned int nSize = GFF_HEADER_SIZE;
    unsigned int tmp = GFF_HEADER_SIZE;
    ::ByteSwap(&tmp);
    m_pHeader->nStructOffset = tmp;

    unsigned int nNew;
    tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_RECORD_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldOffset = tmp;

    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_RECORD_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nLabelOffset = tmp;

    tmp = m_pHeader->nLabelCount;
    ::ByteSwap(&tmp);
    nNew = tmp * GFF_LABEL_SIZE + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldDataOffset = tmp;

    tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    nNew = tmp + nSize;
    nSize = nNew;
    tmp = nNew;
    ::ByteSwap(&tmp);
    m_pHeader->nFieldIndicesOffset = tmp;

    tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    tmp = tmp + nSize;
    ::ByteSwap(&tmp);
    m_pHeader->nListIndicesOffset = tmp;

    if (fwrite(m_pHeader, GFF_HEADER_SIZE, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nStructCount;
    ::ByteSwap(&tmp);
    if (fwrite(m_paStructs, tmp * GFF_RECORD_SIZE, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nFieldCount;
    ::ByteSwap(&tmp);
    if (fwrite(m_paFields, tmp * GFF_RECORD_SIZE, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nLabelCount;
    ::ByteSwap(&tmp);
    unsigned int nLabelBytes = tmp << 4;
    if (fwrite(m_paLabels, nLabelBytes, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nFieldDataCount;
    ::ByteSwap(&tmp);
    if (fwrite(m_pFieldData, tmp, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nFieldIndicesCount;
    ::ByteSwap(&tmp);
    if (fwrite(m_pFieldIndices, tmp, 1, f) != 1) {
        return 0;
    }
    tmp = m_pHeader->nListIndicesCount;
    ::ByteSwap(&tmp);
    if (fwrite(m_pListIndices, tmp, 1, f) != 1) {
        return 0;
    }
    fclose(f);
    return 1;
}

int CResGFF::CreateGFFFile(CResStruct* pStruct, const CExoString& sType, const CExoString& sVersion)
{
    K2_UNUSED(sVersion);
    // sVersion is accepted but never read; the expected version string is the class constant "V3.2".
    CExoString sExpectedVersion(VERSION);
    unsigned int tag;
    int idx;
    if (pStruct == 0)
        return 0;
#if KOTOR1
    if (static_cast<unsigned int>(sType.GetLength()) < 4 || static_cast<unsigned int>(sExpectedVersion.GetLength()) < 4)
        return 0;
#else
    if (sType.m_sString == 0 || static_cast<unsigned int>(strlen(sType.m_sString)) < 4 ||
        sExpectedVersion.m_sString == 0 || static_cast<unsigned int>(strlen(sExpectedVersion.m_sString)) < 4)
        return 0;
#endif
    if (m_bResourceBacked != 0)
        ReleaseResource();
    InitializeForWriting();

    tag = static_cast<unsigned char>(sType[0]) | (static_cast<unsigned char>(sType[1]) << 8) |
                        (static_cast<unsigned char>(sType[2]) << 16) | (static_cast<unsigned int>(static_cast<unsigned char>(sType[3])) << 24);
    ::ByteSwap(&tag);
    *reinterpret_cast<unsigned int*>(m_pHeader->sFileType) = tag;  // raw-offset: GFF file bytes

    tag = static_cast<unsigned char>(sExpectedVersion[0]) | (static_cast<unsigned char>(sExpectedVersion[1]) << 8) |
          (static_cast<unsigned char>(sExpectedVersion[2]) << 16) |
          (static_cast<unsigned int>(static_cast<unsigned char>(sExpectedVersion[3])) << 24);
    ::ByteSwap(&tag);
    *reinterpret_cast<unsigned int*>(m_pHeader->sFileVersion) = tag;  // raw-offset: GFF file bytes

    idx = AddStruct(TOP_LEVEL_STRUCT_TYPE);
    pStruct->nStructIndex = idx;
    return idx != -1;
}

void CResGFF::Pack(unsigned char bWhat, unsigned int n)
{
    // Reclaims the orphaned FieldIndices/ListIndices bytes once the waste exceeds both a minimum
    // byte count and a percentage of the total size.
    if ((m_nFieldIndicesUsed != 0 || m_nListIndicesUsed != 0) && m_pHeader != 0 &&
        m_pFieldIndices != 0 && m_pListIndices != 0 && m_paStructs != 0 && m_paFields != 0)
    {
        unsigned int nTotalSize = GetTotalSize();
        unsigned int nUsed = m_nFieldIndicesUsed + m_nListIndicesUsed;
        if (nUsed > n && (nUsed * 100 / nTotalSize) > bWhat) {
            unsigned int tmp;
            if (m_nFieldIndicesUsed != 0) {
                tmp = m_pHeader->nFieldIndicesCount;
                ::ByteSwap(&tmp);
                if (tmp == m_nFieldIndicesUsed) {
                    m_pHeader->nFieldIndicesCount = 0;
                } else {
                    tmp = m_pHeader->nFieldIndicesCount;
                    ::ByteSwap(&tmp);
                    unsigned char* pNewFieldIndices = new unsigned char[tmp - m_nFieldIndicesUsed];

                    tmp = m_pHeader->nStructCount;
                    ::ByteSwap(&tmp);
                    unsigned int nNewFieldOffset = 0;
                    if (tmp != 0) {
#if KOTOR1
                        // K1: each struct record is copied out, edited and written back whole.
                        unsigned int i = 0;
                        for (;;) {
                            CResGFFStruct rec = (m_paStructs + i).Get();
                            tmp = rec.nFieldCount;
                            ::ByteSwap(&tmp);
                            if (tmp >= 2) {
                                tmp = rec.nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                unsigned int nOldOffset = tmp;
                                tmp = m_pHeader->nFieldIndicesCount;
                                ::ByteSwap(&tmp);
                                if (nOldOffset < tmp) {
                                    tmp = m_pHeader->nFieldIndicesCount;
                                    ::ByteSwap(&tmp);
                                    unsigned int nFieldIndicesCount = tmp;
                                    tmp = rec.nDataOrDataOffset;
                                    ::ByteSwap(&tmp);
                                    nOldOffset = tmp;
                                    unsigned int nRemaining = nFieldIndicesCount - nOldOffset;
                                    tmp = rec.nFieldCount;
                                    ::ByteSwap(&tmp);
                                    if (static_cast<uint64_t>(nRemaining) >= static_cast<uint64_t>(tmp) * 4) {
                                        unsigned char* pSrc = m_pFieldIndices;
                                        unsigned char* pDst = pNewFieldIndices + nNewFieldOffset;
                                        tmp = rec.nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        pSrc = pSrc + tmp;
                                        tmp = rec.nFieldCount;
                                        ::ByteSwap(&tmp);
                                        memcpy(pDst, pSrc, static_cast<uint64_t>(tmp) * 4);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                                        tmp = nNewFieldOffset;
                                        ::ByteSwap(&tmp);
                                        rec.nDataOrDataOffset = tmp;
                                        tmp = rec.nFieldCount;
                                        ::ByteSwap(&tmp);
                                        nNewFieldOffset += tmp * 4;
                                        (m_paStructs + i).Set(rec);
                                        goto next_struct;
                                    }
                                }
                                rec.nDataOrDataOffset = INVALID_INDEX;
                                rec.nFieldCount = 0;
                                (m_paStructs + i).Set(rec);
                            }
#else
                        unsigned int i = 0;
                        for (;;) {
                            tmp = m_paStructs[i].nFieldCount;
                            ::ByteSwap(&tmp);
                            if (tmp >= 2) {
                                tmp = m_paStructs[i].nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                unsigned int nOldOffset = tmp;
                                tmp = m_pHeader->nFieldIndicesCount;
                                ::ByteSwap(&tmp);
                                if (nOldOffset < tmp) {
                                    tmp = m_pHeader->nFieldIndicesCount;
                                    ::ByteSwap(&tmp);
                                    unsigned int nFieldIndicesCount = tmp;
                                    tmp = m_paStructs[i].nDataOrDataOffset;
                                    ::ByteSwap(&tmp);
                                    nOldOffset = tmp;
                                    unsigned int nRemaining = nFieldIndicesCount - nOldOffset;
                                    tmp = m_paStructs[i].nFieldCount;
                                    ::ByteSwap(&tmp);
                                    if (static_cast<uint64_t>(nRemaining) >= static_cast<uint64_t>(tmp) * 4) {
                                        unsigned char* pSrc = m_pFieldIndices;
                                        unsigned char* pDst = pNewFieldIndices + nNewFieldOffset;
                                        tmp = m_paStructs[i].nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        pSrc = pSrc + tmp;
                                        tmp = m_paStructs[i].nFieldCount;
                                        ::ByteSwap(&tmp);
                                        memcpy(pDst, pSrc, static_cast<uint64_t>(tmp) * 4);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                                        tmp = nNewFieldOffset;
                                        ::ByteSwap(&tmp);
                                        unsigned int nNewOffset = tmp;
                                        m_paStructs[i].nDataOrDataOffset = nNewOffset;
                                        tmp = m_paStructs[i].nFieldCount;
                                        ::ByteSwap(&tmp);
                                        nNewFieldOffset += tmp * 4;
                                        goto next_struct;
                                    }
                                }
                                m_paStructs[i].nDataOrDataOffset = INVALID_INDEX;
                                m_paStructs[i].nFieldCount = 0;
                            }
#endif
                            next_struct:
                            i++;
                            tmp = m_pHeader->nStructCount;
                            ::ByteSwap(&tmp);
                            if (i >= tmp) {
                                break;
                            }
                        }
                    }
                    delete[] m_pFieldIndices;
                    m_pFieldIndices = pNewFieldIndices;
                    tmp = nNewFieldOffset;
                    ::ByteSwap(&tmp);
                    m_pHeader->nFieldIndicesCount = tmp;
                }
                m_nFieldIndicesUsed = 0;
            }

            if (m_nListIndicesUsed != 0) {
                tmp = m_pHeader->nListIndicesCount;
                ::ByteSwap(&tmp);
                if (tmp == m_nListIndicesUsed) {
                    m_pHeader->nListIndicesCount = 0;
                } else {
                    tmp = m_pHeader->nListIndicesCount;
                    ::ByteSwap(&tmp);
                    unsigned char* pNewListIndices = new unsigned char[tmp - m_nListIndicesUsed];

                    tmp = m_pHeader->nFieldCount;
                    ::ByteSwap(&tmp);
                    unsigned int nNewListOffset = 0;
                    if (tmp != 0) {
#if KOTOR1
                        unsigned int i = 0;
                        for (;;) {
                            CResGFFField field = (m_paFields + i).Get();
                            tmp = field.nType;
                            ::ByteSwap(&tmp);
                            if (tmp == FIELD_LIST) {
                                tmp = field.nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                unsigned int nOldOffset = tmp;
                                tmp = m_pHeader->nListIndicesCount;
                                ::ByteSwap(&tmp);
                                if (nOldOffset < tmp) {
                                    tmp = m_pHeader->nListIndicesCount;
                                    ::ByteSwap(&tmp);
                                    unsigned int nListIndicesCount = tmp;
                                    tmp = field.nDataOrDataOffset;
                                    ::ByteSwap(&tmp);
                                    nOldOffset = tmp;
                                    unsigned int nRemaining = nListIndicesCount - nOldOffset;
                                    if (nRemaining >= 4) {
                                        unsigned char* pCountBase = m_pListIndices;
                                        tmp = field.nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        nOldOffset = tmp;
                                        tmp = *reinterpret_cast<unsigned int*>(pCountBase + nOldOffset);
                                        ::ByteSwap(&tmp);
                                        unsigned int nCount4 = tmp * 4;
                                        uint64_t nElemBytes = nCount4;
                                        tmp = m_pHeader->nListIndicesCount;
                                        ::ByteSwap(&tmp);
                                        nListIndicesCount = tmp;
                                        tmp = field.nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        nOldOffset = tmp;
                                        nElemBytes += 4;
                                        nRemaining = nListIndicesCount - nOldOffset;
                                        if (nElemBytes <= nRemaining) {
                                            unsigned char* pSrc = m_pListIndices;
                                            unsigned char* pDst = pNewListIndices + nNewListOffset;
                                            tmp = field.nDataOrDataOffset;
                                            ::ByteSwap(&tmp);
                                            pSrc = pSrc + tmp;
                                            memcpy(pDst, pSrc, nElemBytes);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                                            tmp = nNewListOffset;
                                            ::ByteSwap(&tmp);
                                            field.nDataOrDataOffset = tmp;
                                            nNewListOffset += static_cast<unsigned int>(nElemBytes);
                                            (m_paFields + i).Set(field);
                                            goto next_field;
                                        }
                                    }
                                }
                                field.nDataOrDataOffset = INVALID_INDEX;
                                (m_paFields + i).Set(field);
                            }
#else
                        unsigned int i = 0;
                        for (;;) {
                            tmp = m_paFields[i].nType;
                            ::ByteSwap(&tmp);
                            if (tmp == FIELD_LIST) {
                                tmp = m_paFields[i].nDataOrDataOffset;
                                ::ByteSwap(&tmp);
                                unsigned int nOldOffset = tmp;
                                tmp = m_pHeader->nListIndicesCount;
                                ::ByteSwap(&tmp);
                                if (nOldOffset < tmp) {
                                    tmp = m_pHeader->nListIndicesCount;
                                    ::ByteSwap(&tmp);
                                    unsigned int nListIndicesCount = tmp;
                                    tmp = m_paFields[i].nDataOrDataOffset;
                                    ::ByteSwap(&tmp);
                                    nOldOffset = tmp;
                                    unsigned int nRemaining = nListIndicesCount - nOldOffset;
                                    if (nRemaining >= 4) {
                                        unsigned char* pCountBase = m_pListIndices;
                                        tmp = m_paFields[i].nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        nOldOffset = tmp;
                                        tmp = *reinterpret_cast<unsigned int*>(pCountBase + nOldOffset);
                                        ::ByteSwap(&tmp);
                                        unsigned int nCount4 = tmp * 4;
                                        uint64_t nElemBytes = nCount4;
                                        tmp = m_pHeader->nListIndicesCount;
                                        ::ByteSwap(&tmp);
                                        nListIndicesCount = tmp;
                                        tmp = m_paFields[i].nDataOrDataOffset;
                                        ::ByteSwap(&tmp);
                                        nOldOffset = tmp;
                                        nElemBytes += 4;
                                        nRemaining = nListIndicesCount - nOldOffset;
                                        if (nElemBytes <= nRemaining) {
                                            unsigned char* pSrc = m_pListIndices;
                                            unsigned char* pDst = pNewListIndices + nNewListOffset;
                                            tmp = m_paFields[i].nDataOrDataOffset;
                                            ::ByteSwap(&tmp);
                                            pSrc = pSrc + tmp;
                                            memcpy(pDst, pSrc, nElemBytes);  // idiom-exempt: runtime length (file/buffer-derived, no compile-time size)
                                            tmp = nNewListOffset;
                                            ::ByteSwap(&tmp);
                                            m_paFields[i].nDataOrDataOffset = tmp;
                                            nNewListOffset += static_cast<unsigned int>(nElemBytes);
                                            goto next_field;
                                        }
                                    }
                                }
                                m_paFields[i].nDataOrDataOffset = INVALID_INDEX;
                            }
#endif
                            next_field:
                            i++;
                            tmp = m_pHeader->nFieldCount;
                            ::ByteSwap(&tmp);
                            if (i >= tmp) {
                                break;
                            }
                        }
                    }
                    delete[] m_pListIndices;
                    m_pListIndices = pNewListIndices;
                    tmp = nNewListOffset;
                    ::ByteSwap(&tmp);
                    m_pHeader->nListIndicesCount = tmp;
                }
                m_nListIndicesUsed = 0;
            }
        }
    }
}
