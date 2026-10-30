#include "k2stdint.h"
#include "aurres.h"
#include "port/libcbase.h"
#include <string.h>
#include "port/libcmath.h"
#include "port/crtapi.h"

// Declare both face destructors without bodies so List<T>::allocate keeps its per-element delete
// loop; a host build takes the header bodies instead.
#ifndef K2_HOST_BUILD
#define MAXFACE_DTOR_DECLARED_ONLY
#define MAXFACE_PP_DTOR_DECLARED_ONLY
#endif

#if KOTOR1
#define K1_AURORA_LIST  // K1's List grows from a different initial capacity
#endif
#include "Vector.h"
#include "Plane.h"
#include "Quaternion.h"
#include "nullname.h"

char* FindSpace(char* pszLine)
{
    for (;; pszLine++) {
        char c = *pszLine;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            return pszLine;
        if (c == 0)
            return 0;
    }
}

char* skipspace(char* pszLine)
{
    if (pszLine) {
        while (*pszLine == ' ' || *pszLine == '\t')
            pszLine++;
    }
    return pszLine;
}

// The token is copied into a shared static buffer, so the result is valid only until the next call.
char* firstword(char* pszLine)
{
    static char buf[64];
    int nLen = 0;
    if (pszLine) {
        while (pszLine[nLen] && pszLine[nLen] != ' ' && pszLine[nLen] != '\t' && pszLine[nLen] != '\n' && pszLine[nLen] != '\r') {
            nLen++;
            buf[nLen - 1] = pszLine[nLen - 1];
        }
    }
    buf[nLen] = 0;
    return buf;
}

void Parse(char* pszLine, Vector& vValue)
{
    static_cast<void>(sscanf(pszLine, "%f %f %f", &vValue.x, &vValue.y, &vValue.z));
}

// Reads four floats, then normalizes the (a,b,c) normal and scales d by the same magnitude.
void Parse(char* pszLine, Plane& pValue)
{
    static_cast<void>(sscanf(pszLine, "%f %f %f %f", &pValue.a, &pValue.b, &pValue.c, &pValue.d));
    float fMag = magnitude(*reinterpret_cast<Vector*>(&pValue));
    pValue.d = pValue.d / fMag;
    if (fMag != 0.0f) {
        float fInv = 1.0f / fMag;
        pValue.a = pValue.a * fInv;
        pValue.b = pValue.b * fInv;
        pValue.c = pValue.c * fInv;
    }
}

// On a failed conversion the destination is emptied rather than left alone.
void Parse(char* pszLine, char* pszValue)
{
    if (!sscanf(pszLine, "%s", pszValue))
        *pszValue = 0;
}

void Parse(char* pszLine, float& fValue)
{
    static_cast<void>(sscanf(pszLine, "%f", &fValue));
}

// A failed `%d` retries as a word and also accepts the true/false spellings.
void Parse(char* pszLine, int& nValue)
{
    if (!sscanf(pszLine, "%d", &nValue)) {
        char buf[64];
        buf[0] = 0;
        static_cast<void>(sscanf(pszLine, "%s", buf));
        if (!_stricmp("TRUE", buf))
            nValue = 1;
        if (!_stricmp("FALSE", buf))
            nValue = 0;
    }
}

// Same spellings as the int overload, parsed through an int scratch and stored unconditionally.
void Parse(char* pszLine, unsigned int& nValue)
{
    int nScratch;
    if (!sscanf(pszLine, "%d", &nScratch)) {
        char buf[64];
        buf[0] = 0;
        static_cast<void>(sscanf(pszLine, "%s", buf));
        if (!_stricmp("TRUE", buf))
            nScratch = 1;
        if (!_stricmp("FALSE", buf))
            nScratch = 0;
    }
    nValue = nScratch;
}

// `%d` needs an int destination, so the value is truncated on the store.
void Parse(char* pszLine, short& nValue)
{
    int nScratch;
    static_cast<void>(sscanf(pszLine, "%d", &nScratch));
    nValue = static_cast<short>(nScratch);
}

void Parse(char* pszLine, unsigned char& nValue)
{
    static_cast<void>(sscanf(pszLine, "%c", &nValue));
}

// Each spelling test is independent and never short-circuits; the false spellings come last so
// they win.
void Parse(char* pszLine, bool& bValue)
{
    char buf[8];
    buf[0] = 0;
    static_cast<void>(sscanf(pszLine, "%s", buf));
    if (!_stricmp("true", buf))
        bValue = true;
    if (!_stricmp("false", buf))
        bValue = false;
    if (!_stricmp("1", buf))
        bValue = true;
    if (!_stricmp("0", buf))
        bValue = false;
}

// A zero axis means no rotation, so the identity quaternion is stored instead.
void Parse(char* pszLine, Quaternion& qValue)
{
    Vector vAxis;
    float fAngle;
    static_cast<void>(sscanf(pszLine, "%f %f %f %f", &vAxis.x, &vAxis.y, &vAxis.z, &fAngle));
    if (vAxis.x == 0.0f && vAxis.y == 0.0f && vAxis.z == 0.0f)
        qValue = Quaternion();
    else
        qValue = Quaternion(vAxis, fAngle);
}

// Reads the bounding-box mesh tree. One line gives the box's six bounds plus two ints; when the
// index is -1 the next two lines each recurse into a child pointer.
//
// The 0.01f inflation below touches the bounds locals, not the box just built from them, so that
// store is dead.
#define AABB_DTOR_DECLARED_ONLY noexcept(false)
#include "PointerMap.h"

// Returns a six-bit octant code, where bits 0-2 are +X/+Y/+Z and bits 3-5 are -X/-Y/-Z. The
// dominant axis has the largest absolute centre delta; ties fall to the last axis tested, so equal
// magnitudes pick Z and then Y.
static int AABBDirection(const AABB* pChild0, const AABB* pChild1)
{
    float fDx = (pChild1->min.x + pChild1->max.x) - (pChild0->min.x + pChild0->max.x);
    float fDy = (pChild1->min.y + pChild1->max.y) - (pChild0->min.y + pChild0->max.y);
    float fDz = (pChild1->min.z + pChild1->max.z) - (pChild0->min.z + pChild0->max.z);
    if ((fDx < 0.0f ? -fDx : fDx) > (fDy < 0.0f ? -fDy : fDy)
        && (fDx < 0.0f ? -fDx : fDx) > (fDz < 0.0f ? -fDz : fDz))
        return fDx > 0.0f ? AABB::kDirPosX : AABB::kDirNegX;
    if ((fDy < 0.0f ? -fDy : fDy) > (fDz < 0.0f ? -fDz : fDz))
        return fDy > 0.0f ? AABB::kDirPosY : AABB::kDirNegY;
    return fDz > 0.0f ? AABB::kDirPosZ : AABB::kDirNegZ;
}

void Parse(char* pszLine, AABB*& pValue)
{
    pszLine = skipspace(pszLine);
    Vector vMin, vMax;
    int nIndex, nDir;
    int nRead = sscanf(pszLine, "%f %f %f %f %f %f %d %d", &vMin.x, &vMin.y, &vMin.z,
                       &vMax.x, &vMax.y, &vMax.z, &nIndex, &nDir);
    pValue = new AABB();
    pValue->min = vMin;
    pValue->pChild0 = 0;
    pValue->pChild1 = 0;
    pValue->m_nIndex = nIndex;
    pValue->m_nDir = 0;
    pValue->max = vMax;
    if (nRead == 7) {
        vMin.x -= 0.01f;
        vMin.y -= 0.01f;
        vMin.z -= 0.01f;
        vMax.x += 0.01f;
        vMax.y += 0.01f;
        vMax.z += 0.01f;
    }
    if (nIndex == -1) {
        Parse(AurResGetNextLine(), pValue->pChild0);
        Parse(AurResGetNextLine(), pValue->pChild1);
        if (nRead == 7)
            pValue->m_nDir = AABBDirection(pValue->pChild0, pValue->pChild1);
        else
            pValue->m_nDir = nDir;
    }
}

// ---- the empty rows and the tree walker --------------------------------------------------------
template <class T> class List;
class NewController;
class MdlNode;
// One parsed numeric key line from a .txi or .mdl file: ten value slots plus the parser's scan count.
struct AurGenericKey {
    float m_afValues[10];
    // sscanf's return value; five selects the axis-angle interpretation.
    int   m_nCount;
};
#include "AURORA/aurmodel.h"

void AddGenericController(char* pszName, char* pszType, List<NewController>& controllers,
                          List<float>& floats, void* pA, void* pB)
{
    K2_UNUSED(pszName); K2_UNUSED(pszType); K2_UNUSED(controllers); K2_UNUSED(floats); K2_UNUSED(pA); K2_UNUSED(pB);
}

void AddBezierController(char* pszName, char* pszType, List<NewController>& controllers,
                         List<float>& floats, void* pA, void* pB)
{
    K2_UNUSED(pszName); K2_UNUSED(pszType); K2_UNUSED(controllers); K2_UNUSED(floats); K2_UNUSED(pA); K2_UNUSED(pB);
}

void Parse(char* pszLine, MaxFace& value)
{
    K2_UNUSED(pszLine); K2_UNUSED(value);
}

MdlNode* FindMdlNode(MdlNode* pNode, char* pszName);

MdlNode* FindMdlNode(MdlNode* pNode, char* pszName)
{
    if (!pNode)
        return 0;

    if (!_stricmp(pszName, pNode->TranslateNodeName()))
        return pNode;

    int nCount = pNode->m_listChildren.m_count;
    for (int i = 0; i < nCount; i++) {
        MdlNode* pFound = FindMdlNode(pNode->m_listChildren.m_pData[i], pszName);
        if (pFound)
            return pFound;
    }
    return 0;
}

MdlNode* FindMdlNode(MaxTree* pTree, char* pszName)
{
    if (pTree)
        return FindMdlNode(pTree->m_pMdlNode, pszName);
    return 0;
}

// ---- the "one count line, then N value lines" readers -------------------------------------------

void Parse(char* pszLine, unsigned int* value)
{
    int nCount = 0;
    static_cast<void>(sscanf(pszLine, "%d", &nCount));
    if (nCount >= 1) {
        for (int64_t i = 0; i < nCount; i++) {
            static_cast<void>(sscanf(AurResGetNextLine(), "%d", reinterpret_cast<int*>(value)));
            value++;
        }
    }
}

// The `int` twin of the overload above.
void Parse(char* pszLine, int* value)
{
    int nCount = 0;
    static_cast<void>(sscanf(pszLine, "%d", &nCount));
    if (nCount >= 1) {
        for (int64_t i = 0; i < nCount; i++) {
            static_cast<void>(sscanf(AurResGetNextLine(), "%d", value));
            value++;
        }
    }
}

// Reads a trimesh face. It frees the three owned buffers, re-allocates them at the sizes a face
// needs, and reads eight ints.
void Parse(char* pszLine, MaxFace_PostProcess& value)
{
    if (value.m_paVertIndices)
        delete[] value.m_paVertIndices;
    if (value.m_paNormalIndices)
        delete[] value.m_paNormalIndices;
    if (value.m_paTVertIndices)
        delete[] value.m_paTVertIndices;
    int* pnTVertIndices = new int[12];
    value.m_paTVertIndices = pnTVertIndices;
    int* pnVertIndices = new int[3];
    value.m_paVertIndices = pnVertIndices;
    int* pnNormalIndices = new int[3];
    value.m_paNormalIndices = pnNormalIndices;
    static_cast<void>(sscanf(pszLine, "%d %d %d %d %d %d %d %d",
           pnVertIndices, pnVertIndices + 1, pnVertIndices + 2, &value.m_nSmoothGroup,
           pnTVertIndices, pnTVertIndices + 1, pnTVertIndices + 2, &value.m_nMaterial));
}

// ---- the bone-weight record --------------------------------------------------------------------


