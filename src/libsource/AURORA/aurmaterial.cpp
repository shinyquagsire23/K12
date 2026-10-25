// The AURORA per-mesh material. This TU also owns the `materials` registry every Material
// constructor registers itself into.
#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurres.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurmaterial.h"
#include "nullname.h"

#include "aurtexman.h"
#include "aurtexture.h"
#include "AURORA/aurmodel.h"
#include "auremitter.h"

CAurTexture* AurTextureGetNULL();
int AurTextureMessage(CAurTexture* pTexture, char* pszLine);
extern int enablebumpmap;

CAurTexture* AurTextureGetReference(char* pszName, char* pszExtension);
void AurTextureRelease(CAurTexture* pTexture);
int AurTextureReplaceSingleReference(CAurTexture* pTexture, char* pszName, char* pszFilter,
                                     char* pszResRef, int* pnOffset, int nType);
void AurTextureSetLayered(CAurTexture* pTexture, char* pszName, int* pnOffset, int nType);

// The GL surface the three blend-state rows program, plus the tables they translate through.
const float* Homogenize(const Vector& v, const float& w);
extern unsigned int blendTranslationTable[];
extern const unsigned int cmpTranslationTable[];
extern const int facingTranslationTable[3];
extern const unsigned int materialTranslationTable[];
extern float g_fGlobalAlpha;
int usenewalpha = 1;
float materialalpha = 0.349999994f;
extern unsigned char g_useDistortionOutput;
#if KOTOR1
extern bool s_useAlphaTestShader;
#endif
#include "port/libcbase.h"
#include "port/libcstring.h"

#include "port/win32api.h"
#include "port/crtapi.h"

char* firstword(char* pszLine);
void Parse(char* pszLine, char* pszValue);
void Parse(char* pszLine, bool& bValue);
void Parse(char* pszLine, int& nValue);
void Parse(char* pszLine, float& fValue);

List<Material*> materials;

// Size-checked sprintf.
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

int Material::GetTextureTID() {
    return AurTextureGetTID(m_spr0.m_pObject);
}

bool Material::HasNormalMap() {
    return false;
}

int Material::GetBumpMapTID() {
    return AurTextureGetTID(m_spr4.m_pObject);
}

// The two stage-5 lookups read the same watcher under two names.
int Material::GetBumpyShinyMapTID() {
    return AurTextureGetTID(m_spr5.m_pObject);
}

int Material::GetEnvMapTID() {
    return AurTextureGetTID(m_spr5.m_pObject);
}

// Reports an unusable texture and drops it; the watcher itself is left alone.
void Material::PurgeBadTexture(SafePointer<CAurTexture>& spr) {
    CAurTexture* pTexture = spr.m_pObject->GetReferencedTexture();
    if (pTexture == 0) {
        AurTextureGetNULL();
    } else if (pTexture->IsInvalid()) {
        AurTextureGetNULL();
        AurTextureRelease(pTexture);
    }
}

// Looks a texture up by name; an empty name and the "nullptr" sentinel both give the sentinel.
CAurTexture* LoadTexture(char* pszName)
{
    if (pszName == 0 || pszName[0] == 0)
        return AurTextureGetNULL();
    if (strcmp(NULL_NAME, pszName) == 0)
        return AurTextureGetNULL();
    return AurTextureGetReference(pszName, 0);
}

// MessageBoxA's style bits, as winuser.h names them (MB_SYSTEMMODAL, MB_ICONEXCLAMATION).
enum { kMbIconExclamation = 0x30, kMbSystemModal = 0x1000 };

