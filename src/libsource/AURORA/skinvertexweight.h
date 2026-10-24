#ifndef SKINVERTEXWEIGHT_H
#define SKINVERTEXWEIGHT_H

// One skinned vertex's up to four bone influences: the bone names as parsed, their weights and the
// resolved bone indices.
struct SkinVertexWeight {
    char* m_paszName0;
    char* m_paszName1;
    char* m_paszName2;
    char* m_paszName3;
    float m_fWeight0;
    float m_fWeight1;
    float m_fWeight2;
    float m_fWeight3;
    int   m_nBoneIdx0;
    int   m_nBoneIdx1;
    int   m_nBoneIdx2;
    int   m_nBoneIdx3;
    int   m_nWeightCount;
    int   m_nReserved1;
};
K2_LAYOUT_ASSERT(sizeof(SkinVertexWeight) == 0x48, "SkinVertexWeight must stay 0x48");

#endif // SKINVERTEXWEIGHT_H