// Kept at external linkage so the constant format argument stays at the call sites.
int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nResult = __vsprintf_chk(pszBuf, 0, nSize, pszFmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

// Reads one record per weighted bone, with four 32-byte name buffers, the four weights, then the
// bone ids (seeded to -1).
void Parse(char* pszLine, SkinVertexWeight& value)
{
    value.m_paszName0 = new char[32];
    char* pszRoot = new char[32];
    value.m_paszName1 = pszRoot;
    value.m_paszName2 = new char[32];
    value.m_paszName3 = new char[32];
    FormatName(pszRoot, 32, "ROOT");
    value.m_fWeight1 = 0.0f;
    FormatName(value.m_paszName2, static_cast<uint64_t>(-1), "ROOT");
    value.m_fWeight2 = 0.0f;
    FormatName(value.m_paszName3, static_cast<uint64_t>(-1), "ROOT");
    value.m_fWeight3 = 0.0f;
    int nRead = sscanf(pszLine, "%s %f %s %f %s %f %s %f",
                       value.m_paszName0, &value.m_fWeight0,
                       value.m_paszName1, &value.m_fWeight1,
                       value.m_paszName2, &value.m_fWeight2,
                       value.m_paszName3, &value.m_fWeight3);
    value.m_nBoneIdx0 = -1;
    value.m_nBoneIdx1 = -1;
    value.m_nWeightCount = nRead / 2;
}

// Ten floats straight into the record; the scan count doubles as the value count. A count of five
// is the axis-plus-angle form: the first four values are re-interpreted as a rotation and written
// back over the same slots.
void Parse(char* pszLine, AurGenericKey& key)
{
    key.m_nCount = sscanf(pszLine, "%f %f %f %f %f %f %f %f %f %f",
                          &key.m_afValues[0], &key.m_afValues[1], &key.m_afValues[2],
                          &key.m_afValues[3], &key.m_afValues[4], &key.m_afValues[5],
                          &key.m_afValues[6], &key.m_afValues[7], &key.m_afValues[8],
                          &key.m_afValues[9]);
    if (key.m_nCount == 5) {
        static Vector v;
        v.x = key.m_afValues[1];
        v.y = key.m_afValues[2];
        v.z = key.m_afValues[3];
        Quaternion q(v, key.m_afValues[4]);
        key.m_afValues[1] = q.x;
        key.m_afValues[2] = q.y;
        key.m_afValues[3] = q.z;
        key.m_afValues[4] = q.w;
    }
}

// ---- the `List<T>` readers ----------------------------------------------------------------------
// Reads a counted element list from the .mdl text format. The explicit instantiations below cover
// every element type the format uses.
#include <string.h>
#include "List.h"
unsigned char* AurResGetData(int nBytes);

template <class T>
void Parse(char* pszLine, List<T>& list)
{
    int nCount = 0;
    char cType;
    int nScan = sscanf(pszLine, "%d %c", &nCount, &cType);
    if (nScan <= 0) {
        // One element per line until the terminator.
        pszLine = AurResGetNextLine();
        while (pszLine) {
            if (!strncmp(skipspace(pszLine), "endlist", strlen("endlist")))
                break;
            if (list.m_count != 0) {
                if (list.m_count == nCount)
                    list.Add(list.m_pData[list.m_count - 1]);
            } else {
                list.SetSize(1);
            }
            // The local copy is required; it keeps the boundary read ahead of the data load.
            int n = nCount;
            Parse(pszLine, list.m_pData[n]);
            nCount++;
            pszLine = AurResGetNextLine();
        }
        skipspace(skipspace(pszLine) + strlen("endlist"));
        return;
    }

    list.SetSize(nCount);
    if (nScan == 1) {
        // The count-only form reads one element per following line.
        for (int64_t i = 0; i < list.m_count; i++) {
            char* pszNext = AurResGetNextLine();
            if (!pszNext)
                break;
            Parse(pszNext, list.m_pData[i]);
        }
    } else {
        // In the packed-blob form the elements are the resource's raw bytes.
        int nBytes = nCount * sizeof(T);
        memcpy(static_cast<void*>(list.m_pData), AurResGetData(nBytes), nBytes);  // idiom-exempt: runtime length -- nCount * sizeof(T), hoisted into an int because the .so sign-extends that 32-bit value into memcpy's size_t (sxtw x2)
        pszLine[0] = 0;
    }
}

// Explicit instantiations for the element types above.
template void Parse<int>(char*, List<int>&);
template void Parse<MaxFace_PostProcess>(char*, List<MaxFace_PostProcess>&);
template void Parse<unsigned int>(char*, List<unsigned int>&);
template void Parse<SkinVertexWeight>(char*, List<SkinVertexWeight>&);
template void Parse<Quaternion>(char*, List<Quaternion>&);
#if KOTOR1  // K1 emits the float and Vector readers here; in K2 they live in other translation units
template void Parse<float>(char*, List<float>&);
template void Parse<Vector>(char*, List<Vector>&);
#endif

// The one explicit specialization in the family reads a list of int arrays, one per line, where
// each element is itself a count plus that many `%d` lines.
template <>
void Parse<unsigned int*>(char* pszLine, List<unsigned int*>& list)
{
    int nCount = 0;
    static_cast<void>(sscanf(pszLine, "%d", &nCount));
    list.SetSize(nCount);
    for (int64_t i = 0; i < list.m_count; i++) {
        char* pszNext = AurResGetNextLine();
        int n;
        static_cast<void>(sscanf(pszNext, "%d", &n));
        list.m_pData[i] = new unsigned int[n];
        Parse(pszNext, list.m_pData[i]);
    }
}

// A second list reader under the generic name, reading `"%d"` only, and each element is a freshly
// allocated name buffer. Modelled as an explicit specialization.
template <>
void Parse<char*>(char* pszLine, List<char*>& list)
{
    int nCount = 0;
    int nScan = sscanf(pszLine, "%d", &nCount);
    if (nScan <= 0) {
        pszLine = AurResGetNextLine();
        while (pszLine) {
            if (!strncmp(skipspace(pszLine), "endlist", strlen("endlist")))
                break;
            if (list.m_count != 0) {
                if (list.m_count == nCount)
                    list.Add(list.m_pData[list.m_count - 1]);
            } else {
                list.allocate(1);
                list.m_count = 1;
            }
            list.m_pData[nCount] = new char[128];
            int n = nCount;
            Parse(pszLine, list.m_pData[n]);
            nCount++;
            pszLine = AurResGetNextLine();
        }
        skipspace(skipspace(pszLine) + strlen("endlist"));
        return;
    }

    list.SetSize(nCount);
    for (int64_t i = 0; i < list.m_count; i++) {
        list.m_pData[i] = new char[128];
        Parse(AurResGetNextLine(), list.m_pData[i]);
    }
}

// ---- the AurGenericKey controller family --------------------------------------------------------
#include "auranim.h"
extern float cullvalueforkeys;

// True when two keys hold the same pose, that is when every one of the four axis/angle values
// differs by less than the engine threshold.
static inline bool KeysWithinThreshold(const AurGenericKey* pKeyA, const AurGenericKey* pKeyB,
                                       float fThreshold)
{
    return fabsf(pKeyA->m_afValues[1] - pKeyB->m_afValues[1]) < fThreshold
        && fabsf(pKeyA->m_afValues[2] - pKeyB->m_afValues[2]) < fThreshold
        && fabsf(pKeyA->m_afValues[3] - pKeyB->m_afValues[3]) < fThreshold
        && fabsf(pKeyA->m_afValues[4] - pKeyB->m_afValues[4]) < fThreshold;
}

// Walks the key list backwards and drops a key that sits within the threshold of both its
// neighbours, i.e. one that adds nothing to the straight line between them.
void CullUnnecessaryKeys(List<AurGenericKey>& list)
{
    float fThreshold = cullvalueforkeys;
    for (int i = list.m_count - 2; i > 1; i--) {
        if (KeysWithinThreshold(&list.m_pData[i], &list.m_pData[i - 1], fThreshold)
            && KeysWithinThreshold(&list.m_pData[i - 2], &list.m_pData[i - 1], fThreshold))
            list.DelIndex(i - 1);
    }
}

// Parses the body of every `node <kind> <name>` block. It reads the kind word, builds the matching
// MdlNode subclass, dispatches the block's lines to it until `endnode`, and registers the node's
// name with the owning model.
#include "auremitter.h"

// The node-kind post-process gate, defined in aurora.cpp.
extern bool disablepostprocessing;

// The animation arm's controller-list copy. List.h keeps `operator=` and `Add` declaration-only, so
// both bodies are defined here for this instantiation.
template <> inline void List<NewController>::Add(NewController item)
{
    if (m_count == m_capacity)
#if KOTOR1  // K1 grows the list from 8 on first allocation
        allocate(m_count ? m_count * 2 : static_cast<int>(K1ListFirstGrow<NewController>::value));
#else
        allocate(m_count ? m_count * 2 : 16);
#endif
    new (&m_pData[m_count++]) NewController(item);
}

template <> inline List<NewController>& List<NewController>::operator=(List<NewController>& rhs)
{
    m_count = 0;
    for (int i = 0; i < rhs.m_count; i++)
        Add(rhs.m_pData[i]);
    return *this;
}

// Packs the key list in place. Each key's quaternion collapses from four floats into one word (11
// bits x, 11 bits y, 10 bits z, each mapped from [-1,1] onto a fixed-point range) and the run is
// rewritten through a scratch list.
//
// The clamps are ternaries so the NaN-propagating minimum and maximum are selected, and the values
// are read as ints so the negation is a sign-bit flip.
#if KOTOR1
// K1 flips a negative-w key through Quaternion::operator* and hands the scratch list back in one
// assignment; K2 clears the list and re-adds element by element.
void CompressQuaternionKey(List<AurGenericKey>& list)
{
    List<AurGenericKey> compressed;
    Quaternion q;
    for (int64_t i = 0; i < list.m_count; i++) {
        AurGenericKey key;
        key.m_afValues[0] = list.m_pData[i].m_afValues[0];
        q.x = list.m_pData[i].m_afValues[1];
        q.y = list.m_pData[i].m_afValues[2];
        q.z = list.m_pData[i].m_afValues[3];
        q.w = list.m_pData[i].m_afValues[4];
        if (q.w < 0.0f)
            q = q * -1.0f;
        float fX = (q.x + 1.0f) * 2046.0f * 0.5f;
        fX = fX < 2046.0f ? fX : 2046.0f;
        fX = fX > 0.0f ? fX : 0.0f;
        float fY = (q.y + 1.0f) * 2046.0f * 0.5f;
        fY = fY < 2046.0f ? fY : 2046.0f;
        fY = fY > 0.0f ? fY : 0.0f;
        float fZ = (q.z + 1.0f) * 1022.0f * 0.5f;
        fZ = fZ < 1022.0f ? fZ : 1022.0f;
        fZ = fZ > 0.0f ? fZ : 0.0f;
        *reinterpret_cast<int*>(&key.m_afValues[1]) = static_cast<int>(fX) | (static_cast<int>(fY) << 11) | (static_cast<int>(fZ) << 22);  // raw-offset: model file key values: int bits in a float array
        key.m_nCount = 2;
        compressed.Add(key);
    }
    list = compressed;
}
#else
void CompressQuaternionKey(List<AurGenericKey>& list)
{
    List<AurGenericKey> compressed;
    for (int64_t i = 0; i < list.m_count; i++) {
        AurGenericKey key;
        key.m_afValues[0] = list.m_pData[i].m_afValues[0];
        int nX = *reinterpret_cast<int*>(&list.m_pData[i].m_afValues[1]);  // raw-offset: model file key values: int bits in a float array
        int nY = *reinterpret_cast<int*>(&list.m_pData[i].m_afValues[2]);  // raw-offset: model file key values: int bits in a float array
        int nZ = *reinterpret_cast<int*>(&list.m_pData[i].m_afValues[3]);  // raw-offset: model file key values: int bits in a float array
        if (list.m_pData[i].m_afValues[4] < 0.0f) {
            nX ^= static_cast<int>(0x80000000);
            nY ^= static_cast<int>(0x80000000);
            nZ ^= static_cast<int>(0x80000000);
        }
        float fX = *reinterpret_cast<float*>(&nX);
        float fY = *reinterpret_cast<float*>(&nY);
        float fZ = *reinterpret_cast<float*>(&nZ);
        fX = (fX + 1.0f) * 2046.0f * 0.5f;
        fY = (fY + 1.0f) * 2046.0f * 0.5f;
        fZ = (fZ + 1.0f) * 1022.0f * 0.5f;
        fX = fX < 2046.0f ? fX : 2046.0f;
        fY = fY < 2046.0f ? fY : 2046.0f;
        fZ = fZ < 1022.0f ? fZ : 1022.0f;
        fX = fX > 0.0f ? fX : 0.0f;
        fY = fY > 0.0f ? fY : 0.0f;
        fZ = fZ > 0.0f ? fZ : 0.0f;
        *reinterpret_cast<int*>(&key.m_afValues[1]) = static_cast<int>(fX) | (static_cast<int>(fY) << 11) | (static_cast<int>(fZ) << 22);  // raw-offset: model file key values: int bits in a float array
        key.m_nCount = 2;
        compressed.Add(key);
    }

    list.m_count = 0;
    for (int64_t j = 0; j < compressed.m_count; j++)
        list.Add(compressed.m_pData[j]);
}
#endif

// The kind word per key value count, one entry per count 1..4, already shifted into the upper half
// of the record word.
static const unsigned long long g_anControllerKinds[4] = {
    1ULL << 32, 0, 3ULL << 32, 4ULL << 32,
};

int countnewcontrollers;

// Packs a single-key controller. It turns one parsed key line into a controller record and appends
// the key's value run to the node's parallel float list.
void AddGenericControllerSingle(char* pszName, char* pszType, List<NewController>& controllers,
                                List<float>& floats, void* pOffset, void* pDefaultOffset)
{
    K2_UNUSED(pszName);
    AurGenericKey key;
    Parse(pszType, key);

    // A four-value line is an axis plus an angle, not a value run: build the quaternion and store it
    // back over the first four slots in x,y,z,w order (the shift below moves it to 1..4).
    if (key.m_nCount == 4) {
        static Vector v;
        v = Vector(key.m_afValues[0], key.m_afValues[1], key.m_afValues[2]);
        Quaternion q(v, key.m_afValues[3]);
        key.m_afValues[0] = q.x;
        key.m_afValues[1] = q.y;
        key.m_afValues[2] = q.z;
        key.m_afValues[3] = q.w;
    }

    int nCount = key.m_nCount;
    int nType = nCount - 1;
    for (int i = nType; i >= 0; i--)
        key.m_afValues[i + 1] = key.m_afValues[i];
    key.m_nCount = nCount + 1;
    key.m_afValues[0] = 0.0f;

    unsigned long long nKind = 0;
    if (static_cast<unsigned>(nType) < 4) {
        K2_ASSUME(nType >= 0);   // the unsigned test bounds both ends
        nKind = g_anControllerKinds[nType];
    }

    NewController c;
    c.m_nKeysPacked = nKind | static_cast<unsigned long long>(static_cast<unsigned short>(floats.m_count));
    floats.Add(0.0f);
    c.m_nKeysPacked |= static_cast<unsigned long long>(static_cast<unsigned short>(floats.m_count)) << 16;
    for (int i = 1; i < key.m_nCount; i++)
        floats.Add(key.m_afValues[i]);
    // The first word is built last, so the two pointer arguments do not stay live across the float
    // loop below.
    // The first word is built last, so the two pointer arguments do not stay live across the float
    // loop below.
    c.m_nTargetPacked = static_cast<unsigned long long>(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pOffset)))
              | ((static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(pDefaultOffset)) & 0xffff) << 32)
              | (1ULL << 48);
    controllers.Add(c);
    countnewcontrollers++;
}

#include "aurio.h"
#include "aurprimitive.h"

// A controller's target is the member's offset in its part, as GetOffset64 (aurmodel.cpp) names
// them.
#define K2_PART_OFFSET(C, m) static_cast<int64_t>(K2_OFFSETOF(C, m))

void AddGenericControllerSingle(char* pszName, char* pszType, List<NewController>& controllers,
                                List<float>& floats, void* pA, void* pB);