// Loads the seven texture stages: the four name parameters fill the first four watchers and the
// three .txi name buffers ParseField filled fill the last three, each buffer freed as it is used.
// EFFECTIVE: clang tail-duplicates one AurTextureGetNULL() arm (5 call sites, the orig 4); same calls on every path.
void Material::InitializeTextures(char* pszTexture, char* p2, char* p3, char* p4)
{
    if (pszTexture != 0 && pszTexture[0] != 0 && strcmp(NULL_NAME, pszTexture) != 0)
        m_spr0 = AurTextureGetReference(pszTexture, 0);
    else
        m_spr0 = AurTextureGetNULL();
    if (pszTexture != 0 && pszTexture[0] != 0 && strcmp(NULL_NAME, pszTexture) != 0)
        Init(pszTexture);

    if (p2 != 0 && p2[0] != 0 && strcmp(NULL_NAME, p2) != 0)
        m_spr1 = AurTextureGetReference(p2, 0);
    else
        m_spr1 = AurTextureGetNULL();
    if (p3 != 0 && p3[0] != 0 && strcmp(NULL_NAME, p3) != 0)
        m_spr2 = AurTextureGetReference(p3, 0);
    else
        m_spr2 = AurTextureGetNULL();
    if (p4 != 0 && p4[0] != 0 && strcmp(NULL_NAME, p4) != 0)
        m_spr3 = AurTextureGetReference(p4, 0);
    else
        m_spr3 = AurTextureGetNULL();

    if (m_apszMapNames[0] != 0) {
        if (m_apszMapNames[0][0] != 0 && strcmp(NULL_NAME, m_apszMapNames[0]) != 0)
            m_spr4 = AurTextureGetReference(m_apszMapNames[0], 0);
        delete[] m_apszMapNames[0];
        m_apszMapNames[0] = 0;
    }
    if (m_apszMapNames[1] != 0) {
        if (m_apszMapNames[1][0] != 0 && strcmp(NULL_NAME, m_apszMapNames[1]) != 0) {
            m_spr5 = AurTextureGetReference(m_apszMapNames[1], 0);
            m_spr5.m_pObject->GetReferencedTexture()->m_bUsedAsEnvMap = 1;
        }
        delete[] m_apszMapNames[1];
        m_apszMapNames[1] = 0;
    }
    if (m_apszMapNames[2] != 0) {
        if (m_apszMapNames[2][0] != 0 && strcmp(NULL_NAME, m_apszMapNames[2]) != 0) {
            m_spr5 = AurTextureGetReference(m_apszMapNames[2], 0);
            m_spr5.m_pObject->GetReferencedTexture()->m_bUsedAsEnvMap = 1;
        }
        m_spr0.m_pObject->SetIsEnvironmentMapped(true);
        delete[] m_apszMapNames[2];
        m_apszMapNames[2] = 0;
    }
}

// The Bind* family purges the stage, binds it and reports its texture id. BindTexture0 can be
// redirected to stage 6 by the ini's bump-map switch; BindBumpMap skips the purge.
void Material::BindTexture0() {
    PurgeBadTexture(m_spr0);
    CAurTexture* pTexture =
        (enablebumpmap && m_spr6.m_pObject != 0) ? m_spr6.m_pObject : m_spr0.m_pObject;
    pTexture->Bind();
    AurTextureGetTID(m_spr0.m_pObject);
}

void Material::BindTexture1() {
    PurgeBadTexture(m_spr1);
    m_spr1.m_pObject->Bind();
    AurTextureGetTID(m_spr1.m_pObject);
}

void Material::BindTexture2() {
    PurgeBadTexture(m_spr2);
    m_spr2.m_pObject->Bind();
    AurTextureGetTID(m_spr2.m_pObject);
}

void Material::BindTexture3() {
    PurgeBadTexture(m_spr3);
    m_spr3.m_pObject->Bind();
    AurTextureGetTID(m_spr3.m_pObject);
}

void Material::BindBumpMap() {
    m_spr4.m_pObject->Bind();
    AurTextureGetTID(m_spr4.m_pObject);
}

void Material::BindBumpyShinyMap() {
    PurgeBadTexture(m_spr5);
    m_spr5.m_pObject->Bind();
    AurTextureGetTID(m_spr5.m_pObject);
}

