#ifndef CAURPATHINFO_H
#define CAURPATHINFO_H

class CAurPathInfo {
public:
    virtual ~CAurPathInfo() {}

    CAurPathInfo();

    // The count is in floats, not points: RenderPaths walks `m_nPathFloats / 3` vertices.
    int    m_nPathFloats;
    float* m_pPathFloats;
};

#endif // CAURPATHINFO_H