// The emitter node's field chain, the largest single function in the engine, with 30 scalar keys
// plus 48 controller groups, each group being `<name>` / `<name>key` / `<name>bezierkey`.
//
// Every arm re-evaluates firstword and the arms are independent `if`s, not an else-if chain, so a
// line matching one scalar key still falls through to the controller groups below it.
void MdlNodeEmitter::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        bool bPointToPoint = false;
        int  nPointToPointSel = 0;
        bool bAffectedByWind = false;
        bool bTinted = false;
        bool bBouncing = false;
        int  nRandom = 0;
        bool bInheriting = false;
        bool bInheritingVelocity = false;
        bool bInheritingLocal = false;
        bool bSplat = false;
        bool bInheritingPart = false;
        bool bDepthTexture = false;

        if (_stricmp("p2p", firstword(pszLine)) == 0) {
            int64_t nName = strlen("p2p");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bPointToPoint = true;
            if (!_stricmp("false", szValue)) bPointToPoint = false;
            if (!_stricmp("1", szValue)) bPointToPoint = true;
            if (!_stricmp("0", szValue)) bPointToPoint = false;
        }
        if (_stricmp("p2p_sel", firstword(pszLine)) == 0) {
            int64_t nName = strlen("p2p_sel");
            if (sscanf(pszLine + nName, "%d", &nPointToPointSel) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) nPointToPointSel = 1;
                if (!_stricmp("FALSE", szValue)) nPointToPointSel = 0;
            }
        }
        if (_stricmp("affectedByWind", firstword(pszLine)) == 0) {
            int64_t nName = strlen("affectedByWind");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bAffectedByWind = true;
            if (!_stricmp("false", szValue)) bAffectedByWind = false;
            if (!_stricmp("1", szValue)) bAffectedByWind = true;
            if (!_stricmp("0", szValue)) bAffectedByWind = false;
        }
        if (_stricmp("m_isTinted", firstword(pszLine)) == 0) {
            int64_t nName = strlen("m_isTinted");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bTinted = true;
            if (!_stricmp("false", szValue)) bTinted = false;
            if (!_stricmp("1", szValue)) bTinted = true;
            if (!_stricmp("0", szValue)) bTinted = false;
        }
        if (_stricmp("bounce", firstword(pszLine)) == 0) {
            int64_t nName = strlen("bounce");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bBouncing = true;
            if (!_stricmp("false", szValue)) bBouncing = false;
            if (!_stricmp("1", szValue)) bBouncing = true;
            if (!_stricmp("0", szValue)) bBouncing = false;
        }
        if (_stricmp("inherit", firstword(pszLine)) == 0) {
            int64_t nName = strlen("inherit");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bInheriting = true;
            if (!_stricmp("false", szValue)) bInheriting = false;
            if (!_stricmp("1", szValue)) bInheriting = true;
            if (!_stricmp("0", szValue)) bInheriting = false;
        }
        if (_stricmp("random", firstword(pszLine)) == 0) {
            int64_t nName = strlen("random");
            if (sscanf(pszLine + nName, "%d", &nRandom) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) nRandom = 1;
                if (!_stricmp("FALSE", szValue)) nRandom = 0;
            }
        }
        if (_stricmp("inheritvel", firstword(pszLine)) == 0) {
            int64_t nName = strlen("inheritvel");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bInheritingVelocity = true;
            if (!_stricmp("false", szValue)) bInheritingVelocity = false;
            if (!_stricmp("1", szValue)) bInheritingVelocity = true;
            if (!_stricmp("0", szValue)) bInheritingVelocity = false;
        }
        if (_stricmp("inherit_local", firstword(pszLine)) == 0) {
            int64_t nName = strlen("inherit_local");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bInheritingLocal = true;
            if (!_stricmp("false", szValue)) bInheritingLocal = false;
            if (!_stricmp("1", szValue)) bInheritingLocal = true;
            if (!_stricmp("0", szValue)) bInheritingLocal = false;
        }
        if (_stricmp("splat", firstword(pszLine)) == 0) {
            int64_t nName = strlen("splat");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bSplat = true;
            if (!_stricmp("false", szValue)) bSplat = false;
            if (!_stricmp("1", szValue)) bSplat = true;
            if (!_stricmp("0", szValue)) bSplat = false;
        }
        if (_stricmp("inherit_part", firstword(pszLine)) == 0) {
            int64_t nName = strlen("inherit_part");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bInheritingPart = true;
            if (!_stricmp("false", szValue)) bInheritingPart = false;
            if (!_stricmp("1", szValue)) bInheritingPart = true;
            if (!_stricmp("0", szValue)) bInheritingPart = false;
        }
        if (_stricmp("depth_texture", firstword(pszLine)) == 0) {
            int64_t nName = strlen("depth_texture");
            char szValue[64];
            szValue[0] = 0;
            static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
            if (!_stricmp("true", szValue)) bDepthTexture = true;
            if (!_stricmp("false", szValue)) bDepthTexture = false;
            if (!_stricmp("1", szValue)) bDepthTexture = true;
            if (!_stricmp("0", szValue)) bDepthTexture = false;
        }
        if (_stricmp("renderorder", firstword(pszLine)) == 0) {
            int64_t nName = strlen("renderorder");
            int nValue;
            static_cast<void>(sscanf(pszLine + nName, "%d", &nValue));
            m_nRenderOrder = static_cast<short>(nValue);
        }
        if (_stricmp("spawntype", firstword(pszLine)) == 0) {
            int64_t nName = strlen("spawntype");
            if (sscanf(pszLine + nName, "%d", &m_nSpawnType) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nSpawnType = 1;
                if (!_stricmp("FALSE", szValue)) m_nSpawnType = 0;
            }
        }
        if (_stricmp("update", firstword(pszLine)) == 0) {
            int64_t nName = strlen("update");
            if (sscanf(pszLine + nName, "%s", m_szUpdate) == 0) m_szUpdate[0] = 0;
        }
        if (_stricmp("render", firstword(pszLine)) == 0) {
            int64_t nName = strlen("render");
            if (sscanf(pszLine + nName, "%s", m_szRender) == 0) m_szRender[0] = 0;
        }
        if (_stricmp("blend", firstword(pszLine)) == 0) {
            int64_t nName = strlen("blend");
            if (sscanf(pszLine + nName, "%s", m_szBlend) == 0) m_szBlend[0] = 0;
        }
        if (_stricmp("texture", firstword(pszLine)) == 0) {
            int64_t nName = strlen("texture");
            if (sscanf(pszLine + nName, "%s", m_szTexture) == 0) m_szTexture[0] = 0;
        }
        if (_stricmp("chunkName", firstword(pszLine)) == 0) {
            int64_t nName = strlen("chunkName");
            if (sscanf(pszLine + nName, "%s", m_szChunkName) == 0) m_szChunkName[0] = 0;
        }
        if (_stricmp("xgrid", firstword(pszLine)) == 0) {
            int64_t nName = strlen("xgrid");
            if (sscanf(pszLine + nName, "%d", &m_nXGrid) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nXGrid = 1;
                if (!_stricmp("FALSE", szValue)) m_nXGrid = 0;
            }
        }
        if (_stricmp("ygrid", firstword(pszLine)) == 0) {
            int64_t nName = strlen("ygrid");
            if (sscanf(pszLine + nName, "%d", &m_nYGrid) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nYGrid = 1;
                if (!_stricmp("FALSE", szValue)) m_nYGrid = 0;
            }
        }
        if (_stricmp("loop", firstword(pszLine)) == 0) {
            int64_t nName = strlen("loop");
            if (sscanf(pszLine + nName, "%d", &m_nLoop) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nLoop = 1;
                if (!_stricmp("FALSE", szValue)) m_nLoop = 0;
            }
        }
        if (_stricmp("deadspace", firstword(pszLine)) == 0) {
            int64_t nName = strlen("deadspace");
            static_cast<void>(sscanf(pszLine + nName, "%f", &m_fDeadSpace));
        }
        if (_stricmp("twosidedtex", firstword(pszLine)) == 0) {
            int64_t nName = strlen("twosidedtex");
            if (sscanf(pszLine + nName, "%d", &m_nTwoSidedTex) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nTwoSidedTex = 1;
                if (!_stricmp("FALSE", szValue)) m_nTwoSidedTex = 0;
            }
        }
        if (_stricmp("blastRadius", firstword(pszLine)) == 0) {
            int64_t nName = strlen("blastRadius");
            static_cast<void>(sscanf(pszLine + nName, "%f", &m_fBlastRadius));
        }
        if (_stricmp("blastLength", firstword(pszLine)) == 0) {
            int64_t nName = strlen("blastLength");
            static_cast<void>(sscanf(pszLine + nName, "%f", &m_fBlastLength));
        }
        if (_stricmp("m_bFrameBlending", firstword(pszLine)) == 0)
            Parse(pszLine + strlen("m_bFrameBlending"),
                  m_bFrameBlending);
        if (_stricmp("m_sDepthTextureName", firstword(pszLine)) == 0) {
            int64_t nName = strlen("m_sDepthTextureName");
            if (sscanf(pszLine + nName, "%s", m_szDepthTextureName) == 0) m_szDepthTextureName[0] = 0;
        }
        if (_stricmp("numBranches", firstword(pszLine)) == 0) {
            int64_t nName = strlen("numBranches");
            if (sscanf(pszLine + nName, "%d", &m_nNumBranches) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_nNumBranches = 1;
                if (!_stricmp("FALSE", szValue)) m_nNumBranches = 0;
            }
        }
        if (_stricmp("controlptsmoothing", firstword(pszLine)) == 0) {
            int64_t nName = strlen("controlptsmoothing");
            if (sscanf(pszLine + nName, "%d", &m_bControlPtSmoothing) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszLine + nName, "%s", szValue));
                if (!_stricmp("TRUE", szValue)) m_bControlPtSmoothing = 1;
                if (!_stricmp("FALSE", szValue)) m_bControlPtSmoothing = 0;
            }
        }

        if (bPointToPoint) EnablePointToPoint();
        if (nPointToPointSel == 1) EnablePointToPointSel();
        if (bAffectedByWind) EnableAffectedByWind();
        if (bTinted) EnableTinted();
        if (bBouncing) EnableBouncing();
        if (nRandom & 1) EnableRandom();
        if (nRandom & 2) EnableRandomOrder();
        if (bInheriting) EnableInheriting();
        if (bInheritingVelocity) EnableInheritingVelocity();
        if (bInheritingLocal) EnableInheritingLocal();
        if (bSplat) EnableSplat();
        if (bInheritingPart) EnableInheritingPart();
        if (bDepthTexture) EnableDepthTexture();
        if (_stricmp("blurlength", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("blurlength"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBlurLength)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("blurlengthkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("blurlengthkey"),
                                 pszLine + strlen("blurlengthkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBlurLength)), reinterpret_cast<void*>(-1));
        else if (_stricmp("blurlengthbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("blurlengthbezierkey",
                                                       sizeof("blurlengthbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBlurLength)), reinterpret_cast<void*>(-1));
        if (_stricmp("detonate", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("detonate"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_bDetonate)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("detonatekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("detonatekey"),
                                 pszLine + strlen("detonatekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_bDetonate)), reinterpret_cast<void*>(-1));
        else if (_stricmp("detonatebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("detonatebezierkey",
                                                       sizeof("detonatebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_bDetonate)), reinterpret_cast<void*>(-1));
        if (_stricmp("velocity", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("velocity"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fVelocity)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("velocitykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("velocitykey"),
                                 pszLine + strlen("velocitykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fVelocity)), reinterpret_cast<void*>(-1));
        else if (_stricmp("velocitybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("velocitybezierkey",
                                                       sizeof("velocitybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fVelocity)), reinterpret_cast<void*>(-1));
        if (_stricmp("randvel", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("randvel"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandVel)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("randvelkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("randvelkey"),
                                 pszLine + strlen("randvelkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandVel)), reinterpret_cast<void*>(-1));
        else if (_stricmp("randvelbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("randvelbezierkey",
                                                       sizeof("randvelbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandVel)), reinterpret_cast<void*>(-1));
        if (_stricmp("birthrate", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("birthrate"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBirthRate)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("birthratekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("birthratekey"),
                                 pszLine + strlen("birthratekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBirthRate)), reinterpret_cast<void*>(-1));
        else if (_stricmp("birthratebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("birthratebezierkey",
                                                       sizeof("birthratebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBirthRate)), reinterpret_cast<void*>(-1));
        if (_stricmp("m_fRandomBirthRate", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("m_fRandomBirthRate"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandomBirthRate)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("m_fRandomBirthRatekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("m_fRandomBirthRatekey"),
                                 pszLine + strlen("m_fRandomBirthRatekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandomBirthRate)), reinterpret_cast<void*>(-1));
        else if (_stricmp("m_fRandomBirthRatebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("m_fRandomBirthRatebezierkey",
                                                       sizeof("m_fRandomBirthRatebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fRandomBirthRate)), reinterpret_cast<void*>(-1));
        if (_stricmp("colorStart", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("colorStart"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[9])), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("colorStartkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("colorStartkey"),
                                 pszLine + strlen("colorStartkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[9])), reinterpret_cast<void*>(-1));
        else if (_stricmp("colorStartbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("colorStartbezierkey",
                                                       sizeof("colorStartbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[9])), reinterpret_cast<void*>(-1));
        if (_stricmp("colorMid", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("colorMid"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("colorMidkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("colorMidkey"),
                                 pszLine + strlen("colorMidkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors)), reinterpret_cast<void*>(-1));
        else if (_stricmp("colorMidbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("colorMidbezierkey",
                                                       sizeof("colorMidbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors)), reinterpret_cast<void*>(-1));
        if (_stricmp("colorEnd", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("colorEnd"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[8])), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("colorEndkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("colorEndkey"),
                                 pszLine + strlen("colorEndkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[8])), reinterpret_cast<void*>(-1));
        else if (_stricmp("colorEndbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("colorEndbezierkey",
                                                       sizeof("colorEndbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_avVectors[8])), reinterpret_cast<void*>(-1));
        if (_stricmp("percentStart", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("percentStart"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentStart)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("percentStartkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("percentStartkey"),
                                 pszLine + strlen("percentStartkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentStart)), reinterpret_cast<void*>(-1));
        else if (_stricmp("percentStartbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("percentStartbezierkey",
                                                       sizeof("percentStartbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentStart)), reinterpret_cast<void*>(-1));
        if (_stricmp("percentMid", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("percentMid"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentMid)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("percentMidkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("percentMidkey"),
                                 pszLine + strlen("percentMidkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentMid)), reinterpret_cast<void*>(-1));
        else if (_stricmp("percentMidbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("percentMidbezierkey",
                                                       sizeof("percentMidbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentMid)), reinterpret_cast<void*>(-1));
        if (_stricmp("percentEnd", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("percentEnd"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentEnd)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("percentEndkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("percentEndkey"),
                                 pszLine + strlen("percentEndkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentEnd)), reinterpret_cast<void*>(-1));
        else if (_stricmp("percentEndbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("percentEndbezierkey",
                                                       sizeof("percentEndbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fPercentEnd)), reinterpret_cast<void*>(-1));
        if (_stricmp("alphaStart", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("alphaStart"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaStart)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("alphaStartkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("alphaStartkey"),
                                 pszLine + strlen("alphaStartkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaStart)), reinterpret_cast<void*>(-1));
        else if (_stricmp("alphaStartbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("alphaStartbezierkey",
                                                       sizeof("alphaStartbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaStart)), reinterpret_cast<void*>(-1));
        if (_stricmp("alphaMid", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("alphaMid"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaMid)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("alphaMidkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("alphaMidkey"),
                                 pszLine + strlen("alphaMidkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaMid)), reinterpret_cast<void*>(-1));
        else if (_stricmp("alphaMidbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("alphaMidbezierkey",
                                                       sizeof("alphaMidbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaMid)), reinterpret_cast<void*>(-1));
        if (_stricmp("alphaEnd", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("alphaEnd"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaEnd)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("alphaEndkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("alphaEndkey"),
                                 pszLine + strlen("alphaEndkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaEnd)), reinterpret_cast<void*>(-1));
        else if (_stricmp("alphaEndbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("alphaEndbezierkey",
                                                       sizeof("alphaEndbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fAlphaEnd)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeStart", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeStart"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStart)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeStartkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeStartkey"),
                                 pszLine + strlen("sizeStartkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStart)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeStartbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeStartbezierkey",
                                                       sizeof("sizeStartbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStart)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeMid", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeMid"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMid)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeMidkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeMidkey"),
                                 pszLine + strlen("sizeMidkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMid)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeMidbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeMidbezierkey",
                                                       sizeof("sizeMidbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMid)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeEnd", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeEnd"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEnd)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeEndkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeEndkey"),
                                 pszLine + strlen("sizeEndkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEnd)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeEndbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeEndbezierkey",
                                                       sizeof("sizeEndbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEnd)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeStart_y", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeStart_y"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStartY)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeStart_ykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeStart_ykey"),
                                 pszLine + strlen("sizeStart_ykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStartY)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeStart_ybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeStart_ybezierkey",
                                                       sizeof("sizeStart_ybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeStartY)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeMid_y", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeMid_y"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMidY)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeMid_ykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeMid_ykey"),
                                 pszLine + strlen("sizeMid_ykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMidY)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeMid_ybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeMid_ybezierkey",
                                                       sizeof("sizeMid_ybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeMidY)), reinterpret_cast<void*>(-1));
        if (_stricmp("sizeEnd_y", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("sizeEnd_y"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEndY)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("sizeEnd_ykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("sizeEnd_ykey"),
                                 pszLine + strlen("sizeEnd_ykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEndY)), reinterpret_cast<void*>(-1));
        else if (_stricmp("sizeEnd_ybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("sizeEnd_ybezierkey",
                                                       sizeof("sizeEnd_ybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSizeEndY)), reinterpret_cast<void*>(-1));
        if (_stricmp("frameStart", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("frameStart"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameStart)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("frameStartkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("frameStartkey"),
                                 pszLine + strlen("frameStartkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameStart)), reinterpret_cast<void*>(-1));
        else if (_stricmp("frameStartbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("frameStartbezierkey",
                                                       sizeof("frameStartbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameStart)), reinterpret_cast<void*>(-1));
        if (_stricmp("frameEnd", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("frameEnd"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameEnd)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("frameEndkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("frameEndkey"),
                                 pszLine + strlen("frameEndkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameEnd)), reinterpret_cast<void*>(-1));
        else if (_stricmp("frameEndbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("frameEndbezierkey",
                                                       sizeof("frameEndbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFrameEnd)), reinterpret_cast<void*>(-1));
        if (_stricmp("lifeExp", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lifeExp"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLifeExp)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lifeExpkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lifeExpkey"),
                                 pszLine + strlen("lifeExpkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLifeExp)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lifeExpbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lifeExpbezierkey",
                                                       sizeof("lifeExpbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLifeExp)), reinterpret_cast<void*>(-1));
        if (_stricmp("mass", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("mass"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fMass)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("masskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("masskey"),
                                 pszLine + strlen("masskey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fMass)), reinterpret_cast<void*>(-1));
        else if (_stricmp("massbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("massbezierkey",
                                                       sizeof("massbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fMass)), reinterpret_cast<void*>(-1));
        if (_stricmp("spread", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("spread"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSpread)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("spreadkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("spreadkey"),
                                 pszLine + strlen("spreadkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSpread)), reinterpret_cast<void*>(-1));
        else if (_stricmp("spreadbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("spreadbezierkey",
                                                       sizeof("spreadbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fSpread)), reinterpret_cast<void*>(-1));
        if (_stricmp("particleRot", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("particleRot"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fParticleRot)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("particleRotkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("particleRotkey"),
                                 pszLine + strlen("particleRotkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fParticleRot)), reinterpret_cast<void*>(-1));
        else if (_stricmp("particleRotbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("particleRotbezierkey",
                                                       sizeof("particleRotbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fParticleRot)), reinterpret_cast<void*>(-1));
        if (_stricmp("fps", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("fps"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFPS)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("fpskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("fpskey"),
                                 pszLine + strlen("fpskey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFPS)), reinterpret_cast<void*>(-1));
        else if (_stricmp("fpsbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("fpsbezierkey",
                                                       sizeof("fpsbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fFPS)), reinterpret_cast<void*>(-1));
        if (_stricmp("xsize", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("xsize"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fXSize)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("xsizekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("xsizekey"),
                                 pszLine + strlen("xsizekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fXSize)), reinterpret_cast<void*>(-1));
        else if (_stricmp("xsizebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("xsizebezierkey",
                                                       sizeof("xsizebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fXSize)), reinterpret_cast<void*>(-1));
        if (_stricmp("ysize", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("ysize"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fYSize)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("ysizekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("ysizekey"),
                                 pszLine + strlen("ysizekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fYSize)), reinterpret_cast<void*>(-1));
        else if (_stricmp("ysizebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("ysizebezierkey",
                                                       sizeof("ysizebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fYSize)), reinterpret_cast<void*>(-1));
        if (_stricmp("bounce_co", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("bounce_co"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBounceCoeff)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("bounce_cokey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("bounce_cokey"),
                                 pszLine + strlen("bounce_cokey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBounceCoeff)), reinterpret_cast<void*>(-1));
        else if (_stricmp("bounce_cobezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("bounce_cobezierkey",
                                                       sizeof("bounce_cobezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fBounceCoeff)), reinterpret_cast<void*>(-1));
        if (_stricmp("lightningDelay", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lightningDelay"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningDelay)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lightningDelaykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lightningDelaykey"),
                                 pszLine + strlen("lightningDelaykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningDelay)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lightningDelaybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lightningDelaybezierkey",
                                                       sizeof("lightningDelaybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningDelay)), reinterpret_cast<void*>(-1));
        if (_stricmp("lightningRadius", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lightningRadius"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningRadius)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lightningRadiuskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lightningRadiuskey"),
                                 pszLine + strlen("lightningRadiuskey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningRadius)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lightningRadiusbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lightningRadiusbezierkey",
                                                       sizeof("lightningRadiusbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningRadius)), reinterpret_cast<void*>(-1));
        if (_stricmp("lightningScale", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lightningScale"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningScale)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lightningScalekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lightningScalekey"),
                                 pszLine + strlen("lightningScalekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningScale)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lightningScalebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lightningScalebezierkey",
                                                       sizeof("lightningScalebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningScale)), reinterpret_cast<void*>(-1));
        if (_stricmp("lightningSubDiv", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lightningSubDiv"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningSubDiv)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lightningSubDivkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lightningSubDivkey"),
                                 pszLine + strlen("lightningSubDivkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningSubDiv)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lightningSubDivbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lightningSubDivbezierkey",
                                                       sizeof("lightningSubDivbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningSubDiv)), reinterpret_cast<void*>(-1));
        if (_stricmp("lightningZigzag", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("lightningZigzag"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningZigzag)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("lightningZigzagkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("lightningZigzagkey"),
                                 pszLine + strlen("lightningZigzagkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningZigzag)), reinterpret_cast<void*>(-1));
        else if (_stricmp("lightningZigzagbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("lightningZigzagbezierkey",
                                                       sizeof("lightningZigzagbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fLightningZigzag)), reinterpret_cast<void*>(-1));
        if (_stricmp("p2p_bezier2", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("p2p_bezier2"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier2)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("p2p_bezier2key", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("p2p_bezier2key"),
                                 pszLine + strlen("p2p_bezier2key"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier2)), reinterpret_cast<void*>(-1));
        else if (_stricmp("p2p_bezier2bezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("p2p_bezier2bezierkey",
                                                       sizeof("p2p_bezier2bezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier2)), reinterpret_cast<void*>(-1));
        if (_stricmp("p2p_bezier3", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("p2p_bezier3"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier3)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("p2p_bezier3key", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("p2p_bezier3key"),
                                 pszLine + strlen("p2p_bezier3key"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier3)), reinterpret_cast<void*>(-1));
        else if (_stricmp("p2p_bezier3bezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("p2p_bezier3bezierkey",
                                                       sizeof("p2p_bezier3bezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fP2PBezier3)), reinterpret_cast<void*>(-1));
        if (_stricmp("combinetime", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("combinetime"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fCombineTime)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("combinetimekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("combinetimekey"),
                                 pszLine + strlen("combinetimekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fCombineTime)), reinterpret_cast<void*>(-1));
        else if (_stricmp("combinetimebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("combinetimebezierkey",
                                                       sizeof("combinetimebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fCombineTime)), reinterpret_cast<void*>(-1));
        if (_stricmp("drag", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("drag"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fDrag)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("dragkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("dragkey"),
                                 pszLine + strlen("dragkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fDrag)), reinterpret_cast<void*>(-1));
        else if (_stricmp("dragbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("dragbezierkey",
                                                       sizeof("dragbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fDrag)), reinterpret_cast<void*>(-1));
        if (_stricmp("grav", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("grav"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fGrav)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("gravkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("gravkey"),
                                 pszLine + strlen("gravkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fGrav)), reinterpret_cast<void*>(-1));
        else if (_stricmp("gravbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("gravbezierkey",
                                                       sizeof("gravbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fGrav)), reinterpret_cast<void*>(-1));
        if (_stricmp("threshold", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("threshold"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fThreshold)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("thresholdkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("thresholdkey"),
                                 pszLine + strlen("thresholdkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fThreshold)), reinterpret_cast<void*>(-1));
        else if (_stricmp("thresholdbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("thresholdbezierkey",
                                                       sizeof("thresholdbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fThreshold)), reinterpret_cast<void*>(-1));
        if (_stricmp("targetsize", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("targetsize"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTargetSize)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("targetsizekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("targetsizekey"),
                                 pszLine + strlen("targetsizekey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTargetSize)), reinterpret_cast<void*>(-1));
        else if (_stricmp("targetsizebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("targetsizebezierkey",
                                                       sizeof("targetsizebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTargetSize)), reinterpret_cast<void*>(-1));
        if (_stricmp("numcontrolpts", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("numcontrolpts"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fNumControlPts)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("numcontrolptskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("numcontrolptskey"),
                                 pszLine + strlen("numcontrolptskey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fNumControlPts)), reinterpret_cast<void*>(-1));
        else if (_stricmp("numcontrolptsbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("numcontrolptsbezierkey",
                                                       sizeof("numcontrolptsbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fNumControlPts)), reinterpret_cast<void*>(-1));
        if (_stricmp("controlptradius", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("controlptradius"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtRadius)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("controlptradiuskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("controlptradiuskey"),
                                 pszLine + strlen("controlptradiuskey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtRadius)), reinterpret_cast<void*>(-1));
        else if (_stricmp("controlptradiusbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("controlptradiusbezierkey",
                                                       sizeof("controlptradiusbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtRadius)), reinterpret_cast<void*>(-1));
        if (_stricmp("controlptdelay", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("controlptdelay"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtDelay)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("controlptdelaykey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("controlptdelaykey"),
                                 pszLine + strlen("controlptdelaykey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtDelay)), reinterpret_cast<void*>(-1));
        else if (_stricmp("controlptdelaybezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("controlptdelaybezierkey",
                                                       sizeof("controlptdelaybezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fControlPtDelay)), reinterpret_cast<void*>(-1));
        if (_stricmp("tangentspread", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("tangentspread"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentSpread)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("tangentspreadkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("tangentspreadkey"),
                                 pszLine + strlen("tangentspreadkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentSpread)), reinterpret_cast<void*>(-1));
        else if (_stricmp("tangentspreadbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("tangentspreadbezierkey",
                                                       sizeof("tangentspreadbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentSpread)), reinterpret_cast<void*>(-1));
        if (_stricmp("tangentlength", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(strlen("tangentlength"));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentLength)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("tangentlengthkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("tangentlengthkey"),
                                 pszLine + strlen("tangentlengthkey"),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentLength)), reinterpret_cast<void*>(-1));
        else if (_stricmp("tangentlengthbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("tangentlengthbezierkey",
                                                       sizeof("tangentlengthbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartEmitter, m_fTangentLength)), reinterpret_cast<void*>(-1));
    }
    MdlNode::InternalParseField(pszLine);
}

// Builds a node from its kind word and dispatches the block's lines to it. The kind tests run in a
// fixed order, with a plain MdlNode as the fallthrough.
MdlNode* ParseNode(char* pszLine, MaxTree* pTree)
{
    char szType[64];
    char szName[64];
#ifdef K2_FIX_ENGINE_BUGS
    szType[0] = 0;
    szName[0] = 0;
#endif
    static_cast<void>(sscanf(pszLine, "%s %s", szType, szName));

    MdlNode* pNode;
    if (!_stricmp(szType, "camera"))
        pNode = new MdlNodeCamera(szName);
    else if (!_stricmp(szType, "emitter"))
        pNode = new MdlNodeEmitter(szName);
    else if (!_stricmp(szType, "light"))
        pNode = new MdlNodeLight(szName);
    else if (!_stricmp(szType, "trimesh"))
        pNode = new MdlNodeTriMesh(szName);
    else if (!_stricmp(szType, "animmesh"))
        pNode = new MdlNodeAnimMesh(szName);
    else if (!_stricmp(szType, "danglymesh"))
        pNode = new MdlNodeDanglyMesh(szName);
    else if (!_stricmp(szType, "reference"))
        pNode = new MdlNodeReference(szName);
    else if (!_stricmp(szType, "aabb"))
        pNode = new MdlNodeAABB(szName);
    else if (!_stricmp(szType, "skin"))
        pNode = new MdlNodeSkin(szName);
    else if (!_stricmp(szType, "lightsaber"))
        pNode = new MdlNodeLightsaber(szName);
    else
        pNode = new MdlNode(szName);

    pNode->m_pTree = pTree;

    // Everything up to the `endnode` terminator belongs to the node.
    char* pszWork = AurResGetNextLine();
    while (pszWork) {
        pszWork = skipspace(pszWork);
        if (!strncmp(pszWork, "endnode", strlen("endnode")))
            break;
        pNode->ParseFieldDispatch(pszWork);
        pszWork = AurResGetNextLine();
    }

    // An animation replaces the parsed node with a bare MdlNode carrying its three
    // animation-relevant members over, then destroys the old node with its full type.
    if (pTree->AsAnimation()) {
        MdlNode* pNewNode = new MdlNode(szName);

        pNewNode->m_listFloats = pNode->m_listFloats;
        pNewNode->m_nNodeID = pNode->m_nNodeID;
        pNewNode->m_listControllers = pNode->m_listControllers;
        pNewNode->m_pTree = pTree;

        // A node that was already attached hands its place in the parent's child list on.
        if (pNode->m_pParent) {
            pNewNode->m_pParent = pNode->m_pParent;
            reinterpret_cast<MdlNode*>(pNode->m_pParent)->m_listChildren.Remove(pNode);
            reinterpret_cast<MdlNode*>(pNewNode->m_pParent)->m_listChildren.Add(pNewNode);
        } else if (pTree->m_pMdlNode == pNode) {
            pTree->m_pMdlNode = pNewNode;
        }

        if (pNode->AsMdlNodeAABB())
            delete pNode->AsMdlNodeAABB();
        else if (pNode->AsMdlNodeAnimMesh())
            delete pNode->AsMdlNodeAnimMesh();
        else if (pNode->AsMdlNodeCamera())
            delete pNode->AsMdlNodeCamera();
        else if (pNode->AsMdlNodeDanglyMesh())
            delete pNode->AsMdlNodeDanglyMesh();
        else if (pNode->AsMdlNodeEmitter())
            delete pNode->AsMdlNodeEmitter();
        else if (pNode->AsMdlNodeLight())
            delete pNode->AsMdlNodeLight();
        else if (pNode->AsMdlNodeLightsaber())
            delete pNode->AsMdlNodeLightsaber();
        else if (pNode->AsMdlNodeReference())
            delete pNode->AsMdlNodeReference();
        else if (pNode->AsMdlNodeSkin())
            delete pNode->AsMdlNodeSkin();
        else if (pNode->AsMdlNodeTriMesh())
            delete pNode->AsMdlNodeTriMesh();
        else
            delete pNode;

        pNode = pNewNode;
    }

    // A model node appends its freshly copied name to the model's string table and takes the new
    // last index; an animation node first searches the table so every animation node shares one name
    // per model.
    int nIndex;
    if (pNode->m_pTree->AsModel()) {
        char* pszCopy = new char[strlen(szName) + 1];
        strcpy(pszCopy, szName);
        pNode->m_pTree->AsModel()->m_listStrings.Add(pszCopy);
        nIndex = pNode->m_pTree->AsModel()->m_listStrings.m_count - 1;
    } else {
        nIndex = pNode->m_pTree->AsAnimation()->m_pModel->m_listStrings.m_count - 1;
        while (nIndex >= 0
               && _stricmp(
                      pNode->m_pTree->AsAnimation()->m_pModel->m_listStrings.m_pData[nIndex],
                      szName) != 0)
            nIndex--;
        if (nIndex < 0) {
            char* pszCopy = new char[strlen(szName) + 1];
            strcpy(pszCopy, szName);
            pNode->m_pTree->AsAnimation()->m_pModel->m_listStrings.Add(pszCopy);
            nIndex = pNode->m_pTree->AsAnimation()->m_pModel->m_listStrings.m_count - 1;
        }
    }
    pNode->m_nNameIndex = nIndex;

    // A node that is neither attached to a parent nor the tree's root is dropped.
#if KOTOR1  // K1 treats an orphan node as a hard error instead of returning null
    if (pNode->m_pParent == 0 && pTree->m_pMdlNode != pNode)
        throw NULL;
    pNode->m_nNodeID = static_cast<short>(pTree->m_nNodeCount++);
    if (!disablepostprocessing)
        pNode->PostProcessDispatch();
#else
    if (pNode->m_pParent == 0 && pTree->m_pMdlNode != pNode) {
        pNode = 0;
    } else {
        pNode->m_nNodeID = static_cast<short>(pTree->m_nNodeCount++);
        if (!disablepostprocessing)
            pNode->PostProcessDispatch();
    }
#endif
    return pNode;
}

// The .mdl parser drives every tree type through a table of `void*` trampolines; each concrete tree
// supplies its own pair and they do nothing but re-type the receiver.

void InsertModel(Model* pModel);
void UpdateAnimFootprint(MdlNode* pNode, int nDelta);

extern List<Model*> Models;
extern int removedelref;

// ---- the .mdl file-level callbacks --------------------------------------------------------------
// The free functions the `.mdl` token parser calls back into while a model is being built.
extern Model* currentmodel;

void BinaryInsertModel(Model* pModel, int nLow, int nHigh);

// ---- the sorted model table ---------------------------------------------------------------------

#if KOTOR1  // K1 reads the log file through SDL_RWops
#include "sdlrwops.h"
#endif

int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...);

Model* BinaryFindModel(char* pszName);

// ---- the MaxTree parser hooks -------------------------------------------------------------------
// Marks MaxTree's constructor and destructor so callers always emit a call.
#define MAXTREE_OUT_OF_LINE

// Each concrete tree supplies its own pair of parse/destructor trampolines, which do nothing but
// re-type the receiver.

// The anim-mesh node's no-op post-process override lives here so that neither PostProcessDispatch
// nor the anim-mesh translation unit can see the body; a visible body prunes the whole case out of
// the dispatch switch.

// ---- the MdlNodeSkin parser hooks ----------------------------------------------------------------

// ---- the MdlNodeLight hooks ---------------------------------------------------------------------

// ---- the MdlNodeDanglyMesh hooks -----------------------------------------------------------------

// ---- the MdlNodeLightsaber hooks -----------------------------------------------------------------

// The four segment counts are defined in MdlNode.cpp rather than here.

// ---- the MdlNodeAABB hooks ----------------------------------------------------------------------

class MaxTree;

// ---- the MdlNode dispatch family -----------------------------------------------------------------

extern int countnode;
extern int countmodelnodes;

// The node seed set includes the identity quaternion {0, 0, 0, 1}.

// The parser drives every node through `void*` entry points, and each trampoline does nothing but
// re-type the receiver.

// The derived kinds supply their own copies of the same trampolines plus their factories; every
// callee is declared in its class header and defined elsewhere.

// The node-tree builder's factory for a plain node.

// The derived kinds' factories.

// The kind dispatch's plain-Part arm routes a new Part to m_pNode.

// ---- the InternalDestructor family --------------------------------------------------------
//
// Each kind has one body, which walks the children in reverse through DestructorDispatch, then
// decrements the live-node count. The derived copies are the same body as the base's.

#define MDLNODE_PLAIN_INTERNAL_DTOR(classname)                                            \
    /* one body per kind */                                                               \
    void classname::InternalDestructor()                                                   \
    {                                                                                      \
        for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)                             \
            m_listChildren.m_pData[i]->DestructorDispatch();                               \
        countnode--;                                                                        \
    }

// ---- the AsX/IsX family -------------------------------------------------------------------------
//
// The dispatch arms downcast through the checked As* helpers, so their bodies live in this unit.

// The IsX siblings are plain mask tests.

// ---- the checked downcasts --------------------------------------------------------------------
//
// The bodies the dispatch arms call directly.

#ifndef _MSC_VER  // MSVC: aurmodel.cpp's definition only (an inline copy beside it is LNK2005)
inline MdlNodeLight* MdlNode::AsMdlNodeLight()
{
    if ((m_nFlags & NODE_LIGHT) != NODE_LIGHT)
        return 0;
    return static_cast<MdlNodeLight*>(this);
}
#endif

// ---- the per-kind ConJoin factories --------------------------------------------------------------
//
// The dispatcher above expands this block whole; the shape splits on the constructor's visibility.
// The emitter's constructor is declared only, so its factory is a `new` plus a call, while every
// other kind has an in-class constructor body and expands its whole seed set here.

// ---- the trimesh-derived kind ctors and factories --------------------------------------------
//
// The three trimesh-derived kinds each carry the whole draw-node seed set, with one body shape, the
// MdlNode base seed, the trimesh block, then the kind's own extra seeds.
//
// The flat trimesh layout is the size of an MdlNodeSkin object rather than an MdlNodeTriMesh one,
// because MdlNodeTriMesh.h carries every TriMesh-derived class's fields in one layout. The factories
// therefore pass the required size to `operator new` explicitly and construct into the result.

// The dangerous pair's own bodies stay here and not in the dangly class's unit, because this unit
// only declares the two methods, so the wrappers keep their tail calls.

// ---- the per-kind factories ---------------------------------------------------------------------
//
// The no-argument ones are spelled `new PartCamera` with no parentheses, because `new T()`
// value-initialises the object, which prepends a whole-object memset. The node-taking factories
// pass an argument and are unaffected.
//
// There are two shapes. Part, PartCamera and PartReference take the default constructor and store
// m_pNode explicitly; PartTriMesh, PartAnimMesh, PartDanglyMesh, PartAABB and PartLightsaber take
// the node as a constructor argument, so those arms are `operator new` plus a call. The Gob is
// unused by every one of them and stays in the signature because all twelve arms share one function
// type.
//
// The zero stores from Part's in-class initialisers pair up into one 16-byte store, and which pairs
// form is fixed by the declaration order. Do not reorder CAurPart.h's members: every offset in that
// header is pinned.
// The family's InternalPostProcess hook bodies cannot live here, because PostProcessDispatch must
// keep every arm an external call.

// ---- MdlNodeCamera's parser-table trampolines ------------------------------------------------
//
// The camera node's hooks live in MdlNodeCamera.h, shared with MdlNodeControl.cpp, which defines the
// control body.

// ---- the Light / Reference / TriMesh parser-table trampolines --------------------------------
//
// The callees' bodies are not in this unit, so each reference stays external and the wrapper keeps a
// single branch.

// ---- the per-kind factory / destructor table closures -----------------------------------------
//
// Each destructor wrapper repeats the same child walk plus countnode decrement, and each
// CreateInstance wrapper is `new <Part kind>(node)`; the Gob is unused by these arms.

// ==== the .mdl text-field parser ==================================================================

// ---- the MdlNodeTriMesh hooks -------------------------------------------------------------------
//
// This unit hosts the destructor pair, which must stay out of line for the units that delete a node
// by kind.

// The call sites that want the checked strcpy name __strcpy_chk directly.


// ---- MdlNodeTriMesh_PostProcess ---------------------------------------------------------------
// The trimesh's parse scratchpad; the type and its member model live in its own header.

// The text-field parser's shared surface, file-local copies of firstword and skipspace plus the
// Parse<T> declarations.


// The default constructor is defined in PartSkin.cpp; with only a declaration in scope, the lazy
// allocation site in InternalParseField makes a real call, which is what the engine build does.

// ---- the .mdl text-field parser -----------------------------------------------------------------

// ---- the MdlNodeAnimMesh parser hooks ------------------------------------------------------------
// The five file-scope parser-table trampolines must live in a unit that only declares the callees,
// so each reference stays a call.

// In the anim-mesh arm of the kind dispatch, the node is handed straight to the part's node-taking
// constructor, with no field fixups (unlike the TriMesh and Skin arms). The Gob is unused.

// The element type's destructor stays out of line. The header declares it and NodeVertex.cpp
// defines it, so the element loop keeps a destructor call plus delete pair.

Model* FindModel(char* pszName);
extern Model* currentmodel;
extern List<Model*> Models;
char* lastsupermodelname;
int uselastsupermodelname;

extern void      operator delete(void* p) throw();

// The base node's field chain, which every derived node tail-calls once it has consumed its own
// keys. Each field test is `_stricmp("<literal>", firstword(pszLine))`, the key is stripped with
// strlen, and each animatable property has a three-key group: the plain name, `<name>key` and
// `<name>bezierkey`.
//
// The position and orientation keys appear twice, once for the node's own transform and once to open
// the controller group, so both tests run on one line.
static inline void ParseVectorInlined(char* pszLine, Vector& vValue)
{
    static_cast<void>(sscanf(pszLine, "%f %f %f", &vValue.x, &vValue.y, &vValue.z));
}

static inline void ParseQuaternionInlined(char* pszLine, Quaternion& qValue)
{
    Vector vAxis;
    float fAngle;
    static_cast<void>(sscanf(pszLine, "%f %f %f %f", &vAxis.x, &vAxis.y, &vAxis.z, &fAngle));
    if (vAxis.x == 0.0f && vAxis.y == 0.0f && vAxis.z == 0.0f)
        qValue = Quaternion();
    else
        qValue = Quaternion(vAxis, fAngle);
}

void MdlNode::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("position", firstword(pszLine)) == 0)
            ParseVectorInlined(pszLine + __strlen_chk("position", sizeof("position")), m_vPosition);
        if (_stricmp("orientation", firstword(pszLine)) == 0) {
            // Bind the destination to a reference so its address is formed before the scan.
            ParseQuaternionInlined(pszLine + __strlen_chk("orientation", sizeof("orientation")),
                         m_qOrientation);
        }

        // The callee never reads its first argument, so the callers pass the already-computed key
        // length through it.
        if (_stricmp("position", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(__strlen_chk("position", sizeof("position")));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_vLocalPosition)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("positionkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("positionkey"),
                                 pszLine + __strlen_chk("positionkey", sizeof("positionkey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_vLocalPosition)), reinterpret_cast<void*>(-1));
        else if (_stricmp("positionbezierkey", firstword(pszLine)) == 0)
            AddBezierController(
                firstword(pszLine),
                pszLine + __strlen_chk("positionbezierkey", sizeof("positionbezierkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_vLocalPosition)), reinterpret_cast<void*>(-1));

        if (_stricmp("orientation", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(__strlen_chk("orientation", sizeof("orientation")));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_qLocalOrientation)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("orientationkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("orientationkey"),
                                 pszLine + __strlen_chk("orientationkey", sizeof("orientationkey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_qLocalOrientation)), reinterpret_cast<void*>(-1));
        else if (_stricmp("orientationbezierkey", firstword(pszLine)) == 0)
            AddBezierController(
                firstword(pszLine),
                pszLine + __strlen_chk("orientationbezierkey", sizeof("orientationbezierkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_qLocalOrientation)), reinterpret_cast<void*>(-1));

        if (_stricmp("wirecolor", firstword(pszLine)) == 0) {
            Vector vColor;
            static_cast<void>(sscanf(pszLine + __strlen_chk("wirecolor", sizeof("wirecolor")), "%f %f %f",
                   &vColor.x, &vColor.y, &vColor.z));
        }
        if (_stricmp("scale", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(__strlen_chk("scale", sizeof("scale")));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_scale)), reinterpret_cast<void*>(-1));
        }
        else if (_stricmp("scalekey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("scalekey"),
                                 pszLine + __strlen_chk("scalekey", sizeof("scalekey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_scale)), reinterpret_cast<void*>(-1));
        else if (_stricmp("scalebezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("scalebezierkey", sizeof("scalebezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(Part, m_scale)), reinterpret_cast<void*>(-1));
    }

    // The parent link is built outside the disablepostprocessing gate, so the tree's root and every
    // child link still exist in that mode.
    if (!strncmp("parent", pszLine, __strlen_chk("parent", sizeof("parent")))) {
        char szName[64];
#ifdef K2_FIX_ENGINE_BUGS
        szName[0] = 0;
#endif
        static_cast<void>(sscanf(pszLine + __strlen_chk("parent", sizeof("parent")), "%s", szName));
        if (_stricmp(szName, NULL_NAME) == 0) {
            m_pParent = 0;
            m_pTree->m_pMdlNode = this;      // "no parent" means this node is the tree's root
        }
        else {
            MdlNode* pParent = FindMdlNode(m_pTree, szName);
            m_pParent = reinterpret_cast<unsigned long long>(pParent);
            if (pParent)
                pParent->m_listChildren.Add(this);
        }
    }
}

// The light node's field chain has five three-key controller groups (color, radius, shadowradius,
// verticaldisplacement, multiplier) followed by independent single-keyword arms.
//
// Each group's *key and *bezierkey arms call AddGenericController / AddBezierController, which are
// empty in the shipped engine, so only the key-length call remains of them.
//
// AddGenericControllerSingle never reads its first argument; the already-computed key length is
// still passed through it.
void MdlNodeLight::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("color", firstword(pszLine)) == 0)
            { int64_t nName = static_cast<int64_t>(__strlen_chk("color", sizeof("color")));
              AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                         m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_vLightColour)), reinterpret_cast<void*>(-1)); }
        else if (_stricmp("colorkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("colorkey"),
                                 pszLine + __strlen_chk("colorkey", sizeof("colorkey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_vLightColour)), reinterpret_cast<void*>(-1));
        else if (_stricmp("colorbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("colorbezierkey", sizeof("colorbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_vLightColour)), reinterpret_cast<void*>(-1));

        if (_stricmp("radius", firstword(pszLine)) == 0)
            { int64_t nName = static_cast<int64_t>(__strlen_chk("radius", sizeof("radius")));
              AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                         m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightRadius)), reinterpret_cast<void*>(-1)); }
        else if (_stricmp("radiuskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("radiuskey"),
                                 pszLine + __strlen_chk("radiuskey", sizeof("radiuskey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightRadius)), reinterpret_cast<void*>(-1));
        else if (_stricmp("radiusbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("radiusbezierkey", sizeof("radiusbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightRadius)), reinterpret_cast<void*>(-1));

        if (_stricmp("shadowradius", firstword(pszLine)) == 0)
            { int64_t nName = static_cast<int64_t>(__strlen_chk("shadowradius", sizeof("shadowradius")));
              AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                         m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fShadowRadius)), reinterpret_cast<void*>(-1)); }
        else if (_stricmp("shadowradiuskey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("shadowradiuskey"),
                                 pszLine + __strlen_chk("shadowradiuskey", sizeof("shadowradiuskey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fShadowRadius)), reinterpret_cast<void*>(-1));
        else if (_stricmp("shadowradiusbezierkey", firstword(pszLine)) == 0)
            AddBezierController(
                firstword(pszLine),
                pszLine + __strlen_chk("shadowradiusbezierkey", sizeof("shadowradiusbezierkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fShadowRadius)), reinterpret_cast<void*>(-1));

        if (_stricmp("verticaldisplacement", firstword(pszLine)) == 0)
            { int64_t nName = static_cast<int64_t>(__strlen_chk("verticaldisplacement",
                                               sizeof("verticaldisplacement")));
              AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                         m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fVerticalDisplacement)), reinterpret_cast<void*>(-1)); }
        else if (_stricmp("verticaldisplacementkey", firstword(pszLine)) == 0)
            AddGenericController(
                const_cast<char*>("verticaldisplacementkey"),
                pszLine + __strlen_chk("verticaldisplacementkey", sizeof("verticaldisplacementkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fVerticalDisplacement)), reinterpret_cast<void*>(-1));
        else if (_stricmp("verticaldisplacementbezierkey", firstword(pszLine)) == 0)
            AddBezierController(
                firstword(pszLine),
                pszLine + __strlen_chk("verticaldisplacementbezierkey",
                                       sizeof("verticaldisplacementbezierkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fVerticalDisplacement)), reinterpret_cast<void*>(-1));

        if (_stricmp("multiplier", firstword(pszLine)) == 0)
            { int64_t nName = static_cast<int64_t>(__strlen_chk("multiplier", sizeof("multiplier")));
              AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                         m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightScale)), reinterpret_cast<void*>(-1)); }
        else if (_stricmp("multiplierkey", firstword(pszLine)) == 0)
            AddGenericController(const_cast<char*>("multiplierkey"),
                                 pszLine + __strlen_chk("multiplierkey", sizeof("multiplierkey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightScale)), reinterpret_cast<void*>(-1));
        else if (_stricmp("multiplierbezierkey", firstword(pszLine)) == 0)
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("multiplierbezierkey",
                                                       sizeof("multiplierbezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartLight, m_fLightScale)), reinterpret_cast<void*>(-1));

        if (_stricmp("ambientonly", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("ambientonly", sizeof("ambientonly"));
            if (sscanf(pszValue, "%d", &m_bAmbientOnly) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bAmbientOnly = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bAmbientOnly = 0;
            }
        }
        if (_stricmp("shadow", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("shadow", sizeof("shadow"));
            if (sscanf(pszValue, "%d", &m_bShadow) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bShadow = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bShadow = 0;
            }
        }
        if (_stricmp("isdynamic", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("isdynamic", sizeof("isdynamic"));
            if (sscanf(pszValue, "%d", &m_bIsDynamic) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bIsDynamic = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bIsDynamic = 0;
            }
        }
        // `nDynamicType` is a second spelling of the same key (`isdynamic`): it targets the same
        // field and reuses the pointer computed by the arm above.
        if (_stricmp("nDynamicType", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("nDynamicType", sizeof("nDynamicType"));
            if (sscanf(pszValue, "%d", &m_bIsDynamic) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bIsDynamic = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bIsDynamic = 0;
            }
        }
        if (_stricmp("affectdynamic", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("affectdynamic", sizeof("affectdynamic"));
            if (sscanf(pszValue, "%d", &m_bAffectDynamic) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bAffectDynamic = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bAffectDynamic = 0;
            }
        }
        if (_stricmp("lightpriority", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("lightpriority", sizeof("lightpriority"));
            if (sscanf(pszValue, "%d", &m_nPriority) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_nPriority = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_nPriority = 0;
            }
        }
        if (_stricmp("generateflare", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("generateflare", sizeof("generateflare"));
            if (sscanf(pszValue, "%d", &m_bGenerateFlare) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_bGenerateFlare = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bGenerateFlare = 0;
            }
        }

        if (_stricmp("texturenames", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("texturenames", sizeof("texturenames")), m_lFlareNames);
        if (_stricmp("flaresizes", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("flaresizes", sizeof("flaresizes")), m_lFlareSizes);
        if (_stricmp("flarepositions", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("flarepositions", sizeof("flarepositions")),
                  m_lFlarePositions);
        if (_stricmp("flarecolorshifts", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("flarecolorshifts", sizeof("flarecolorshifts")),
                  m_lFlareColourShifts);

        if (_stricmp("flareradius", firstword(pszLine)) == 0) {
            static_cast<void>(sscanf(pszLine + __strlen_chk("flareradius", sizeof("flareradius")), "%f",
                   &m_fFlareRange));
        }
        if (_stricmp("fadingLight", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("fadingLight", sizeof("fadingLight"));
            if (sscanf(pszValue, "%d", &m_nFadingEnabled) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_nFadingEnabled = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_nFadingEnabled = 0;
            }
        }
    }
    MdlNode::InternalParseField(pszLine);
}

// The trimesh node's field chain. Three arms run outside the post-processing gate (texture0,
// texture1 and bitmap, whose texture is needed even in that mode); inside it, the parse scratchpad is
// allocated lazily and the embedded .lmt lightmap reader runs alongside the ordinary keys.
//
// The two animatable colours at the end are three-key controller groups, the same shape the light
// node uses.
void MdlNodeTriMesh::InternalParseField(char* pszLine)
{
    if (_stricmp("texture0", firstword(pszLine)) == 0) {
        if (sscanf(pszLine + __strlen_chk("texture0", sizeof("texture0")), "%s", m_szTexture) == 0)
            m_szTexture[0] = 0;
    }
    if (_stricmp("texture1", firstword(pszLine)) == 0) {
        if (sscanf(pszLine + __strlen_chk("texture1", sizeof("texture1")), "%s", m_szLightmap) == 0)
            m_szLightmap[0] = 0;
    }
    if (_stricmp("bitmap", firstword(pszLine)) == 0) {
        char szValue[64];
        if (sscanf(pszLine + __strlen_chk("bitmap", sizeof("bitmap")), "%s", szValue) == 0)
            szValue[0] = 0;
        if (szValue[0] != 0)
            __strcpy_chk(m_szTexture, szValue, sizeof(m_szTexture));
    }
    if (!disablepostprocessing) {
        // The parse scratchpad, built lazily and kept for the node's lifetime. The pointer is held in
        // a local so the arms below do not each reload the member.
        MdlNodeTriMesh_PostProcess* pPost = PostProcess();
        if (pPost == 0) {
            pPost = new MdlNodeTriMesh_PostProcess();
            m_pPostProcess = pPost;
        }

        if (_stricmp("verts", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("verts", sizeof("verts")),
                  pPost->m_verts);
        if (_stricmp("tverts", firstword(pszLine)) == 0) {
            List<Vector>& l = pPost->m_tverts;
            Parse(pszLine + __strlen_chk("tverts", sizeof("tverts")), l);
        }
        if (_stricmp("animateuv", firstword(pszLine)) == 0) {
            int nValue = 0;
            char* pszValue = pszLine + __strlen_chk("animateuv", sizeof("animateuv"));
            if (sscanf(pszValue, "%d", &nValue) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    nValue = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    nValue = 0;
            }
            if (nValue != 0)
                m_nAnimateUV = 1;
        }
        if (m_nAnimateUV != 0) {
            if (_stricmp("uvdirectionx", firstword(pszLine)) == 0) {
                float& fValue = m_fUVDirectionX;
                static_cast<void>(sscanf(pszLine + __strlen_chk("uvdirectionx", sizeof("uvdirectionx")), "%f", &fValue));
            }
            if (_stricmp("uvdirectiony", firstword(pszLine)) == 0) {
                float& fValue = m_fUVDirectionY;
                static_cast<void>(sscanf(pszLine + __strlen_chk("uvdirectiony", sizeof("uvdirectiony")), "%f", &fValue));
            }
            if (_stricmp("uvjitter", firstword(pszLine)) == 0) {
                float& fValue = m_fUVJitter;
                static_cast<void>(sscanf(pszLine + __strlen_chk("uvjitter", sizeof("uvjitter")), "%f", &fValue));
            }
            if (_stricmp("uvjitterspeed", firstword(pszLine)) == 0) {
                float& fValue = m_fUVJitterSpeed;
                static_cast<void>(sscanf(pszLine + __strlen_chk("uvjitterspeed", sizeof("uvjitterspeed")), "%f", &fValue));
            }
        }
        if (_stricmp("tverts1", firstword(pszLine)) == 0) {
            List<Vector>& l = pPost->m_tverts1;
            Parse(pszLine + __strlen_chk("tverts1", sizeof("tverts1")), l);
        }
        if (_stricmp("tverts2", firstword(pszLine)) == 0) {
            List<Vector>& l = pPost->m_tverts2;
            Parse(pszLine + __strlen_chk("tverts2", sizeof("tverts2")), l);
        }
        if (_stricmp("tverts3", firstword(pszLine)) == 0) {
            List<Vector>& l = pPost->m_tverts3;
            Parse(pszLine + __strlen_chk("tverts3", sizeof("tverts3")), l);
        }
        if (_stricmp("colors", firstword(pszLine)) == 0) {
            List<Vector>& l = pPost->m_colors;
            Parse(pszLine + __strlen_chk("colors", sizeof("colors")), l);
        }
        // Three file-scope lists, shared with the rest of the engine.
        if (_stricmp("texindices1", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("texindices1", sizeof("texindices1")),
                  MaxFace_PostProcess::texindices1);
        if (_stricmp("texindices2", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("texindices2", sizeof("texindices2")),
                  MaxFace_PostProcess::texindices2);
        if (_stricmp("texindices3", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("texindices3", sizeof("texindices3")),
                  MaxFace_PostProcess::texindices3);

        // ---- the embedded .lmt lightmap reader ---------------------------------------------------
        // A second text stream, opened for a node that already carries a lightmap name and has at
        // least one texture-vertex pair. `tverts1offset` fills a local index list (four ints per
        // element: an affine remap plus a normalising divisor) and `texture1` replaces the lightmap
        // name. When the offset count is four per element the table is applied to the vertex pairs in
        // place.
        if (m_szLightmap[0] != 0 && pPost->m_tverts1.m_count >= 1) {
            char szLightmap[64];
            __strcpy_chk(szLightmap, m_szLightmap, sizeof(szLightmap));
            void* pRes = AurResGet(m_szLightmap, const_cast<char*>(".lmt"), 0, true);
            if (pRes != 0) {
                List<int> lIndices;
                // The line fetch drives the loop, so there is a single scan site.
                char* pszResLine;
                while ((pszResLine = AurResGetNextLine()) != 0) {
                    pszResLine = skipspace(pszResLine);
                    if (_stricmp("tverts1offset", firstword(pszResLine)) == 0)
                        Parse(pszResLine + __strlen_chk("tverts1offset", sizeof("tverts1offset")),
                              lIndices);
                    if (_stricmp("texture1", firstword(pszResLine)) == 0) {
                        if (sscanf(pszResLine + __strlen_chk("texture1", sizeof("texture1")), "%s",
                                   m_szLightmap) == 0)
                            m_szLightmap[0] = 0;
                    }
                }
                AurResFree(pRes, 0);
                if (lIndices.m_count == pPost->m_tverts1.m_count * 4
                    && pPost->m_tverts1.m_count >= 1) {
                    float fScale = 1.0f;
                    for (int64_t i = 0; i < pPost->m_tverts1.m_count; i++) {
                        Vector& v = pPost->m_tverts1.m_pData[i];
                        float f = static_cast<float>(lIndices.m_pData[i * 4]);
                        v.z = v.z * f;
                        v.x = v.x * f + static_cast<float>(lIndices.m_pData[i * 4 + 1]);
                        v.y = v.y * f + static_cast<float>(lIndices.m_pData[i * 4 + 2]);
                        int nDivisor = lIndices.m_pData[i * 4 + 3];
                        if (nDivisor != 0) {
                            float fInv = fScale / static_cast<float>(nDivisor);
                            v.x = v.x * fInv;
                            v.y = v.y * fInv;
                            v.z = v.z * fInv;
                        }
                    }
                }
            }
        }

        // The face list is built once, from the scratchpad's own records.
        if (pPost->m_faces.m_count == 0) {
            if (_stricmp("faces", firstword(pszLine)) == 0)
                Parse(pszLine + __strlen_chk("faces", sizeof("faces")),
                      pPost->m_faces);
        }
        if (pPost->m_faces.m_count >= 1) {
            m_faces.SetSize(pPost->m_faces.m_count);
            for (int64_t i = 0; i < pPost->m_faces.m_count; i++)
                m_faces.m_pData[i].m_nSurfaceMaterial = pPost->m_faces.m_pData[i].m_nMaterial;
        }

        if (_stricmp("diffuse", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("diffuse", sizeof("diffuse")), "%f %f %f", &m_vDiffuse.x,
                   &m_vDiffuse.y, &m_vDiffuse.z));
        if (_stricmp("ambient", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("ambient", sizeof("ambient")), "%f %f %f", &m_vAmbient.x,
                   &m_vAmbient.y, &m_vAmbient.z));
        if (_stricmp("shadow", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("shadow", sizeof("shadow")), m_bShadow);
        // This one field is an int, not a bool, so it uses the shared integer reader.
        if (_stricmp("transparencyhint", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("transparencyhint", sizeof("transparencyhint"));
            if (sscanf(pszValue, "%d", &m_nTransparencyHint) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                if (_stricmp("TRUE", szValue) == 0)
                    m_nTransparencyHint = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_nTransparencyHint = 0;
            }
        }
        if (_stricmp("beaming", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("beaming", sizeof("beaming")), m_bBeaming);
        if (_stricmp("render", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("render", sizeof("render")), m_bRender);
        if (_stricmp("rotatetexture", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("rotatetexture", sizeof("rotatetexture")), m_bRotateTexture);
        if (_stricmp("bmin", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("bmin", sizeof("bmin")), "%f %f %f", &m_vMin.x, &m_vMin.y,
                   &m_vMin.z));
        if (_stricmp("bmax", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("bmax", sizeof("bmax")), "%f %f %f", &m_vMax.x, &m_vMax.y,
                   &m_vMax.z));
        if (_stricmp("vertexindicescount", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("vertexindicescount", sizeof("vertexindicescount")),
                  m_lVertexIndicesCount);
        if (_stricmp("leftoverfaces", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("leftoverfaces", sizeof("leftoverfaces")), m_lLeftOverFaces);
        if (_stricmp("vertexindices", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("vertexindices", sizeof("vertexindices")), m_lVertexIndices);
        if (_stricmp("lightmapped", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("lightmapped", sizeof("lightmapped")), m_bHasLightmap);
        if (_stricmp("m_bIsBackgroundGeometry", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("m_bIsBackgroundGeometry", sizeof("m_bIsBackgroundGeometry")),
                  m_bIsBackgroundGeometry);
#if !KOTOR1  // K1 parses no dirt or hologram fields
        if (_stricmp("dirt_enabled", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("dirt_enabled", sizeof("dirt_enabled")), m_bDirtEnabled);
        if (_stricmp("dirt_texture", firstword(pszLine)) == 0) {
            int nValue;
            static_cast<void>(sscanf(pszLine + __strlen_chk("dirt_texture", sizeof("dirt_texture")), "%d", &nValue));
            m_nDirtTexture = static_cast<unsigned short>(nValue);
        }
        if (_stricmp("dirt_worldspace", firstword(pszLine)) == 0) {
            int nValue;
            static_cast<void>(sscanf(pszLine + __strlen_chk("dirt_worldspace", sizeof("dirt_worldspace")), "%d",
                   &nValue));
            m_nDirtWorldspace = static_cast<unsigned short>(nValue);
        }
        if (_stricmp("hologram_donotdraw", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("hologram_donotdraw", sizeof("hologram_donotdraw")),
                  m_bHologramDoNotDraw);
#endif

        // The two animatable-colour groups.
        if (_stricmp("selfillumcolor", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(__strlen_chk("selfillumcolor", sizeof("selfillumcolor")));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_vSelfIllumColor)), reinterpret_cast<void*>(-1));
        } else if (_stricmp("selfillumcolorkey", firstword(pszLine)) == 0) {
            AddGenericController(const_cast<char*>("selfillumcolorkey"),
                                 pszLine + __strlen_chk("selfillumcolorkey",
                                                        sizeof("selfillumcolorkey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_vSelfIllumColor)), reinterpret_cast<void*>(-1));
        } else if (_stricmp("selfillumcolorbezierkey", firstword(pszLine)) == 0) {
            AddBezierController(
                firstword(pszLine),
                pszLine + __strlen_chk("selfillumcolorbezierkey",
                                       sizeof("selfillumcolorbezierkey")),
                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_vSelfIllumColor)), reinterpret_cast<void*>(-1));
        }

        if (_stricmp("alpha", firstword(pszLine)) == 0) {
            int64_t nName = static_cast<int64_t>(__strlen_chk("alpha", sizeof("alpha")));
            AddGenericControllerSingle(reinterpret_cast<char*>(nName), pszLine + nName, m_listControllers,
                                       m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_fOpacity)), reinterpret_cast<void*>(-1));
        } else if (_stricmp("alphakey", firstword(pszLine)) == 0) {
            AddGenericController(const_cast<char*>("alphakey"),
                                 pszLine + __strlen_chk("alphakey", sizeof("alphakey")),
                                 m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_fOpacity)), reinterpret_cast<void*>(-1));
        } else if (_stricmp("alphabezierkey", firstword(pszLine)) == 0) {
            AddBezierController(firstword(pszLine),
                                pszLine + __strlen_chk("alphabezierkey", sizeof("alphabezierkey")),
                                m_listControllers, m_listFloats, reinterpret_cast<void*>(K2_PART_OFFSET(PartTriMesh, m_fOpacity)), reinterpret_cast<void*>(-1));
        }
    }
    MdlNode::InternalParseField(pszLine);
}

void MdlNodeLightsaber::InternalParseField(char* pszLine)
{
    MdlNodeTriMesh::InternalParseField(pszLine);
}

// Each field test re-reads firstword(pszLine) and strips the name with strlen, so the line must be
// exactly "<field> <value>".
void MdlNodeDanglyMesh::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("displacement", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("displacement", sizeof("displacement")), "%f",
                   &m_fDisplacement));
        if (_stricmp("period", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("period", sizeof("period")), "%f", &m_fPeriod));
        if (_stricmp("tightness", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("tightness", sizeof("tightness")), "%f",
                   &m_fTightness));
        if (_stricmp("constraints", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("constraints", sizeof("constraints")), m_lConstraints);
    }
    MdlNodeTriMesh::InternalParseField(pszLine);
}


void MdlNodeAnimMesh::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("sampleperiod", firstword(pszLine)) == 0)
            static_cast<void>(sscanf(pszLine + __strlen_chk("sampleperiod", sizeof("sampleperiod")), "%f",
                   &m_fSamplePeriod));
        if (_stricmp("animverts", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("animverts", sizeof("animverts")), m_lAnimVerts);
        if (_stricmp("animtverts", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("animtverts", sizeof("animtverts")), m_lAnimTVerts);
    }
    MdlNodeTriMesh::InternalParseField(pszLine);
}


void MdlNodeSkin::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("weights", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("weights", sizeof("weights")), m_lWeights);
        if (_stricmp("qbone_ref_inv", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("qbone_ref_inv", sizeof("qbone_ref_inv")),
                  m_lQBoneRefInv);
        if (_stricmp("tbone_ref_inv", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("tbone_ref_inv", sizeof("tbone_ref_inv")), m_lTBoneRefInv);
        if (_stricmp("boneconstantindices", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("boneconstantindices", sizeof("boneconstantindices")),
                  m_lBoneConstantIndices);
    }
    MdlNodeTriMesh::InternalParseField(pszLine);
}

void MdlNodeAABB::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("aabb", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("aabb", sizeof("aabb")), m_pAABB);
    }
    MdlNodeTriMesh::InternalParseField(pszLine);
}

// MaxTree is the .mdl file's root record, so it prefix-matches the line rather than using firstword.
void MaxTree::InternalParseField(char* pszLine)
{
    if (strncmp(pszLine, "node", __strlen_chk("node", sizeof("node"))) == 0)
        ParseNode(pszLine + __strlen_chk("node", sizeof("node")), this);
}

void Model::InternalParseField(char* pszLine)
{
    if (_stricmp("radius", firstword(pszLine)) == 0)
        static_cast<void>(sscanf(pszLine + __strlen_chk("radius", sizeof("radius")), "%f", &m_fRadius));
    if (_stricmp("bmin", firstword(pszLine)) == 0)
        static_cast<void>(sscanf(pszLine + __strlen_chk("bmin", sizeof("bmin")), "%f %f %f",
               &m_vBoundsMin.x, &m_vBoundsMin.y, &m_vBoundsMin.z));
    if (_stricmp("bmax", firstword(pszLine)) == 0)
        static_cast<void>(sscanf(pszLine + __strlen_chk("bmax", sizeof("bmax")), "%f %f %f",
               &m_vBoundsMax.x, &m_vBoundsMax.y, &m_vBoundsMax.z));
    if (strncmp(pszLine, "node", __strlen_chk("node", sizeof("node"))) == 0)
        ParseNode(pszLine + __strlen_chk("node", sizeof("node")), this);
}

// ---- the .mdl file-level callbacks --------------------------------------------------------------
// The free functions the .mdl token parser calls back into while a model is being built.
Model* currentmodel;

// Walks the resource line by line and hands each one to the tree's parse trampoline, stopping at a
// `done` or `end` line. The sModelAnimationRootName line is the one this function consumes itself.
//
// The first parameter is never read by the body, but it stays in the signature because both call
// sites pass two arguments.
void ImportMaxTree(char* pszName, MaxTree* pTree)
{
    K2_UNUSED(pszName);
    char szRootName[64];
    char* pszLine = AurResGetNextLine();
    while (pszLine != 0) {
        pszLine = skipspace(pszLine);
        if (strncmp(pszLine, "done", __strlen_chk("done", sizeof("done"))) == 0)
            break;
        if (strncmp(pszLine, "end", __strlen_chk("end", sizeof("end"))) == 0)
            break;
        if (_stricmp("sModelAnimationRootName", firstword(pszLine)) == 0) {
            if (sscanf(pszLine + __strlen_chk("sModelAnimationRootName", sizeof("sModelAnimationRootName")),
                       "%s", szRootName) == 0)
                szRootName[0] = 0;
        }
        pTree->m_pfnParseField(pTree, pszLine);
        pszLine = AurResGetNextLine();
    }
    Model* pModel = pTree->AsModel();
    if (pModel != 0) {
        pModel->m_pMdlNode->ReorderChildren();
        // The root pointer is written through one local so the field address is formed once.
        MdlNode** ppRoot = &pModel->m_pAnimationRoot;
        if (pTree == 0)
            *ppRoot = 0;
        else {
            *ppRoot = FindMdlNode(pTree->m_pMdlNode, szRootName);
            if (*ppRoot != 0)
                return;
        }
        *ppRoot = pModel->m_pMdlNode;
    }
}

void BinaryInsertModel(Model* pModel, int nLow, int nHigh);

// The name is parsed, the Animation is built on it and appended to the model's list, and the tree is
// handed to the line reader.
void newanim(char* pszName)
{
    char szName[64];
    char szParam[64];
    static_cast<void>(sscanf(pszName, "%s %s", szName, szParam));
    Animation* pAnim = new Animation(szName);
    currentmodel->m_animations.Add(pAnim);
    pAnim->m_pModel = currentmodel;
    ImportMaxTree(nullptr, pAnim);
    pAnim->PostProcess();
    pAnim->SynchronizeTree(currentmodel);
}

void donemodel(char* pszName)
{
    K2_UNUSED(pszName);
}

// Allocates the model; its tree is filled by beginmodelgeom below.
void newmodel(char* pszName)
{
    currentmodel = new Model(pszName);
}

// currentmodel is re-read after every call, because the callbacks and SynchronizeTree can move it.
void beginmodelgeom(char* pszName)
{
    if (currentmodel->m_pSuperModel)
        currentmodel->m_nNodeCount = 1 + currentmodel->m_pSuperModel->m_nNodeCount;
    ImportMaxTree(pszName, currentmodel);
    currentmodel->SynchronizeTree(currentmodel->m_pSuperModel);
}

// The forcedistortion test is not part of the chain. Its arm falls through into the next test, so a
// forcedistortion line is still tested against every other keyword. The other ten arms end the
// classification, which is what gives them one shared tail.
// The effect, effects and track keywords all set bit 0.
void classification(char* pszName)
{
    if (_stricmp(pszName, "forcedistortion") == 0)
        currentmodel->m_nFlags |= Model::kModelForceDistortion;
    if (_stricmp(pszName, "heatdistortion") == 0)
        currentmodel->m_nFlags |= Model::kModelHeatDistortion;
    else if (_stricmp(pszName, "character") == 0)
        currentmodel->m_nFlags |= Model::kModelCharacter;
    else if (_stricmp(pszName, "ambient") == 0)
        currentmodel->m_nFlags |= Model::kModelAmbient;
    else if (_stricmp(pszName, "effect") == 0)
        currentmodel->m_nFlags |= Model::kModelEffect;
    else if (_stricmp(pszName, "effects") == 0)
        currentmodel->m_nFlags |= Model::kModelEffect;
    else if (_stricmp(pszName, "track") == 0)
        currentmodel->m_nFlags |= Model::kModelEffect;
    else if (_stricmp(pszName, "door") == 0)
        currentmodel->m_nFlags |= Model::kModelDoor;
    else if (_stricmp(pszName, "lightsaber") == 0)
        currentmodel->m_nFlags |= Model::kModelLightsaber;
    else if (_stricmp(pszName, "item") == 0)
        currentmodel->m_nFlags |= Model::kModelItem;
    else if (_stricmp(pszName, "placeable") == 0)
        currentmodel->m_nFlags |= Model::kModelPlaceable | Model::kModelItem;
}

// The one classification keyword that is not a plain flag test.
void secondaryclassification(char* pszName)
{
    if (_stricmp(pszName, "sensitive") == 0)
        currentmodel->m_nFlags |= Model::kModelSensitive;
}

// nValue is only written by the scan, so a failed parse leaves it uninitialised.
void ignorefog(char* pszName)
{
    int nValue;
    static_cast<void>(sscanf(pszName, "%d", &nValue));
    if (nValue == 1)
        currentmodel->m_cAffectedByFog = 0;
}

// The model table is kept sorted by name, so all three searches compare through Model::m_szName.
// Written recursively, so each arm keeps its own range test.
Model* BinarySearchModel(char* pszName, int nLow, int nHigh)
{
    if (nHigh < nLow)
        return 0;
    int mid = (nLow + nHigh) / 2;
    int nCmp = _stricmp(pszName, Models.m_pData[mid]->m_szName);
    if (nCmp == 0)
        return Models.m_pData[mid];
    if (nCmp < 0)
        return BinarySearchModel(pszName, nLow, mid - 1);
    return BinarySearchModel(pszName, mid + 1, nHigh);
}

// The same search over the whole table.
Model* BinaryFindModel(char* pszName)
{
    return BinarySearchModel(pszName, 0, Models.m_count - 1);
}

// Does the sorted insert the model table is built with. It finds the first slot whose name is not
// less than the model's over [nLow, nHigh] and inserts there. InsertModel reaches it with the whole
// table as the range.
//
// The range test is repeated inside each arm, which is the only spelling that keeps two comparison
// blocks instead of one select pair.
// Recursive, like BinarySearchModel.
void BinaryInsertModel(Model* pModel, int nLow, int nHigh)
{
    if (nHigh < nLow) {
        Models.Insert(pModel, nLow);
        return;
    }
    int mid = (nLow + nHigh) / 2;
    if (_stricmp(pModel->m_szName, Models.m_pData[mid]->m_szName) < 0)
        BinaryInsertModel(pModel, nLow, mid - 1);
    else
        BinaryInsertModel(pModel, mid + 1, nHigh);
}

// Appends the model through the sorted-insert helper over the whole list. Kept out of line: its one
// caller must keep a real call.
__attribute__((noinline))
void InsertModel(Model* pModel)
{
    BinaryInsertModel(pModel, 0, Models.m_count - 1);
}

// Model lookup by name. On a miss the model file is read through IODispatcher with currentmodel
// temporarily zeroed, and the freshly parsed tree is returned. A name mismatch after a successful
// load is logged to AR_ERROR.LOG.
//
// currentmodel is zeroed across the load so the parser never sees the caller's model, then restored.
// NULL_NAME is a real string the callers pass to mean "no model", not a null test.
Model* FindModel(char* pszName)
{
    if (pszName == 0)
        return 0;
#if KOTOR1  // K1 spells the no-model sentinel inline
    if (_stricmp(pszName, "NULL") == 0)
        return 0;
#else
    if (_stricmp(pszName, NULL_NAME) == 0)
        return 0;
#endif
    Model* pModel = BinaryFindModel(pszName);
    if (pModel == 0) {
        Model* pSaved = currentmodel;
        currentmodel = 0;
        MaxTree* pTree = IODispatcher::GetRef()->ReadSync(pszName);
        if (pTree != 0) {
            currentmodel = pTree->AsModel();
            pModel = currentmodel;
        } else
            pModel = currentmodel;
        currentmodel = pSaved;
        if (pModel == 0)
            return 0;
        if (_stricmp(pModel->m_szName, pszName) != 0) {
#if KOTOR1  // K1 writes the log through SDL_RWops, with no null test
            SDL_RWops* pFile = SDL_RWFromFile("AR_ERROR.LOG", "w");
#ifdef K2_FIX_ENGINE_BUGS
            if (pFile != 0)
#endif
            {
                char szBuf[256];
                int nLen = FormatName(szBuf, 256, "Names Differ: %s %s", pModel->m_szName, pszName);
                SDL_RWwrite(pFile, szBuf, 1, nLen);
                SDL_RWclose(pFile);
            }
#else
            FILE* pFile = fopen("AR_ERROR.LOG", "w");
            if (pFile != 0) {
                char szBuf[256];
                FormatName(szBuf, 256, "Names Differ: %s %s", pModel->m_szName, pszName);
                fprintf(pFile, "Names Differ: %s %s", pModel->m_szName, pszName);
                fclose(pFile);
            }
#endif
        }
    }
    return pModel;
}

#if KOTOR1
Model* LoadModel(SDL_RWops* pFile, SDL_RWops* pAux)
#else
// Loads a .mdl/.max model. Its last act is a registry de-duplication: if a model with the same name
// is already in Models, the freshly parsed one is torn down and the registered one returned
// instead, so two Gobs sharing a resref end up sharing one Model and one refcount.
//
// The null test on pFile does not touch currentmodel; the save, clear and restore are entirely
// inside the non-null path, and the de-duplication scan reads the model count afresh in its
// condition rather than caching it.
Model* LoadModel(FILE* pFile, FILE* pAux)
#endif
{
    if (pFile == 0)
        return 0;

    Model* pSaved = currentmodel;
    currentmodel = 0;

    MaxTree* pTree = IODispatcher::GetRef()->ReadSync(pFile, pAux);
    if (pTree != 0)
        currentmodel = pTree->AsModel();

    Model* pModel = currentmodel;
    currentmodel = pSaved;

    if (pModel == 0)
        return 0;

    // The index is 64-bit, so the element load needs no sign extension.
    for (int64_t i = 0; i < Models.m_count; i++)
    {
        Model* pOther = Models.m_pData[i];
        if (pOther != pModel && _stricmp(pOther->m_szName, pModel->m_szName) == 0)
        {
            // The explicit destructor call plus a bare operator delete avoids the null check a
            // `delete` would emit.
            pModel->~Model();
#if KOTOR1  // K1 uses the class-scope MaxTree::operator delete, not the global free
            MaxTree::operator delete(pModel);
#else
            operator delete(pModel);
#endif
            return Models.m_pData[i];
        }
    }
    return pModel;
}

// Binds an already-loaded model as the current one's animation supermodel.
//
// The first %s goes into a buffer that is never read; the second is the model name. FindModel
// returns currentmodel itself when the name is not registered, so the result is stored through a
// select rather than a null test. currentmodel is re-read after every call, and the two ref calls
// take the current model as a never-read argument and the supermodel as their receiver.
char* setsupermodel(char* pszArgs)
{
    if (lastsupermodelname == 0 && uselastsupermodelname != 0)
        lastsupermodelname = new char[64];

    // The unused buffer is declared first and so lands at the higher address; the frame size depends
    // on both buffers existing.
    char szUnused[64];
    char szModel[64];
    static_cast<void>(sscanf(pszArgs, "%s%s", szUnused, szModel));

    if (currentmodel->m_pSuperModel != 0)
        currentmodel->m_pSuperModel->DelRef(currentmodel);
    __strcpy_chk(currentmodel->m_szSuperModelName, szModel, sizeof(currentmodel->m_szSuperModelName));

    Model* pFound = FindModel(szModel);
    currentmodel->m_pSuperModel = (pFound == currentmodel) ? 0 : pFound;

    if (lastsupermodelname != 0)
        strcpy(lastsupermodelname, szModel);
    if (currentmodel->m_pSuperModel != 0)
        currentmodel->m_pSuperModel->AddRef(currentmodel);

    return const_cast<char*>("done");
}

// The local is seeded to 1.0f and its address is handed to sscanf, so a malformed line leaves the
// scale unchanged; the store goes back through the local.
char* setanimationscale(char* pszArgs)
{
    float fScale = 1.0f;
    static_cast<void>(sscanf(pszArgs, "%f", &fScale));
    currentmodel->m_fAnimScale = fScale;
    return const_cast<char*>("done");
}

// ---- List<T>::allocate instantiations -----------------------------------------------------------
template void List<MaxFace_PostProcess>::allocate(int);
template void List<MaxFace>::allocate(int);

// The post-parse half of Animation, MaxTree's animation-flavoured subclass.

// The animation half of the parse-trampoline pair; defined beside the console handlers in
// GobAnimation.cpp.
void AnimationParseField(void* pAnimation, char* pszLine);

// ==== the post-parse fixup =======================================================================

// Scene.cpp's node lookup, needed to resolve `animroot <name>` against the node hierarchy.
MdlNode* findnode(MdlNode* pNode, char* pszName);

int pruneanimations = 1;

int countanimnodes;
int countprunedanimnodes;

// Walks the whole subtree and, on the way out, deletes every leaf that is neither the animation's own
// root node nor still holding children or controllers. countprunedanimnodes counts the ones actually
// freed.
void Animation::Prune(MdlNode* pNode)
{
    countanimnodes++;

    int64_t i = pNode->m_listChildren.m_count;
    while (i--)
        Prune(pNode->m_listChildren.m_pData[i]);

    if (m_pMdlNode == pNode)
        return;
    if (pNode->m_listChildren.m_count)
        return;
    if (pNode->m_listControllers.m_count)
        return;

    reinterpret_cast<MdlNode*>(pNode->m_pParent)->m_listChildren.Remove(pNode);
    delete pNode;

    countprunedanimnodes++;
}

// Leaves the node hierarchy holding only the chain from the tree's root down to the node the .mdl's
// `animroot` field named: it finds that node by name, then walks up the parent links and, at every
// ancestor, erases and deletes every child except the one it arrived from. Every ancestor's
// controller list is dropped along the way; the animroot node's own is not.
//
// The animroot string's first byte doubles as the `sscanf("%s")` failure flag, and a node tree is
// only pruned for a real name.
void Animation::PostProcess()
{
    char* pszAnimRoot = m_szAnimRoot;
    if (!pszAnimRoot[0])
        return;
    if (_stricmp(pszAnimRoot, NULL_NAME) == 0)
        return;

    MdlNode* pNode = findnode(m_pMdlNode, pszAnimRoot);
    if (!pNode)
        return;

    MdlNode* pChild = pNode;
    MdlNode* pParent = reinterpret_cast<MdlNode*>(pChild->m_pParent);
    while (pParent) {
        pParent->m_listControllers.SetSize(0);

        int i = pParent->m_listChildren.m_count;
        while (i--) {
            MdlNode* pTemp = pParent->m_listChildren.m_pData[i];
            if (pTemp == pChild)
                continue;
            pParent->m_listChildren.DelIndex(i);
            delete pTemp;
        }

        pChild = pParent;
        pParent = reinterpret_cast<MdlNode*>(pChild->m_pParent);
    }

    if (pruneanimations)
        Prune(m_pMdlNode);

    if (m_fLength < 0.0f)
        m_fLength = 0.0f;
}

// ==== the .mdl text-field parser ==================================================================

// The two small Parse bodies are duplicated here so this unit keeps its own copies; keep them as
// calls rather than hand-expanded bodies.
static inline void ParseFloatInlined(char* pszLine, float& fValue)
{
    static_cast<void>(sscanf(pszLine, "%f", &fValue));
}

static inline void ParseStringInlined(char* pszLine, char* pszValue)
{
    if (!sscanf(pszLine, "%s", pszValue))
        *pszValue = 0;
}

// The animation tree's field reader. Six independent ifs rather than an else-if chain, opened by a
// strncmp pair rather than a firstword test, and the event arm carries a list insert instead of a
// controller add. The strncmp argument order differs between the animlength/event arms and the node
// arm.
//
// The event arm's byte offset lives in its own block so it shares the name scratch slot.
void Animation::InternalParseField(char* pszLine)
{
    if (!strncmp("animlength", pszLine, __strlen_chk("animlength", sizeof("animlength")))) {
        char szName[128];
        static_cast<void>(sscanf(pszLine + __strlen_chk("animlength", sizeof("animlength")), "%s %f",
               szName, &m_fLength));
    }

    if (!strncmp("event", pszLine, __strlen_chk("event", sizeof("event")))) {
        AnimationEvent ev;
        ev.m_szName[0] = 0;
        char* pszValue = pszLine + __strlen_chk("event", sizeof("event"));
        {
            int nOffset;
            static_cast<void>(sscanf(pszValue, "%f%n", &ev.m_fTime, &nOffset));
            ParseStringInlined(skipspace(pszValue + nOffset), ev.m_szName);
        }

        // Kept sorted by time: walk to the first entry the new event precedes (or the end).
        int nIndex = 0;
        while (nIndex < m_events.m_count && ev.m_fTime > m_events.m_pData[nIndex].m_fTime)
            nIndex++;
        m_events.Insert(ev, nIndex);
    }

    if (_stricmp("length", firstword(pszLine)) == 0)
        ParseFloatInlined(pszLine + __strlen_chk("length", sizeof("length")), m_fLength);

    if (_stricmp("transtime", firstword(pszLine)) == 0)
        ParseFloatInlined(pszLine + __strlen_chk("transtime", sizeof("transtime")), m_fTransTime);

    if (_stricmp("animroot", firstword(pszLine)) == 0)
        ParseStringInlined(pszLine + __strlen_chk("animroot", sizeof("animroot")), m_szAnimRoot);

    // `node <name>` recurses into the node-tree reader, which owns the new node.
    if (!strncmp(pszLine, "node", __strlen_chk("node", sizeof("node"))))
        ParseNode(pszLine + __strlen_chk("node", sizeof("node")), this);
}

// The camera node's parser hook; the rest of the class's bodies live in the other family units.

void MdlNodeCamera::InternalParseField(char* pszLine)
{
    MdlNode::InternalParseField(pszLine);
}

// The reference node's parser hook.

// The reattachable arm accepts a number or the word true/false, which needs a scratch buffer and
// gives this function its stack canary. The key offset is computed once and reused by both scans.
void MdlNodeReference::InternalParseField(char* pszLine)
{
    if (!disablepostprocessing) {
        if (_stricmp("refModel", firstword(pszLine)) == 0) {
            if (sscanf(pszLine + __strlen_chk("refModel", sizeof("refModel")), "%s",
                       m_szRefModel) == 0)
                m_szRefModel[0] = 0;
        }
        if (_stricmp("reattachable", firstword(pszLine)) == 0) {
            char* pszValue = pszLine + __strlen_chk("reattachable", sizeof("reattachable"));
            if (sscanf(pszValue, "%d", &m_bReattachable) == 0) {
                char szValue[64];
                szValue[0] = 0;
                static_cast<void>(sscanf(pszValue, "%s", szValue));
                // Two independent tests, not an else-if; the false test runs unconditionally.
                if (_stricmp("TRUE", szValue) == 0)
                    m_bReattachable = 1;
                if (_stricmp("FALSE", szValue) == 0)
                    m_bReattachable = 0;
            }
        }
    }
    MdlNode::InternalParseField(pszLine);
}