void Material::BindEnvMap() {
    PurgeBadTexture(m_spr5);
    m_spr5.m_pObject->Bind();
    AurTextureGetTID(m_spr5.m_pObject);
}

int Material::BumpMapIsDiffuse() {
    return m_spr4.m_pObject != 0 && m_spr4.m_pObject->IsDiffuseBumpMap();
}

int Material::BumpMapIsSpecular() {
    return m_spr4.m_pObject != 0 && m_spr4.m_pObject->IsSpecularBumpMap();
}

bool Material::HasBumpyShinyMap() {
    if (m_spr5.m_pObject != 0 && m_spr5.m_pObject->IsCubeMap())
        return true;
    return false;
}

bool Material::HasEnvMap() {
#if KOTOR1  // K1 inlines the sentinel test instead of calling CAurTexture::IsNull
    if (m_spr5.m_pObject != 0 &&
        m_spr5.m_pObject->GetReferencedTexture() != CAurTextureBasic::m_pNullTexture) {
#else
    if (m_spr5.m_pObject != 0 && !m_spr5.m_pObject->IsNull()) {
#endif
        if (!m_spr5.m_pObject->IsInvalid())
            return true;
    }
    return false;
}

// Reports the setting to the texture's field-parse log.
void Material::SetDownSampling(int nDownSample) {
    char szBuffer[20];
    sprintf_s(szBuffer, sizeof(szBuffer), "downsamplemax %d", nDownSample);
    AurTextureMessage(m_spr0.m_pObject, szBuffer);
}

// Takes the texture and lightmap names off whichever node kind the part is.
void Material::LoadTextures(Part* part) {
    char* pszTexture = 0;
    char* pszLightmap = 0;
    PartTriMesh* pMesh = part->AsPartTriMesh();
    PartEmitter* pEmitter = part->AsPartEmitter();
    if (pMesh != 0) {
        MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(pMesh->m_pNode);
        pszTexture = pNode->m_szTexture;
        pszLightmap = pNode->m_szLightmap;
    } else if (pEmitter != 0) {
        MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(pEmitter->m_pNode);
        pszTexture = pNode->m_szTexture;
    } else {
        return;
    }
    InitializeTextures(pszTexture, pszLightmap, 0, 0);
}

// Retargets the bound texture, first when it is the sentinel and then by name, and re-rasters the
// layered result.
void Material::ReplaceTexture(char* pszPublicName, char* pszTextureName, char* pszTexturePath,
                              int* paSizes, int nSizes) {
#if KOTOR1
    if (m_spr0.m_pObject->GetReferencedTexture() == CAurTextureBasic::m_pNullTexture) {
#else
    if (m_spr0.m_pObject->IsNull()) {
#endif
        if (pszTexturePath == 0)
            return;
        CAurTexture* pOld = m_spr0.m_pObject;
        m_spr0 = AurTextureGetReference(pszTexturePath, 0);
        AurTextureRelease(pOld);
    }
    if (pszTextureName != 0 && strlen(pszTextureName) != 0) {
        if (!AurTextureReplaceSingleReference(m_spr0.m_pObject, pszPublicName, pszTextureName,
                                              pszTextureName, paSizes, nSizes))
            return;
    }
    AurTextureSetLayered(m_spr0.m_pObject, pszTextureName, paSizes, nSizes);
}

// The two-name variant. A bound texture named "P_BaoDur_Arm" is never replaced.
void Material::ReplaceTexture(char* pszPublicName, char* pszTextureName) {
#if KOTOR1  // K1 has no "P_BaoDur_Arm" exemption
    if (m_spr0.m_pObject == 0 ||
        m_spr0.m_pObject->GetReferencedTexture() == CAurTextureBasic::m_pNullTexture) {
        if (pszTextureName == 0)
            return;
        CAurTexture* pOld = m_spr0.m_pObject;
        m_spr0 = AurTextureGetReference(pszPublicName, pszTextureName);
        AurTextureRelease(pOld);
    } else {
        AurTextureReplaceSingleReference(m_spr0.m_pObject, pszPublicName, pszTextureName, 0, 0, 0);
    }
#else
    if (m_spr0.m_pObject == 0 || m_spr0.m_pObject->IsNull()) {
        if (pszTextureName == 0)
            return;
        CAurTexture* pOld = m_spr0.m_pObject;
        m_spr0 = AurTextureGetReference(pszPublicName, pszTextureName);
        AurTextureRelease(pOld);
    } else {
        if (_stricmp(m_spr0.m_pObject->GetName(), "P_BaoDur_Arm") == 0)
            return;
        AurTextureReplaceSingleReference(m_spr0.m_pObject, pszPublicName, pszTextureName, 0, 0, 0);
    }
#endif
}

// A stage-4 texture that is still loading or is itself a bump map passes; anything else is reported
// through a message box and dropped.
bool Material::HasBumpMap() {
    if (m_spr4.m_pObject == 0)
        return false;
    if (!m_spr4.m_pObject->IsFullyLoaded() || m_spr4.m_pObject->IsBumpMap())
        return true;
    char szBuffer[256];
    sprintf_s(szBuffer, sizeof(szBuffer), "Invalid bumpmap: %s.tga",
              m_spr4.m_pObject->GetName());
    MessageBoxA(0, szBuffer, "Invalid Bumpmap!", kMbSystemModal | kMbIconExclamation);
    CAurTexture* pOld = m_spr4.m_pObject;
    m_spr4 = 0;
    AurTextureRelease(pOld);
    return false;
}

// nMode indexes both blend-mode lists.
void Material::SetBlendingMode(int nMode) {
#if KOTOR1  // K1 has no distortion pass; it flags the alpha-test shader for a lit alpha cut
    s_useAlphaTestShader = false;
    ASLgl::glBlendFunc(blendTranslationTable[m_lstSrcBlend.m_pData[nMode]],
                       blendTranslationTable[m_lstDstBlend.m_pData[nMode]]);
    if (static_cast<int>(m_lstSrcBlend.m_pData[nMode]) == 0 && static_cast<int>(m_lstDstBlend.m_pData[nMode]) == 2)
        ASLgl::glDepthMask(0);
    else
        ASLgl::glDepthMask(1);
    if (!usenewalpha)
        return;
    float fAlpha = 0.0f;
    if (static_cast<int>(m_lstSrcBlend.m_pData[nMode]) == 2 && static_cast<int>(m_lstDstBlend.m_pData[nMode]) == 3) {
        fAlpha = materialalpha;
        if (fAlpha > 0.0f)
            s_useAlphaTestShader = true;
    }
    ASLgl::glAlphaFunc(cmpTranslationTable[4], fAlpha);
#else
    ASLgl::glBlendFunc(blendTranslationTable[m_lstSrcBlend.m_pData[nMode]],
                       blendTranslationTable[m_lstDstBlend.m_pData[nMode]]);
    if (static_cast<int>(m_lstSrcBlend.m_pData[nMode]) == 0 && static_cast<int>(m_lstDstBlend.m_pData[nMode]) == 2) {
        if (!g_useDistortionOutput)
            ASLgl::glDepthMask(0);
    } else {
        if (!g_useDistortionOutput)
            ASLgl::glDepthMask(1);
    }
    if (!usenewalpha)
        return;
    float fAlpha = 0.0f;
    if (static_cast<int>(m_lstSrcBlend.m_pData[nMode]) == 2)
        fAlpha = static_cast<int>(m_lstDstBlend.m_pData[nMode]) == 3 ? materialalpha : 0.0f;
    ASLgl::glAlphaFunc(cmpTranslationTable[4], fAlpha);
#endif
}

// The GUI-batch twin of the above over list slot 0; the GUI batch owns the depth mask itself.
void Material::SetGUIBlendingMode() {
    ASLgl::glBlendFunc(blendTranslationTable[m_lstSrcBlend.m_pData[0]],
                       blendTranslationTable[m_lstDstBlend.m_pData[0]]);
    if (!usenewalpha)
        return;
    float fAlpha = 0.0f;
    if (static_cast<int>(m_lstSrcBlend.m_pData[0]) == 2)
        fAlpha = static_cast<int>(m_lstDstBlend.m_pData[0]) == 3 ? materialalpha : 0.0f;
    ASLgl::glAlphaFunc(cmpTranslationTable[4], fAlpha);
}

// A material whose texture is not using global alpha emits nothing.
void Material::SetMaterialColor(Vector vColor, float fAlpha) {
    if (m_spr0.m_pObject->IsUsingGlobalAlpha()) {
        float fScale = g_fGlobalAlpha * fAlpha;
        const float* paColor = Homogenize(vColor, fScale);
#if KOTOR1  // K1 passes GL_FRONT_AND_BACK literally, with no facing table
        ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[2], paColor);
#else
        ASLgl::glMaterialfv(facingTranslationTable[0], materialTranslationTable[2], paColor);
#endif
    }
}

// The blend lists are sized and seeded before the registry entry goes in.
Material::Material()
    : m_apszMapNames {}, m_lstSrcBlend {}, m_lstDstBlend {}, m_bDecal(), m_nRenderBMLMType(), m_fWaterAlpha()
{
    m_lstSrcBlend.allocate(2);
    m_lstDstBlend.allocate(2);
    m_lstSrcBlend.Add(Aurora::AuroraBlendModes(0));
    m_lstDstBlend.Add(Aurora::AuroraBlendModes(1));
    m_bDecal = 0;
    m_nRenderBMLMType = 1;
    materials.Add(this);
    m_fWaterAlpha = 1.0f;
}

Material::~Material()
{
    ClearTextures();
    materials.Remove(this);
}

// Drops every watched texture: unregisters the watcher from the old pointee, nulls the member and
// releases the pointee.
void Material::ClearTextures()
{
    CAurTexture* pTexture;

    pTexture = m_spr0.m_pObject;
    if (pTexture != 0) {
        m_spr0 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr1.m_pObject;
    if (pTexture != 0) {
        m_spr1 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr2.m_pObject;
    if (pTexture != 0) {
        m_spr2 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr3.m_pObject;
    if (pTexture != 0) {
        m_spr3 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr4.m_pObject;
    if (pTexture != 0) {
        m_spr4 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr5.m_pObject;
    if (pTexture != 0) {
        m_spr5 = 0;
        AurTextureRelease(pTexture);
    }
    pTexture = m_spr6.m_pObject;
    if (pTexture != 0) {
        m_spr6 = 0;
        AurTextureRelease(pTexture);
    }
}

// Reads the texture's .txi, either the copy already held on the texture or a fresh container read.
void Material::Init(char* pszName)
{
    int nSize = 0;
#if KOTOR1  // K1 forces two named meshes to src-alpha / one-minus-src-alpha
    if (strstr(pszName, "M36_Shrub")) {
        m_lstSrcBlend.m_pData[0] = static_cast<Aurora::AuroraBlendModes>(2);
        m_lstDstBlend.m_pData[0] = static_cast<Aurora::AuroraBlendModes>(3);
    }
    if (strstr(pszName, "LMA_blas01")) {
        m_lstSrcBlend.m_pData[0] = static_cast<Aurora::AuroraBlendModes>(2);
        m_lstDstBlend.m_pData[0] = static_cast<Aurora::AuroraBlendModes>(3);
    }
#endif

    if (m_spr0 != 0 && m_spr0.m_pObject->IsTxiLoaded()) {
        unsigned char* pTxi = m_spr0.m_pObject->GetTxiData();
        int nLength = m_spr0.m_pObject->GetTxiSize();
        if (pTxi != 0 && nLength >= 1) {
            char szLine[8192];
            int nRemaining = nLength;
            do {
                int64_t i = 0;
                unsigned char c;
                do {
                    c = pTxi[i];
                    szLine[i++] = c;
                } while (i < nRemaining && c != '\n');
                nRemaining -= static_cast<int>(i);
                pTxi += i;
                if (nRemaining < 0)
                    break;
                szLine[i] = 0;
                char* pszLine = szLine;
                while (*pszLine == ' ' || *pszLine == '\t')
                    pszLine++;
                ParseField(pszLine);
            } while (nRemaining > 0);
        }
    } else {
        void* pRes = AurResGet(pszName, const_cast<char*>(".txi"), &nSize, true);
        if (pRes != 0) {
            int nCount = (nSize < 0) ? ~nSize : nSize;
            if (nCount > 0) {
                m_spr0.m_pObject->SetTxiData(AurResGetDataBytes(0, pRes), nCount);
                char* pszLine;
                while ((pszLine = AurResGetNextLine()) != 0) {
                    while (*pszLine == ' ' || *pszLine == '\t')
                        pszLine++;
                    ParseField(pszLine);
                }
            }
            AurResFree(pRes, nSize);
            if (m_spr0 != 0)
                m_spr0.m_pObject->SetTxiLoaded(true);
        } else {
            // A missing .txi still marks the texture loaded, with no null test on m_spr0.
            m_spr0.m_pObject->SetTxiLoaded(true);
        }
    }
}

// Reads one .txi keyword line. The texture-name keywords land in the three name slots, which
// InitializeTextures consumes; "blending" rewrites both blend-mode lists in place.
void Material::ParseField(char* pszLine)
{
    char szBumpMap[32];
    szBumpMap[0] = 0;
    char szBumpyShinyMap[32];
    szBumpyShinyMap[0] = 0;
    char szEnvMap[32];
    szEnvMap[0] = 0;
    char szBlending[32];
    szBlending[0] = 0;

    if (_stricmp("bumpmaptexture", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("bumpmaptexture", sizeof("bumpmaptexture")), szBumpMap);
    if (_stricmp("bumpyshinytexture", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("bumpyshinytexture", sizeof("bumpyshinytexture")), szBumpyShinyMap);
    if (_stricmp("envmaptexture", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("envmaptexture", sizeof("envmaptexture")), szEnvMap);
    if (_stricmp("blending", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("blending", sizeof("blending")), szBlending);
    if (_stricmp("decal", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("decal", sizeof("decal")), m_bDecal);
    if (_stricmp("renderbmlmtype", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("renderbmlmtype", sizeof("renderbmlmtype")), m_nRenderBMLMType);
    if (_stricmp("wateralpha", firstword(pszLine)) == 0)
        Parse(pszLine + __strlen_chk("wateralpha", sizeof("wateralpha")), m_fWaterAlpha);

    if (szBumpMap[0] != 0) {
        m_apszMapNames[0] = new char[32];
        __strcpy_chk(m_apszMapNames[0], szBumpMap, 32);
    }
    if (szBumpyShinyMap[0] != 0) {
        m_apszMapNames[1] = new char[32];
        __strcpy_chk(m_apszMapNames[1], szBumpyShinyMap, 32);
    }
    if (szEnvMap[0] != 0) {
        m_apszMapNames[2] = new char[32];
        __strcpy_chk(m_apszMapNames[2], szEnvMap, 32);
    }
    if (szBlending[0] != 0) {
        if (strcmp(szBlending, "additive") == 0) {
            m_lstSrcBlend.m_pData[0] = Aurora::AuroraBlendModes(0);
            m_lstDstBlend.m_pData[0] = Aurora::AuroraBlendModes(2);
        } else if (strcmp(szBlending, "punchthrough") == 0) {
            m_lstSrcBlend.m_pData[0] = Aurora::AuroraBlendModes(2);
            m_lstDstBlend.m_pData[0] = Aurora::AuroraBlendModes(3);
        }
    }
}
