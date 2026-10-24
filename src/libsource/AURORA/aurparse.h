// The AURORA text-field parser's shared surface.

#pragma once
#include "k2stdint.h"

// bionic spells this strcasecmp.
#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"

extern bool disablepostprocessing;

// Translation units outside the parser band that still need these declarations define this, and get
// plain declarations instead of the local definitions.
#ifdef AURPARSE_HELPERS_OUT_OF_LINE
char* firstword(char* pszLine);
char* skipspace(char* pszLine);
#else
// Local copy of the parser's word-cursor helper; the body must stay identical to the one in
// aurparse.cpp.
static inline char* firstword(char* pszLine)
{
    static char buf[64];
    int nLen = 0;
    if (pszLine) {
        while (pszLine[nLen] && pszLine[nLen] != ' ' && pszLine[nLen] != '\t'
               && pszLine[nLen] != '\n' && pszLine[nLen] != '\r') {
            nLen++;
            buf[nLen - 1] = pszLine[nLen - 1];
        }
    }
    buf[nLen] = 0;
    return buf;
}

static inline char* skipspace(char* pszLine)
{
    if (pszLine) {
        while (*pszLine == ' ' || *pszLine == '\t')
            pszLine++;
    }
    return pszLine;
}

class AABB;
class MaxFace;
class MaxFace_PostProcess;
class NewController;
struct SkinVertexWeight;
template <class T> class List;
struct Vector;
struct Plane;
struct Quaternion;

struct AurGenericKey;

void Parse(char* pszLine, Vector& vValue);
void Parse(char* pszLine, Plane& pValue);
void Parse(char* pszLine, char* pszValue);
void Parse(char* pszLine, float& fValue);
void Parse(char* pszLine, int& nValue);
void Parse(char* pszLine, unsigned int& nValue);
void Parse(char* pszLine, short& nValue);
void Parse(char* pszLine, unsigned char& nValue);
void Parse(char* pszLine, bool& bValue);
void Parse(char* pszLine, Quaternion& qValue);
void Parse(char* pszLine, AurGenericKey& key);
void Parse(char* pszLine, AABB*& pValue);
void Parse(char* pszLine, MaxFace& value);
void Parse(char* pszLine, MaxFace_PostProcess& value);
void Parse(char* pszLine, SkinVertexWeight& value);
// The array is filled through the pointer; the pointer passed in is never advanced.
void Parse(char* pszLine, unsigned int* value);
void Parse(char* pszLine, int* value);
template <class T> void Parse(char* pszLine, List<T>& list);
// Declared without a body on purpose: the explicit instantiations are exported, so no body is
// needed for the element types the node parsers reach.
extern template void Parse<Vector>(char*, List<Vector>&);
extern template void Parse<char*>(char*, List<char*>&);
extern template void Parse<int>(char*, List<int>&);
extern template void Parse<unsigned int>(char*, List<unsigned int>&);
extern template void Parse<float>(char*, List<float>&);
extern template void Parse<Quaternion>(char*, List<Quaternion>&);
extern template void Parse<SkinVertexWeight>(char*, List<SkinVertexWeight>&);
extern template void Parse<MaxFace_PostProcess>(char*, List<MaxFace_PostProcess>&);
extern template void Parse<unsigned int*>(char*, List<unsigned int*>&);
void AddGenericControllerSingle(char* pszName, char* pszType, List<NewController>& controllers,
                                List<float>& floats, void* pA, void* pB);

// Both of these are no-ops; leave them empty.
inline void AddGenericController(char* pszName, char* pszType, List<NewController>& controllers,
                                 List<float>& floats, void* pA, void* pB) { K2_UNUSED(pszName); K2_UNUSED(pszType); K2_UNUSED(controllers); K2_UNUSED(floats); K2_UNUSED(pA); K2_UNUSED(pB); }
inline void AddBezierController(char* pszName, char* pszType, List<NewController>& controllers,
                                List<float>& floats, void* pA, void* pB) { K2_UNUSED(pszName); K2_UNUSED(pszType); K2_UNUSED(controllers); K2_UNUSED(floats); K2_UNUSED(pA); K2_UNUSED(pB); }
#endif  // AURPARSE_HELPERS_OUT_OF_LINE

class MdlNode;
class MaxTree;
MdlNode* ParseNode(char* pszLine, MaxTree* pTree);
MdlNode* FindMdlNode(MdlNode* pNode, char* pszName);
MdlNode* FindMdlNode(MaxTree* pTree, char* pszName);
