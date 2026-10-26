
#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurres.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurtexman.h"
#include "nullname.h"

#include <stddef.h>

float g_fDynamicMemoryRatio = 0.5f;
int maxtexmem = 16777216;
float memcheckratio = 0.800000012f;
float maxtimeformemoryoverflow = 50.0f;

bool CAurTextureBasic::m_bLowEndEnvMapEnabled;
unsigned int maxtexid;
bool g_bIsSuspended;
int nocompression;
int texturegettime;

int usedtexturememory;
int useddynamictexturememory;
int usedstatictexturememory;
int usedlightmapmemory;
int usedcomptexmemory;
int usedcubemapmemory;
int usedbumpmapmemory;
int texcurrentframe;
int bindzero;
int countbasictextures;
int constructimagetime;
int glimagetime;
int loadimagetime;
int decodeimagetime;
int basictexturecount;
#if !KOTOR1
bool g_bTextureRefsGotRefreshed;
#endif
float currentoverflow;
float maxdeltat;
int maxtextime;
int currenttextime;
int maxtexloadtime;
int maxdecodetime;
int maxgltime;

#include "port/libcbase.h"
#include <string.h>
#include "port/libcstdlib.h"
#include "port/libcmath.h"
#include "aurtexture.h"

extern List<CAurTextureBasic*> RefreshTextures;

template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item);

int ones32popcount(unsigned int n);

#if !KOTOR1
void AurTextureBindTID(unsigned int nTID, bool bCubeMap, bool bCubeStrip);
#endif

extern int bindzero;

#include "aurtexcontrol.h"
#include "aurglcore.h"
#include <string.h>
#include "aurtexload.h"
#include "Vector.h"
#include "aurcamera.h"
#include "aurimage.h"

int enabletexturecontrollers = 1;

#include "port/crtapi.h"
#include "port/k2qolapi.h"

bool g_bModeIsDynamic = true;

class CAurScene;
class Scene;
void SceneBSPGetTileTextures(Scene* pScene, const Vector& v,
                             List<CAurTexture*>& listTextures);

void AurTextureRelease(CAurTexture* pTexture)
{
    if (pTexture == 0)
        return;
    if (pTexture->m_count != 0)
        return;
    if (pTexture->AsCAurTextureBasic() != 0)
        delete pTexture->AsCAurTextureBasic();
    else
        delete pTexture;
}

CAurTexture::~CAurTexture()
{
    CAurTexture* pOld = m_pReferenced.m_pObject;
    m_pReferenced = 0;
    AurTextureRelease(pOld);
}

void CAurTexture::SetReference(CAurTexture* pReference)
{
#if KOTOR1  // K1 keeps a plain pointer, with no reference tracking
    m_pReferenced = pReference;
#else
    if (m_pReferenced.m_pObject)
        m_pReferenced.m_pObject->Remove(&m_pReferenced);
    m_pReferenced.m_pObject = pReference;
    if (m_pReferenced.m_pObject)
        m_pReferenced.m_pObject->AddPointer(&m_pReferenced);
#endif
}

void CAurTexture::Bind()
{
    static int nCurrentTID;

#if KOTOR1  // K1 binds the null texture separately
    if (bindzero) {
        AurTextureBindTID(0, 0, 0);
        return;
    }
    if (GetReferencedTexture() == CAurTextureBasic::m_pNullTexture) {
        AurTextureBindTID(0, 0, 0);
        return;
    }
#else
    if (bindzero || GetReferencedTexture() == CAurTextureBasic::m_pNullTexture) {
        AurTextureBindTID(0, 0, 0);
        return;
    }
#endif
    CAurTextureBasic* pTexture = AsCAurTextureBasic();
    if (!pTexture)
        pTexture = GetReferencedTexture()->AsCAurTextureBasic();
    if (!pTexture) {
        AurTextureBindTID(0, 0, 0);
        return;
    }
    nCurrentTID = AurTextureGetTID(pTexture);
#if KOTOR1  // K1 has no depth (3D) bind
    AurTextureBindTID(nCurrentTID, pTexture->IsCubeMap());
#else
    if (GetDepth())
        AurTextureBindTID(nCurrentTID, 0, 1);
    else
        AurTextureBindTID(nCurrentTID, pTexture->IsCubeMap(), 0);
#endif
    TextureController* pController
        = pTexture->GetReferencedTexture()->AsCAurTextureBasic()->m_pController;
    if (pController && enabletexturecontrollers)
        pController->Control();
    pTexture->m_bBound = 1;
}

int AurTextureGetTID(CAurTexture* pTexture)
{
    if (pTexture == 0)
        return 0;
    if (pTexture->GetReferencedTexture() == CAurTextureBasic::m_pNullTexture)
        return 0;
    return pTexture->GetReferencedTexture()->AsCAurTextureBasic()->m_nTIDIndex;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
CAurTexture* CAurTexture::GetReferencedTexture()
{
    return m_pReferenced.m_pObject ? m_pReferenced.m_pObject : this;
}
#endif

CAurTextureBasic* CAurTexture::AsCAurTextureBasic() { return 0; }

bool CAurTexture::IsCubeMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bCubeMap != 0;
}

bool CAurTexture::IsFullyLoaded()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bFullyLoaded != 0;
}

bool CAurTexture::IsProcessed()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bProcessed != 0;
}

bool CAurTexture::IsTxiLoaded()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bTxiLoaded != 0;
}

bool CAurTexture::IsEnvironmentMapped()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bEnvironmentMapped != 0;
}

bool CAurTexture::IsUsingGlobalAlpha()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bUsingGlobalAlpha != 0;
}

// No mip maps for the handheld asset prefixes the Android build ships.
bool CAurTexture::GetMipMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    if (strstr(pTexture->m_szName, "ios_main_"))
        return false;
    if (strstr(pTexture->m_szName, "ios_mm_"))
        return false;
    if (strstr(pTexture->m_szName, "ios_tut_"))
        return false;
#if !KOTOR1  // K1 has no MFi artwork exemption
    if (strstr(pTexture->m_szName, "ios_mfi"))
        return false;
#endif
    if (strstr(pTexture->m_szName, "ipho_"))
        return false;
    return pTexture->m_bMipMap != 0;
}

// Border material and every non-power-of-two extent clamps; otherwise wrap.
#if KOTOR1
// K1 clamps the handheld UI textures by name and never tests for powers of two.
int CAurTexture::GetClamp()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    if (strstr(pTexture->m_szName, "boxline"))
        return 3;
    if (strstr(pTexture->m_szName, "lbl_cg_mcg"))
        return 3;
    if (strstr(pTexture->m_szName, "ios_main_"))
        return 3;
    if (strstr(pTexture->m_szName, "ios_mm_"))
        return 3;
    if (strstr(pTexture->m_szName, "ios_tut_"))
        return 3;
    if (strstr(pTexture->m_szName, "ipho_"))
        return 3;
    return pTexture->m_nClamp;
}
#else
int CAurTexture::GetClamp()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    if (!strstr(pTexture->m_szName, "uibit_brdr")) {
        if (ones32popcount(pTexture->m_nWidth) + ones32popcount(pTexture->m_nHeight) <= 2)
            return 0;
    }
    return 3;
}
#endif

bool CAurTexture::GetFilter()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bFilter != 0;
}

bool CAurTexture::GetMapTexelsToPixels()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    return pTexture->m_bMapTexelsToPixels != 0;
}

void CAurTexture::SetAlphaMean(float fAlphaMean)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture != 0 && CAurTextureBasic::m_pNullTexture != this)
        pTexture->m_fAlphaMean = fAlphaMean;
}

void CAurTexture::SetTxiLoaded(bool bValue)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_bTxiLoaded = bValue;
}

void CAurTexture::SetFilter(bool bValue)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_bFilter = bValue;
}

unsigned char* CAurTexture::GetTxiData()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_pTxiData;
}

int CAurTexture::GetTxiSize()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nTxiSize;
}

CAurFontInfo* CAurTexture::GetFontInfo()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_pFontInfo;
}

void* CAurTexture::GetImage()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_pImage;
}

int CAurTexture::GetFileRange()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nFileRange;
}

int CAurTexture::GetWidth()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nWidth;
}

int CAurTexture::GetHeight()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nHeight;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CAurTexture::GetDepth()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nDepth;
}
#endif

int CAurTexture::GetBPP()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nBPP;
}

int CAurTexture::GetNumMipMaps()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nNumMipMaps;
}

int CAurTexture::GetNumX()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nNumX;
}

int CAurTexture::GetNumY()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nNumY;
}

int CAurTexture::GetCompressedSize()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0;
    return pTexture->m_nCompressedSize;
}

float CAurTexture::GetBumpIntensity()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0.0f;
    return pTexture->m_fBumpIntensity;
}

float CAurTexture::GetBumpMapScaling()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 0.0f;
    return pTexture->m_fBumpMapScaling;
}

float CAurTexture::GetEnvMapAlpha()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 1.0f;
    return pTexture->m_fEnvMapAlpha;
}

bool CAurTexture::IsInvalid()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    return pTexture ? pTexture->m_bInvalid != 0 : CAurTextureBasic::m_pNullTexture != this;
}

Vector CAurTexture::GetSpecularColor()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        return pTexture->m_specularColor;
    Vector vColor;
    return vColor;
}

float CAurTexture::GetSpecularBumpIntensity()
{
    float fIntensity = 0.0f;
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture && pTexture->IsSpecularBumpMap())
        fIntensity = pTexture->m_fSpecularBumpIntensity;
    return fIntensity;
}

float CAurTexture::GetDiffuseBumpIntensity()
{
    float fIntensity = 0.0f;
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture && pTexture->IsDiffuseBumpMap())
        fIntensity = pTexture->m_fDiffuseBumpIntensity;
    return fIntensity;
}

// m_nBumpMapType: 1 = diffuse, 2 = normal.
bool CAurTexture::IsBumpMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    if (pTexture->m_nBumpMapType) {
        if (pTexture->IsProcessed())
            return true;
        if (pTexture->m_nBumpMapType == 1) {
            if (pTexture->m_nBPP == 1)
                return true;
        } else if (pTexture->m_nBumpMapType == 2) {
            return pTexture->m_nBPP == 4;
        }
    }
    return false;
}

bool CAurTexture::IsDiffuseBumpMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    if (!pTexture->IsBumpMap())
        return false;
    return pTexture->m_bDiffuseBumpMap != 0;
}

bool CAurTexture::IsSpecularBumpMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    if (!pTexture->IsBumpMap())
        return false;
    return pTexture->m_bSpecularBumpMap != 0;
}

bool CAurTexture::IsNormalMap()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return false;
    if (pTexture->m_nBumpMapType != 2)
        return false;
    return pTexture->m_nBPP == 4;
}

char* CAurTexture::GetName()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    return pTexture ? pTexture->m_szName : 0;
}

#if !KOTOR1
bool CAurTexture::IsNull()
{
    return GetReferencedTexture() == CAurTextureBasic::m_pNullTexture;
}
#endif

void CAurTexture::SetHeight(int nValue)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_nHeight = nValue;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CAurTexture::SetDepth(int nValue)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_nDepth = nValue;
}
#endif

void CAurTexture::SetWidth(int nValue)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_nWidth = nValue;
}

int CAurTexture::GetCompressionFormat()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture && pTexture->m_nCompressedSize != 0)
        return pTexture->m_nTextureFormat;
    return 7;
}

int CAurTexture::GetTextureFormat()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return 7;
    if (pTexture->m_nCompressedSize == 0 && pTexture->m_nTextureFormat == 7)
    {
        int nFormat;
        if (pTexture->m_nBPP == 3)
            nFormat = 1;
        else if (pTexture->m_nBPP == 4)
            nFormat = 0;
        else
            nFormat = (pTexture->m_nBPP == 2) ? 2 : 3;
        pTexture->m_nTextureFormat = static_cast<Aurora::AuroraTextureFormats>(nFormat);
        return nFormat;
    }
    return pTexture->m_nTextureFormat;
}

void CAurTexture::SetTextureFormat(Aurora::AuroraTextureFormats eFormat)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture && pTexture->m_nCompressedSize == 0)
        pTexture->m_nTextureFormat = eFormat;
}

// Setting the flag also re-queues the texture through the dirty queue.
void CAurTexture::SetIsEnvironmentMapped(bool bEnvironmentMapped)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return;
    pTexture->m_bEnvironmentMapped = bEnvironmentMapped;
    if (bEnvironmentMapped)
        RefreshTextures.AddUnique(pTexture);
}

void CAurTexture::FreeImage()
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->FreeImage();
}

void CAurTexture::IncrementTIDIndex(int nIndex)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (pTexture)
        pTexture->m_nTIDIndex = pTexture->m_panTIDIndex[nIndex];
}

void CAurTexture::glImage(bool bParam, unsigned char* pData)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (!pTexture)
        return;
    if (!pData)
    {
        pTexture->glImage(bParam);
        return;
    }
    pTexture->m_pImage = pData;
    pTexture->glImage(bParam);
    pTexture->m_pImage = 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
TextureController* CAurTexture::GetController()
{
    if (!AsCAurTextureBasic())
        return 0;
    return static_cast<CAurTextureBasic*>(AsCAurTextureBasic())->m_pController;
}
#endif

void CAurTexture::SetTxiData(unsigned char* pData, int nSize)
{
    CAurTextureBasic* pTexture = static_cast<CAurTextureBasic*>(GetReferencedTexture()->AsCAurTextureBasic());
    if (nSize >= 1)
    {
        if (pData != 0 && pTexture != 0)
        {
            if (pTexture->m_pTxiData)
            {
                delete[] pTexture->m_pTxiData;
                pTexture->m_pTxiData = 0;
            }
            pTexture->m_pTxiData = new unsigned char[nSize];
            memcpy(pTexture->m_pTxiData, pData, nSize);  // idiom-exempt: the copy length is the caller's runtime nSize
            pTexture->m_nTxiSize = nSize;
        }
    }
}

int AurTextureReplaceSingleReference(CAurTexture* pTexture, char* pszName, char* pszFilter,
                                     char* pszResRef, int* pnOffset, int nType)
{
    if (!pTexture)
        return 0;

    CAurTextureBasic* pBasic =
        static_cast<CAurTextureBasic*>(pTexture->GetReferencedTexture()->AsCAurTextureBasic());
    if (!pBasic)
        return 0;

    if (pszName != 0 && _stricmp(pszName, pBasic->m_szName) != 0)
        return 0;

    bool bEnvironmentMapped = pTexture->IsEnvironmentMapped();
    if (!pBasic->m_bModeIsDynamic)
        g_bModeIsDynamic = false;

    CAurTexture* pNew;
    int bReplaced = 0;
    if (pszResRef != 0)
    {
        char szName[32];
        __strcpy_chk(szName, pBasic->m_szName, 32);
        if (_stricmp(pszResRef, pBasic->m_szResRef) == 0)
        {
            pTexture->m_pReferenced = 0;
            AurTextureRelease(pBasic);
            bReplaced = 1;
        }
        pNew = AurTextureGet(szName, 0, pszResRef, pnOffset, nType);
    }
    else if (pszName != 0)
    {
        if (_stricmp(pszName, pBasic->m_szName) != 0)
        {
            g_bModeIsDynamic = true;
            return 0;
        }
        pTexture->m_pReferenced = 0;
        AurTextureRelease(pBasic);
        pNew = AurTextureGet(pszName, 0, pszFilter, pnOffset, nType);
        bReplaced = 1;
    }
    else
    {
        pNew = AurTextureGet(pBasic->m_szName, 0, pszFilter, pnOffset, nType);
    }

    pNew->SetIsEnvironmentMapped(bEnvironmentMapped);
    CAurTextureBasic* pNewBasic = static_cast<CAurTextureBasic*>(pNew->AsCAurTextureBasic());
    g_bModeIsDynamic = true;

    if (!bReplaced)
    {
        if (pNewBasic == static_cast<CAurTextureBasic*>(pBasic->AsCAurTextureBasic()))
            return 0;
        pTexture->m_pReferenced = pNew;
        AurTextureRelease(pBasic);
    }
    else
    {
        pTexture->m_pReferenced = pNew;
    }

    if (pnOffset != 0 && pNewBasic->m_bFullyLoaded)
        return 0;
    return 1;
}

int AurTextureReplaceTileReferences(CAurScene* pScene, const Vector& v, char* pszFrom, char* pszTo)
{
    CAurTexture* pFound = AurTextureFind(pszTo, pszTo, 0, 0);
    if (pFound == 0)
        return false;

    CAurTextureBasic* pOldBasic = static_cast<CAurTextureBasic*>(pFound->AsCAurTextureBasic());
    if (!pOldBasic->m_bModeIsDynamic)
        g_bModeIsDynamic = false;

    CAurTexture* pNew = AurTextureGet(pszFrom, 0, 0, 0, 0);
    g_bModeIsDynamic = true;

    if (pNew->IsInvalid())
    {
        AurTextureRelease(pNew);
        return false;
    }

    List<CAurTexture*> listTextures;
    List<CAurTexture*> listMatches;
    SceneBSPGetTileTextures(reinterpret_cast<Scene*>(pScene), v, listTextures);
    for (int i = 0; i < listTextures.m_count; i++)
    {
        CAurTexture* pTexture = listTextures.m_pData[i];
        if (pTexture != 0 && pTexture->m_pReferenced == pOldBasic)
        {
            listMatches.AddUnique(pTexture);
            break;
        }
    }
    if (listMatches.m_count < 1)
        return false;
    for (int i = 0; i < listMatches.m_count; i++)
    {
        listMatches.m_pData[i]->m_pReferenced = pNew;
        AurTextureRelease(pOldBasic);
    }
    return true;
}


char* firstword(char* pszLine);
char* skipspace(char* pszLine);
void Parse(char* pszLine, char* pszValue);
void Parse(char* pszLine, short& nValue);
void Parse(char* pszLine, int& nValue);
void Parse(char* pszLine, float& fValue);
void Parse(char* pszLine, bool& bValue);
void Parse(char* pszLine, Vector& vValue);
template <class T> void Parse(char* pszLine, List<T>& list);
#if KOTOR1  // K1 emits Parse<Vector> in the model parser
extern template void Parse<Vector>(char*, List<Vector>&);
#endif
unsigned char* AurResGetData(int nBytes);


bool AurATIValidate();
int  AurCubeMapAvailable();
int  AurMultiTextureAvailable();

// AurResGet reports the raw size; each caller applies the sign patch itself.
#if !KOTOR1
void AurTextureBindTID(unsigned int nTID, bool bCubeMap, bool bCubeStrip);
#endif

extern const unsigned int texParamTargetTranslationTable[];
extern const unsigned int texParamTranslationTable[];

extern unsigned char* (*AurGetLayeredRaster)(char*, int*, int*, int*);
extern void (*AurReleaseLayeredRaster)(void*, int, int);
extern void (*AurReleaseRaster)(void*, int, int, int, int, int, int, int);

extern int (*AurGetRasterHeader)(char*, int*, int*, int*);
extern int (*AurGetProcessedRasterHeader)(char*, int*, int*, char*, int*, int*, float*, char*);
int ImageRead16bitHeader(char*, int*, int*, float*, int*);
int AurTextureSprintf(char* pszBuffer, uint64_t nSize, const char* pszFormat, ...);
int AurGetImageHeaderWrapper(char*, int*, int*, int*, int*, Aurora::AuroraTextureFormats*,
                             float*, int);

void* AurGetImageWrapper(char* pszFile, int* pnWidth, int* pnHeight, int* pnBPP,
                         int* pnCompressedSize, int* pnRasterSize, int* pnTxiSize,
                         Aurora::AuroraTextureFormats* pFormat, float* pfAlphaMean,
                         bool* pbProcessed, short* pnNumMipMaps, int nFlags);
extern unsigned char* paletteimage;
extern int            paletteheight;
extern int countbasictextures;
extern int usedtexturememory;
extern int usedlightmapmemory;
extern int usedcomptexmemory;
extern int usedcubemapmemory;
extern int usedbumpmapmemory;
extern List<CAurTextureBasic*> RefreshTextures;

#if KOTOR1
SafePointer<CAurTexture> CAurTexture::m_pNullTexture;
#else
SafePointer<CAurTexture> CAurTextureBasic::m_pNullTexture;
#endif
Vector                   CAurTextureBasic::m_vLowEndEnvMapColor;
List<CAurTextureBasic*>  CAurTextureBasic::m_OrderedTextureList;
List<CAurTextureBasic*>  CAurTextureBasic::m_StaticTextureList;
List<CAurTextureBasic*>  CAurTextureBasic::m_DynamicTextureList;

float          ImageGetAlphaMean(unsigned char*, int, int, int);
void           ImageGammaCorrect(int, int, int, unsigned char*, float);
unsigned char* ImageDecode(unsigned char*, int, int, unsigned char*, int, int*, int);
void           ImageFillAlpha(unsigned char*, int, int, int, const Vector&);
int            ImageGetS3TCSize(int, int, int);

#include "aurinternal.h"

extern bool g_bModeIsDynamic;

CAurTexture::CAurTexture()
{
    m_pReferenced = 0;
    m_bUsedAsEnvMap = 0;
}

CAurTexture* AurTextureGetNULL()
{
    if (!CAurTextureBasic::m_pNullTexture)
    {
#if KOTOR1  // K1 keeps a plain pointer
        CAurTextureBasic::m_pNullTexture = new CAurTexture();
#else
        CAurTextureBasic::m_pNullTexture.m_pObject = new CAurTexture();
        CAurTextureBasic::m_pNullTexture.m_pObject->AddPointer(&CAurTextureBasic::m_pNullTexture);
#endif
    }
    return CAurTextureBasic::m_pNullTexture;
}

void AurTextureFreeAll()
{
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_OrderedTextureList.m_pData[i];
        if (pTexture->m_panTIDIndex)
        {
            ASLgl::glDeleteTextures(pTexture->m_nNumX * pTexture->m_nNumY,
                                    reinterpret_cast<const unsigned int*>(pTexture->m_panTIDIndex));
            delete[] pTexture->m_panTIDIndex;
            pTexture->m_panTIDIndex = 0;
        }
        pTexture->m_nTIDIndex = 0;
    }
#if !KOTOR1  // K1 keeps the null sentinel
    if (CAurTextureBasic::m_pNullTexture)
    {
        delete CAurTextureBasic::m_pNullTexture;
        CAurTextureBasic::m_pNullTexture = 0;
    }
#endif
}

CAurFontInfo::CAurFontInfo()
{
    __builtin_memset(static_cast<void*>(this), 0, sizeof(CAurFontInfo));
}

// Reads a vector list. A leading count selects the packed-blob form, otherwise the list is
// `endlist`-terminated, one vector per line.
template <class T>
void Parse(char* pszLine, List<T>& list)
{
    int nCount = 0;
    char cType;
    int nScan = sscanf(pszLine, "%d %c", &nCount, &cType);
    if (nScan <= 0) {
        // `endlist`-terminated form: one Vector per line until the terminator (or EOF).
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
        // In the count-only form there is one Vector per following line.
        for (int64_t i = 0; i < list.m_count; i++) {
            char* pszNext = AurResGetNextLine();
            if (!pszNext)
                break;
            Parse(pszNext, list.m_pData[i]);
        }
    } else {
        // In the packed-blob form the vertices are the resource's raw bytes.
        int nBytes = nCount * sizeof(Vector);
        memcpy(list.m_pData, AurResGetData(nBytes), nBytes);  // idiom-exempt: runtime length -- nCount * sizeof(Vector), hoisted into an int because the .so sign-extends that 32-bit value into memcpy's size_t (sxtw x2)
        pszLine[0] = 0;
    }
}

void CAurFontInfo::ParseField(char* pszLine)
{
    if (!_stricmp("numchars", firstword(pszLine)))
        Parse(pszLine + strlen("numchars"), m_nCharCount);
    if (!_stricmp("fontheight", firstword(pszLine)))
        Parse(pszLine + strlen("fontheight"), m_fBottom);
    if (!_stricmp("baselineheight", firstword(pszLine)))
        Parse(pszLine + strlen("baselineheight"), m_fTop);
    if (!_stricmp("texturewidth", firstword(pszLine)))
        Parse(pszLine + strlen("texturewidth"), m_fWidthScale);
    if (!_stricmp("spacingR", firstword(pszLine)))
        Parse(pszLine + strlen("spacingR"), m_fSpacing);
    if (!_stricmp("spacingB", firstword(pszLine)))
        Parse(pszLine + strlen("spacingB"), m_fSpacingB);
    if (!_stricmp("upperleftcoords", firstword(pszLine)))
        Parse(pszLine + strlen("upperleftcoords"), m_aMin);
    if (!_stricmp("lowerrightcoords", firstword(pszLine)))
        Parse(pszLine + strlen("lowerrightcoords"), m_aMax);
}

// Computes a rolling hash, hash = v * (hash ? hash : 1) % 65101, first over the name bytes, then
// the caller's int array.
short AurTextureCalculateHashID(char* pszName, short nCount, int* panValues)
{
    int nHash = 0;
    int nLen = static_cast<int>(strlen(pszName));
    if (nLen >= 1) {
        const char* p = pszName;
        uint64_t i = static_cast<uint64_t>(static_cast<unsigned int>(nLen));
        do {
            nHash = tolower(*p++) * (nHash ? nHash : 1) % 65101;
        } while (--i);
    }
    if (nCount >= 1) {
        uint64_t i = static_cast<uint64_t>(static_cast<unsigned int>(nCount));
        do {
            nHash = *panValues++ * (nHash ? nHash : 1) % 65101;
        } while (--i);
    }
    return static_cast<short>(nHash);
}

CAurTextureBasic::CAurTextureBasic(char* pName, char* pResRef)
{
    m_specularColor = Vector(0.0f, 0.0f, 0.0f);
    m_szResRef[0] = 0;
    if (pName == 0)
        pName = pResRef;
    __strcpy_chk(m_szName, pName, 32);
    __strcpy_chk(m_szResRef, pResRef, 32);
    m_fBumpMapScaling = 1.0f;
    m_fBumpIntensity = 1.0f;
    m_fAlphaMean = -1.0f;
    m_fGamma = 1.0f;
    m_fEnvMapAlpha = 1.0f;
    m_fDiffuseBumpIntensity = 1.0f;
    m_fSpecularBumpIntensity = 1.0f;
    m_nMaxDownSampleLevel = 15;
    m_nDefaultDownSampleLevel = 0;
    m_nDownSampleLevel = 0;
    m_nDownSampleLevelBuilt = 0;
    m_nWidth = 2;
    m_nHeight = 2;
    m_nDefaultWidth = 2;
    m_nDefaultHeight = 2;
#if !KOTOR1
    m_nDepth = 0;
#endif
    m_nBPP = 0;
    m_nClamp = 0;
    m_nNumX = 1;
    m_nNumY = 1;
    m_nNumSchemes = 0;
    m_nFileRange = 0;
    m_nNumMipMaps = 0;
    m_nTIDIndex = 0;
    m_nCompressedSize = 0;
    m_nRasterSize = 0;
    m_nTxiSize = 0;
    m_nTextureFormat = static_cast<Aurora::AuroraTextureFormats>(7);
    m_bModeIsDynamic = g_bModeIsDynamic;
    m_bTemporary = 0;
    m_bCubeMap = 0;
    m_bDiffuseBumpMap = 1;
    m_bSpecularBumpMap = 1;
    m_bEnvironmentMapped = 0;
    m_bMipMap = 1;
    m_bFilter = 1;
    m_nBumpMapType = 0;
    m_bProcessed = 0;
    m_bTxiLoaded = 0;
    m_bNormalizationCubeMap = 0;
    m_bReleaseRaster = 0;
    m_bUsingGlobalAlpha = 0;
    m_bMapTexelsToPixels = 0;
    m_bFullyLoaded = 0;
    m_bInvalid = 0;
    m_bBound = 1;
    m_specularColor = Vector(1.0f, 1.0f, 1.0f);
    m_pFontInfo = new CAurFontInfo();
    m_pController = 0;
    m_nHashID = 0;
    m_panScheme = 0;
    m_panTIDIndex = 0;
    m_pImage = 0;
    m_pTxiData = 0;
    m_nHashID = AurTextureCalculateHashID(m_szResRef, 0, 0);
    AddToOrderedLists();
    countbasictextures++;
    m_nMemoryUsage = -1;
    m_bMemoryDirty = 1;
#if !KOTOR1
    m_bTexture3D = 0;
#endif
}

int CAurTextureBasic::glImage(bool bParam)
{
    AurATIValidate();
    if (!m_pImage) {
        m_nTIDIndex = 0;
        return 0;
    }
    if (m_nTIDIndex == 0 || !m_panTIDIndex) {
        if (m_panTIDIndex)
            delete[] m_panTIDIndex;
        short nNumX = m_nNumX;
        short nNumY = m_nNumY;
#if KOTOR1  // K1 brackets the generation with glGetError calls
        int* panTID = new int[static_cast<int64_t>(nNumX) * nNumY];
        m_panTIDIndex = panTID;
        int nCount = nNumX * nNumY;
        ASLgl::glGetError();
        ASLgl::glGenTextures(nCount, reinterpret_cast<unsigned int*>(panTID));
        ASLgl::glGetError();
#else
        m_panTIDIndex = new int[static_cast<int64_t>(nNumX) * nNumY];
        ASLgl::glGenTextures(nNumX * nNumY, reinterpret_cast<unsigned int*>(m_panTIDIndex));
#endif
        m_nTIDIndex = m_panTIDIndex[0];
    }
#if KOTOR1  // K1 has no cube-strip flag
    for (int i = 0; i < m_nNumX * m_nNumY; i++) {
        AurTextureBindTID(static_cast<unsigned int>(m_panTIDIndex[i]), m_bCubeMap);
#else
    unsigned char bCubeStrip = m_bTexture3D;
    for (int i = 0; i < m_nNumX * m_nNumY; i++) {
        AurTextureBindTID(static_cast<unsigned int>(m_panTIDIndex[i]), m_bCubeMap, bCubeStrip);
#endif
        if (m_bCubeMap) {
            if (IsProcessed())
                GLRender::CreateProcessedCubeTexture(this, bParam, i);
            else
                GLRender::CreateCubeTexture(this, bParam, i);
        } else if (IsProcessed()) {
            GLRender::CreateProcessedTexture(this, bParam, i);
        } else {
            if (m_pController)
                m_pController->IsLinear();
            GLRender::CreateTexture(this, bParam, i);
        }
    }
    if (m_bCubeMap && AurCubeMapAvailable())
        GLRender::Disable(Aurora::kCapTextureCubeMap);
#if !KOTOR1
    if (bCubeStrip)
        GLRender::Disable(Aurora::kCapTexture3D);
#endif
    return m_nTIDIndex;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
TextureController* CAurTextureBasic::GetController()
{
    return m_pController;
}
#endif

CAurTextureBasic* CAurTextureBasic::AsCAurTextureBasic() { return this; }

void CAurTextureBasic::DownSample() {}

void CAurTextureBasic::CheckAttributes()
{
    if (!m_pImage) {
        if (m_nBumpMapType)
            m_nBPP = 1;
        m_nWidth = (m_nWidth > 2) ? m_nWidth : 2;
        m_nHeight = (m_nHeight > 2) ? m_nHeight : 2;
    }
}

void CAurTextureBasic::SetScheme(int* panScheme, int nCount)
{
    if (!m_panScheme)
        m_panScheme = new int[256];
    for (int i = 0; i < 256; i++)
        m_panScheme[i] = (i < nCount) ? panScheme[i] : 0;
    m_nNumSchemes = static_cast<short>(nCount);
}

void CAurTextureBasic::Reset()
{
    FreeImage();
    short nLevel = m_nDefaultDownSampleLevel;
    m_nCompressedSize = 0;
    m_nRasterSize = 0;
    m_nTxiSize = 0;
    m_nNumMipMaps = 0;
    m_nDownSampleLevelBuilt = 0;
    m_bFullyLoaded = 0;
    m_bMemoryDirty = 1;
    m_nDownSampleLevel = nLevel;
    m_bProcessed = 0;
    m_bTxiLoaded = 0;
    if (m_pTxiData) {
        delete[] m_pTxiData;
        m_pTxiData = 0;
    }
    if (m_panTIDIndex) {
        ASLgl::glDeleteTextures(m_nNumX * m_nNumY, reinterpret_cast<const unsigned int*>(m_panTIDIndex));
        delete m_panTIDIndex;
        m_panTIDIndex = 0;
        m_nTIDIndex = 0;
    }
}

void CAurTextureBasic::CalculateHashID()
{
    m_nHashID = AurTextureCalculateHashID(m_szResRef, m_nNumSchemes, m_panScheme);
}

// Accounts for the compressed payload handed back to the releaser: the base level plus one
// block count per extra mip level.
void CAurTextureBasic::FreeImage()
{
    if (!m_pImage)
        return;
    int nBits = (m_nBPP == 4) ? 16 : 8;
    int nWidth = m_nWidth;
    int nHeight = m_nHeight;
    int nRatio = nHeight / nWidth;
    int nFaceHeight = nHeight;
    if (nRatio == 6)
        nFaceHeight = nHeight / 6;
    int nSize = 0;
    if (m_nCompressedSize) {
        int nW = nWidth;
        nSize = m_nCompressedSize;
        for (int i = 0; i < m_nNumMipMaps - 1; i++) {
            nW >>= 1;
            nFaceHeight >>= 1;
            int nWq = (nW + 3) / 4;
            int nHq = (nFaceHeight + 3) / 4;
            nSize += nWq * nBits * nHq;
        }
        nSize = nSize * ((nRatio == 6) ? 6 : 1);
    }
    m_bTxiLoaded = 0;
    if (m_bReleaseRaster) {
        AurReleaseRaster(m_pImage, m_nWidth, m_nHeight, m_nBPP, nSize, m_nRasterSize,
                         m_nTxiSize, m_bProcessed);
    } else {
        delete[] static_cast<unsigned char*>(m_pImage);
    }
    m_bReleaseRaster = 0;
    m_pImage = 0;
}

// Multi-file textures stack their slices into one column: 1 x (range * slice height).
void CAurTextureBasic::LoadImageHeader()
{
    if (m_szResRef[0] == 0)
        __strcpy_chk(m_szResRef, m_szName, 32);

    m_nWidth = 0;
    m_nHeight = 0;

    int bLoaded = 0;
    if (m_nFileRange == 0)
    {
        int nFlags = 0;
        if (!m_bCubeMap)
            nFlags = (m_nNumX * m_nNumY == 1);
        bLoaded = AurGetImageHeaderWrapper(m_szResRef, &m_nWidth, &m_nHeight, &m_nBPP,
                                           &m_nCompressedSize, &m_nTextureFormat, &m_fAlphaMean,
                                           nFlags);
    }
    else if (m_nFileRange <= 0)
    {
        bLoaded = 0;
    }
    else
    {
            // A multi-file texture is invalid when its last slice fails every reader.
        for (int i = 0; i < m_nFileRange; i++)
        {
            char szName[128];
            int nWidth, nHeight, nMipChainSize, nBPP, nFlags;
            char cBPP, cMipCount;

            AurTextureSprintf(szName, sizeof(szName), "%s%d", m_szResRef, i);
            nFlags = 0;
            bLoaded = AurGetProcessedRasterHeader(szName, &nWidth, &nHeight, &cBPP,
                                                  &nFlags, &nMipChainSize, &m_fAlphaMean, &cMipCount);
            nBPP = cBPP;
            if (bLoaded == 0)
            {
                nBPP = 2;
                bLoaded = ImageRead16bitHeader(szName, &nWidth, &nHeight, &m_fAlphaMean,
                                               reinterpret_cast<int*>(&m_nTextureFormat));
                if (bLoaded == 0)
                    bLoaded = AurGetRasterHeader(szName, &nWidth, &nHeight, &nBPP);
            }
            if (bLoaded == 0)
            {
                AurTextureSprintf(szName, sizeof(szName), "%s0%d", m_szResRef, i);
                nFlags = 0;
                bLoaded = AurGetProcessedRasterHeader(szName, &nWidth, &nHeight, &cBPP,
                                                      &nFlags, &nMipChainSize, &m_fAlphaMean, &cMipCount);
                nBPP = cBPP;
                if (bLoaded == 0)
                {
                    nBPP = 2;
                    bLoaded = ImageRead16bitHeader(szName, &nWidth, &nHeight, &m_fAlphaMean,
                                                   reinterpret_cast<int*>(&m_nTextureFormat));
                    if (bLoaded == 0)
                        bLoaded = AurGetRasterHeader(szName, &nWidth, &nHeight, &nBPP);
                }
            }
            if (i == 0)
            {
                m_nWidth = nWidth;
                m_nBPP = nBPP;
            }
            m_nHeight += nHeight;
        }
    }
    if (bLoaded == 0)
        m_bInvalid = 1;

    m_nDownSampleLevelBuilt = m_nDownSampleLevel;
    int nWidth = m_nWidth >> m_nDownSampleLevel;
    int nHeight = m_nHeight >> m_nDownSampleLevel;
    m_nWidth = (nWidth > 2) ? nWidth : 2;
    m_nHeight = (nHeight > 2) ? nHeight : 2;
}

int AurTextureSprintf(char* pszBuffer, uint64_t nSize, const char* pszFormat, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFormat);
    int n = __vsprintf_chk(pszBuffer, 0, nSize, pszFormat, ap);
    __builtin_va_end(ap);
    return n;
}

// Loads multiple faces, one raster per file range, concatenated into a single buffer.
#if KOTOR1
void CAurTextureBasic::LoadImage()
#else
void CAurTextureBasic::LoadImageA()
#endif
{
    FreeImage();

    if (m_szResRef[0] == 0)
        __strcpy_chk(m_szResRef, m_szName, 32);

    if (m_nFileRange == 0) {
        int nFlags = 0;
        if (!m_bCubeMap && m_nNumX * m_nNumY == 1)
            nFlags = (m_pController == 0);
        bool bProcessed = m_bProcessed;
        m_pImage = AurGetImageWrapper(m_szResRef, &m_nWidth, &m_nHeight, &m_nBPP,
                                      &m_nCompressedSize, &m_nRasterSize, &m_nTxiSize,
                                      &m_nTextureFormat, &m_fAlphaMean, &bProcessed,
                                      &m_nNumMipMaps, nFlags);
        m_bProcessed = bProcessed;
        m_bReleaseRaster = (AurReleaseRaster != 0) && (m_pImage != 0);
        return;
    }

    void** apFiles = new void*[m_nFileRange];
    m_nHeight = 0;
    for (int64_t i = 0; i < m_nFileRange; i++) {
        int nWidth, nHeight, nBPP, nCompressedSize;
        char szFile[128];
        apFiles[i] = 0;
        AurTextureSprintf(szFile, sizeof(szFile), "%s%d", m_szResRef, static_cast<int>(i));
        bool bProcessed = m_bProcessed;
        apFiles[i] = AurGetImageWrapper(szFile, &nWidth, &nHeight, &nBPP, &nCompressedSize,
                                        &m_nRasterSize, &m_nTxiSize, &m_nTextureFormat,
                                        &m_fAlphaMean, &bProcessed, &m_nNumMipMaps, 0);
        m_bProcessed = bProcessed;
        if (!apFiles[i]) {
            AurTextureSprintf(szFile, sizeof(szFile), "%s0%d", m_szResRef, static_cast<int>(i));
            apFiles[i] = AurGetImageWrapper(szFile, &nWidth, &nHeight, &nBPP, &nCompressedSize,
                                            &m_nRasterSize, &m_nTxiSize, &m_nTextureFormat,
                                            &m_fAlphaMean, &bProcessed, &m_nNumMipMaps, 0);
            m_bProcessed = bProcessed;
        }
        m_nWidth  = nWidth;
        m_nHeight = nHeight;
        m_nBPP    = nBPP;
    }

    m_pImage = new unsigned char[m_nWidth * m_nFileRange * m_nHeight * m_nBPP];

    unsigned char* pDest = static_cast<unsigned char*>(m_pImage);
    for (int64_t i = 0; i < m_nFileRange; i++) {
        memcpy(pDest, apFiles[i], m_nWidth * m_nHeight * m_nBPP);  // idiom-exempt: runtime face size
        if (AurReleaseRaster)
            AurReleaseRaster(apFiles[i], m_nWidth, m_nHeight, m_nBPP, 0, m_nRasterSize,
                             m_nTxiSize, m_bProcessed);
        else if (apFiles[i])
            delete[] static_cast<unsigned char*>(apFiles[i]);
        pDest += m_nWidth * m_nHeight * m_nBPP;
    }

    if (!IsCubeMap())
        m_nNumY = m_nFileRange;
    m_bReleaseRaster = 0;
    delete[] apFiles;
    m_nHeight *= m_nFileRange;
}

void CAurTextureBasic::LoadImagePLT()
{
    FreeImage();
    if (m_szResRef[0] == 0)
        __strcpy_chk(m_szResRef, m_szName, 32);
    int nFileRange;
    m_pImage = AurGetLayeredRaster(m_szResRef, &m_nWidth, &m_nHeight, &nFileRange);
    m_nBPP = 2;
    m_nTextureFormat = static_cast<Aurora::AuroraTextureFormats>(0);
    m_bReleaseRaster = (AurReleaseRaster != 0) && (m_pImage != 0);
}

void CAurTextureBasic::BuildAndStoreTexture()
{
    if (m_bNormalizationCubeMap) {
        if (strcmp(m_szName, "NormCubeMap") != 0)
            return;
        AsNormalizeVectorCubeMap();
        return;
    }
    if (!m_bFullyLoaded)
        ConstructImage();
    if (m_pImage) {
        glImage(false);
        if (m_fAlphaMean == -1.0f) {
            if (m_nCompressedSize >= 1)
                m_fAlphaMean = 1.0f;
            else
                m_fAlphaMean = ImageGetAlphaMean(static_cast<unsigned char*>(m_pImage), m_nWidth, m_nHeight,
                                                 m_nBPP);
        }
    }
    if (m_nCompressedSize == 0 && m_pController)
        m_pController->Init();
    if (!IsProcessed())
        FreeImage();
}

// Bakes the shared 256 x 1536 normalization cube map and leaves it bound.
void CAurTextureBasic::AsNormalizeVectorCubeMap()
{
    m_nBPP = 3;
    m_nDownSampleLevel = 0;
    m_nMaxDownSampleLevel = 0;
    m_nDefaultDownSampleLevel = 0;
    m_bNormalizationCubeMap = 1;
    m_bFilter = 1;
    m_bCubeMap = 1;
    m_bMipMap = 1;
    m_nWidth = 256;
    m_nHeight = 1536;
    if (!m_panTIDIndex)
        m_panTIDIndex = new int[1];
#if KOTOR1  // K1 brackets the bake with glGetError and sets no filter or wrap parameters
    unsigned int* pTID = reinterpret_cast<unsigned int*>(&m_nTIDIndex);
    ASLgl::glGetError();
    ASLgl::glGenTextures(1, pTID);
    ASLgl::glGetError();
#else
    ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&m_nTIDIndex));
#endif
    m_panTIDIndex[0] = m_nTIDIndex;
    GLRender::Enable(Aurora::kCapNormalize);
    GLRender::SetActiveTexture(1);
    GLRender::BindTexture(Aurora::kCapTextureCubeMap, m_nTIDIndex);
    GLRender::MakeNormalizationCubeMap(256);
#if !KOTOR1
    GLRender::SetTextureParameteri(Aurora::kTexTarget2D, Aurora::kTexParamMagFilter, 5);
    GLRender::SetTextureParameteri(Aurora::kTexTarget2D, Aurora::kTexParamMinFilter, 6);
#endif
    GLRender::Disable(Aurora::kCapTextureCubeMap);
    GLRender::SetActiveTexture(0);
}

// Gamma-corrects the current image, or rebuilds from the layered raster when a palette scheme
// exists, and falls back to a procedural raster when there is no image at all.
void CAurTextureBasic::ConstructImage()
{
    ImageGammaCorrect(m_nWidth, m_nHeight, m_nBPP, static_cast<unsigned char*>(m_pImage), m_fGamma);
    if (m_panScheme) {
        FreeImage();
        if (m_szResRef[0] == 0)
            __strcpy_chk(m_szResRef, m_szName, 32);
        int nFileRange;
        unsigned char* pRaster =
            AurGetLayeredRaster(m_szResRef, &m_nWidth, &m_nHeight, &nFileRange);
        m_pImage = pRaster;
        m_nBPP = 2;
        m_nTextureFormat = static_cast<Aurora::AuroraTextureFormats>(0);
        m_nCompressedSize = 0;
        m_bReleaseRaster = (pRaster != 0) && (AurReleaseRaster != 0);
        if (pRaster) {
            m_pImage = ImageDecode(static_cast<unsigned char*>(pRaster), m_nWidth, m_nHeight, paletteimage,
                                   paletteheight, m_panScheme, m_nNumSchemes);
            if (m_bReleaseRaster)
                AurReleaseLayeredRaster(pRaster, m_nWidth, m_nHeight);
            else
                delete[] pRaster;
            m_bReleaseRaster = 0;
            m_nBPP = 4;
        }
        m_fAlphaMean = 1.0f;
    } else {
        LoadImageA();
    }
    CheckAttributes();
    if (!m_pImage) {
        if (m_pController) {
            m_nBPP = 4;
            m_nWidth = m_nDefaultWidth;
            m_nHeight = m_nDefaultHeight;
#if KOTOR1  // K1 has no depth factor
            int nPixels = m_nHeight * m_nWidth;
#else
            int nPixels = m_nHeight * m_nWidth * (m_nDepth ? m_nDepth : 1);
#endif
            m_pImage = new unsigned char[nPixels * static_cast<int>(sizeof(unsigned int))];
            memset(m_pImage, 127, nPixels * static_cast<int>(sizeof(unsigned int)));
        } else {
            m_bInvalid = 1;
        }
    }
    if (m_bEnvironmentMapped && m_bLowEndEnvMapEnabled)
        ImageFillAlpha(static_cast<unsigned char*>(m_pImage), m_nWidth, m_nHeight, m_nBPP,
                       m_vLowEndEnvMapColor);
    m_bFullyLoaded = 1;
}

// Loads the .txi field table, from the object's own in-memory copy when it has one, otherwise
// from the resource.
void CAurTextureBasic::Init(char* pResRef)
{
    int nTxiSize = 0;

    if (IsTxiLoaded()) {
        unsigned char* pTxi = GetTxiData();
        int nSize = GetTxiSize();
        if (pTxi != 0 && nSize >= 1) {
            char szLine[8192];
            int nRemaining = nSize;
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
        if (m_pFontInfo == 0)
            m_pFontInfo = new CAurFontInfo();

        void* pRes = AurResGet(pResRef, const_cast<char*>(".txi"), &nTxiSize, true);
        if (pRes == 0) {
            delete m_pFontInfo;
            m_pFontInfo = 0;
            SetTxiLoaded(true);
            return;
        } else {
            int nCount = (nTxiSize < 0) ? ~nTxiSize : nTxiSize;
            if (nCount > 0) {
                unsigned char* pBytes = static_cast<unsigned char*>(AurResGetDataBytes(0, pRes));
                SetTxiData(pBytes, nCount);
                char* pszLine;
                while ((pszLine = AurResGetNextLine()) != 0) {
                    while (*pszLine == ' ' || *pszLine == '\t')
                        pszLine++;
                    ParseField(pszLine);
                }
            }
            AurResFree(pRes, nTxiSize);
            if (m_pFontInfo != 0 && m_pFontInfo->m_aMin.m_count == 0) {
                delete m_pFontInfo;
                m_pFontInfo = 0;
            }
            SetTxiLoaded(true);
        }
    }

    if (!AurCubeMapAvailable())
        m_bCubeMap = false;
}

// Parses one .txi line: `proceduretype` selects an animation controller, then one key per
// member, then the font and controller get their own shot at the line.
void CAurTextureBasic::ParseField(char* pszLine)
{
    char szValue[64];
    szValue[0] = 0;

    if (!_stricmp("proceduretype", firstword(pszLine)))
        Parse(pszLine + strlen("proceduretype"), szValue);

    if (strlen(szValue)) {
        if (m_pController) {
            delete m_pController;
            m_pController = 0;
        }
#if !KOTOR1  // K1 has no dirty texture controllers
        if (!memcmp(szValue, "dirty", sizeof("dirty"))) {
            m_pController = new DirtyTextureController(this, 0);
            m_bTexture3D = 1;
        }
        if (!memcmp(szValue, "dirty2", sizeof("dirty2"))) {
            m_pController = new DirtyTextureController(this, 1);
            m_bTexture3D = 1;
        }
        if (!memcmp(szValue, "dirty3", sizeof("dirty3"))) {
            m_pController = new DirtyTextureController(this, 2);
            m_bTexture3D = 1;
        }
#endif
        if (!memcmp(szValue, "water", sizeof("water"))) {
            m_pController = new WaterTextureController(this);
        }
        if (!memcmp(szValue, "life", sizeof("life"))) {
            m_pController = new LifeTextureController(this);
        }
        if (!memcmp(szValue, "perlin", sizeof("perlin"))) {
            m_pController = new PerlinTextureController(this);
        }
        if (!memcmp(szValue, "arturo", sizeof("arturo"))) {
            m_pController = new ArturoTextureController(this);
        }
        if (!memcmp(szValue, "wave", sizeof("wave"))) {
            m_pController = new WaveTextureController(this);
        }
        if (!memcmp(szValue, "cycle", sizeof("cycle"))) {
            m_pController = new CycleTIDTextureController(this);
        }
        if (!memcmp(szValue, "random", sizeof("random"))) {
            m_pController = new RandomTextureController(this);
        }
        if (!memcmp(szValue, "ringtexdistort", sizeof("ringtexdistort"))) {
            m_pController = new RingDistortTextureController(this);
        }
    }
    if (!_stricmp("filerange", firstword(pszLine)))
        Parse(pszLine + strlen("filerange"), m_nFileRange);
    if (!_stricmp("defaultwidth", firstword(pszLine)))
        Parse(pszLine + strlen("defaultwidth"), m_nDefaultWidth);
    if (!_stricmp("defaultheight", firstword(pszLine)))
        Parse(pszLine + strlen("defaultheight"), m_nDefaultHeight);
    if (!_stricmp("downsamplemax", firstword(pszLine)))
        Parse(pszLine + strlen("downsamplemax"), m_nMaxDownSampleLevel);
    if (!_stricmp("downsamplemin", firstword(pszLine)))
        Parse(pszLine + strlen("downsamplemin"), m_nDefaultDownSampleLevel);
    if (!_stricmp("mipmap", firstword(pszLine)))
        Parse(pszLine + strlen("mipmap"), m_bMipMap);
    if (!_stricmp("filter", firstword(pszLine)))
        Parse(pszLine + strlen("filter"), m_bFilter);
    if (!_stricmp("maptexelstopixels", firstword(pszLine)))
        Parse(pszLine + strlen("maptexelstopixels"), m_bMapTexelsToPixels);
    if (!_stricmp("gamma", firstword(pszLine)))
        Parse(pszLine + strlen("gamma"), m_fGamma);
    if (!_stricmp("isbumpmap", firstword(pszLine)))
        Parse(pszLine + strlen("isbumpmap"), m_nBumpMapType);
    if (!_stricmp("clamp", firstword(pszLine)))
        Parse(pszLine + strlen("clamp"), m_nClamp);
    if (!_stricmp("alphamean", firstword(pszLine)))
        Parse(pszLine + strlen("alphamean"), m_fAlphaMean);
    if (!_stricmp("isdiffusebumpmap", firstword(pszLine)))
        Parse(pszLine + strlen("isdiffusebumpmap"), m_bDiffuseBumpMap);
    if (!_stricmp("isspecularbumpmap", firstword(pszLine)))
        Parse(pszLine + strlen("isspecularbumpmap"), m_bSpecularBumpMap);
    if (!_stricmp("bumpmapscaling", firstword(pszLine)))
        Parse(pszLine + strlen("bumpmapscaling"), m_fBumpMapScaling);
    if (!_stricmp("specularcolor", firstword(pszLine)))
        Parse(pszLine + strlen("specularcolor"), m_specularColor);
    if (!_stricmp("numx", firstword(pszLine)))
        Parse(pszLine + strlen("numx"), m_nNumX);
    if (!_stricmp("numy", firstword(pszLine)))
        Parse(pszLine + strlen("numy"), m_nNumY);
    if (!_stricmp("cube", firstword(pszLine)))
        Parse(pszLine + strlen("cube"), m_bCubeMap);
    if (!_stricmp("bumpintensity", firstword(pszLine)))
        Parse(pszLine + strlen("bumpintensity"), m_fBumpIntensity);
    if (!_stricmp("temporary", firstword(pszLine)))
        Parse(pszLine + strlen("temporary"), m_bTemporary);
    if (!_stricmp("useglobalalpha", firstword(pszLine)))
        Parse(pszLine + strlen("useglobalalpha"), m_bUsingGlobalAlpha);
    if (!_stricmp("isenvironmentmapped", firstword(pszLine)))
        Parse(pszLine + strlen("isenvironmentmapped"), m_bEnvironmentMapped);
    if (!_stricmp("envmapalpha", firstword(pszLine)))
        Parse(pszLine + strlen("envmapalpha"), m_fEnvMapAlpha);
    if (!_stricmp("diffusebumpintensity", firstword(pszLine)))
        Parse(pszLine + strlen("diffusebumpintensity"), m_fDiffuseBumpIntensity);
    if (!_stricmp("specularbumpintensity", firstword(pszLine)))
        Parse(pszLine + strlen("specularbumpintensity"), m_fSpecularBumpIntensity);

    m_pFontInfo->ParseField(pszLine);
    if (m_pController)
        m_pController->ParseField(pszLine);
    if (m_nDefaultDownSampleLevel > 0)
        m_nDownSampleLevel = m_nDefaultDownSampleLevel;
}

// Keeps the ordered list sorted by hash id, skipping past any equal-hash run, then adds the
// texture to the dynamic or static list.
void CAurTextureBasic::AddToOrderedLists()
{
    int nCount = m_OrderedTextureList.m_count;
    int nIndex = 0;
    while (nIndex < nCount && m_OrderedTextureList.m_pData[nIndex]->m_nHashID < m_nHashID)
        nIndex++;
    while (nIndex < nCount && m_OrderedTextureList.m_pData[nIndex]->m_nHashID == m_nHashID)
        nIndex++;
    m_OrderedTextureList.Insert(this, nIndex);

    if (m_bModeIsDynamic)
        m_DynamicTextureList.Add(this);
    else
        m_StaticTextureList.Add(this);
}

CAurTextureBasic::~CAurTextureBasic()
{
    delete m_pFontInfo;
    delete[] m_panScheme;
    delete m_pController;

    TEXTUREMEMORYSTATS stats;
    if (!m_bTemporary)
    {
        GetMemoryUsage(&stats);
        usedtexturememory -= stats.m_anMemory[TMS_TOTAL];
        usedlightmapmemory -= stats.m_anMemory[TMS_LIGHTMAP];
        usedcomptexmemory -= stats.m_anMemory[TMS_COMPRESSED];
        usedcubemapmemory -= stats.m_anMemory[TMS_CUBEMAP];
        usedbumpmapmemory -= stats.m_anMemory[TMS_BUMPMAP];
    }

    if (m_panTIDIndex)
    {
        ASLgl::glDeleteTextures(m_nNumX * m_nNumY, reinterpret_cast<unsigned int*>(m_panTIDIndex));
        delete[] m_panTIDIndex;
        m_panTIDIndex = 0;
        m_nTIDIndex = 0;
    }
    if (m_pTxiData)
    {
        delete[] m_pTxiData;
        m_pTxiData = 0;
    }

    FreeImage();
    countbasictextures--;
    RemoveFromOrderedLists();
    if (RefreshTextures.Contains(this))
        RefreshTextures.Remove(this);
}

void CAurTextureBasic::RemoveFromOrderedLists()
{
    if (m_OrderedTextureList.Contains(this))
        m_OrderedTextureList.Remove(this);

    if (m_bModeIsDynamic)
        m_DynamicTextureList.Remove(this);
    else
        m_StaticTextureList.Remove(this);
}

CAurTexture* AurTextureGetTextureWrapper(unsigned int nTID, bool bMipMap, bool bClamp,
                                         int nWidth, int nHeight, int nBPP)
{
    CAurTexture* pTexture = new CAurTexture();

    char szName[64];
    AurTextureSprintf(szName, sizeof(szName), "%dtexture", nTID);

    CAurTextureBasic* pBasic = new CAurTextureBasic(szName, szName);
    pBasic->m_panTIDIndex = new int[1];
    pBasic->m_panTIDIndex[0] = nTID;
    pBasic->m_nTIDIndex = nTID;
    pBasic->m_nWidth = nWidth;
    pBasic->m_nHeight = nHeight;
    pBasic->m_nBPP = nBPP;
    pBasic->m_bMipMap = bMipMap;
    pBasic->m_nClamp = bClamp;
    pBasic->m_nMaxDownSampleLevel = 0;
    pBasic->m_nDefaultDownSampleLevel = 0;
    pBasic->m_bInvalid = 0;
    pTexture->m_pReferenced = pBasic;
    return pTexture;
}

CAurTexture* AurTextureFind(char* pszName, char* pszFilter, int* pnOffset, int nType);

#ifdef K2_QOL_CONTROLLER
#include <strings.h>
#endif

// Cache lookup that wraps a supplied texture, else looks up the filter/name pair, else creates the
// texture and queues it.
CAurTexture* AurTextureGet(char* pszName, CAurTexture* pTexture, char* pszFilter, int* pnOffset,
                           int nType)
{
    char szFilter[32];
    szFilter[0] = 0;

    if (pTexture != 0)
    {
        CAurTexture* pReference = new CAurTexture();
        pReference->m_pReferenced = pTexture;
        return pReference;
    }

#ifdef K2_QOL_CONTROLLER
    // The button-prompt textures follow the Controller options page's choice.
    if (const char* pszRenamed = pszName ? k2qol_texture_name(pszName) : 0)
    {
        if (pszFilter && *pszFilter && !strcasecmp(pszFilter, pszName))
            pszFilter = const_cast<char*>(pszRenamed);
        pszName = const_cast<char*>(pszRenamed);
    }
#endif
#if KOTOR1 && defined(K1_UI_XBOX)
    // The PC art's "abutton" is the Spacebar keycap ("Sp"; the swoop HUD's last gear segment,
    // m17mg_camera): the pad's A glyph instead.
    if (pszName && !strcasecmp(pszName, "abutton"))
    {
        if (pszFilter && *pszFilter && !strcasecmp(pszFilter, pszName))
            pszFilter = const_cast<char*>("ios_button_a");
        pszName = const_cast<char*>("ios_button_a");
    }
#endif

    if (pszFilter == 0 || *pszFilter == 0)
        __strcpy_chk(szFilter, pszName, 32);
    else
        __strcpy_chk(szFilter, pszFilter, 32);

    CAurTexture* pFound = AurTextureFind(pszName, szFilter, pnOffset, nType);
    if (pFound != 0)
        return pFound;

    CAurTextureBasic* pNew = new CAurTextureBasic(pszName, szFilter);
    pNew->Init(pNew->m_szResRef);
    RefreshTextures.Add(pNew);
    return pNew;
}

// Looks a texture up in the ordered list, linearly by name, or by hash id, scheme count and resref
// when a filter is given.
CAurTexture* AurTextureFind(char* pszName, char* pszFilter, int* pnOffset, int nType)
{
    if (pszFilter == 0)
    {
        for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
        {
            if (!_stricmp(
                    pszName, CAurTextureBasic::m_OrderedTextureList.m_pData[i]->m_szName))
                return CAurTextureBasic::m_OrderedTextureList.m_pData[i];
        }
        return 0;
    }

    short nHash = AurTextureCalculateHashID(pszFilter, static_cast<short>(nType), pnOffset);

    int i = 0;
    while (i < CAurTextureBasic::m_OrderedTextureList.m_count &&
           CAurTextureBasic::m_OrderedTextureList.m_pData[i]->m_nHashID < static_cast<unsigned short>(nHash))
        i++;

    for (; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_OrderedTextureList.m_pData[i];
        if (pTexture->m_nHashID != static_cast<unsigned short>(nHash))
            break;
        if (pTexture->m_nNumSchemes != nType)
            continue;
        if (_stricmp(pszFilter, pTexture->m_szResRef))
            continue;
        int j = 0;
        while (j < nType && pnOffset[j] == pTexture->m_panScheme[j])
            j++;
        if (j < nType)
            continue;
        return pTexture;
    }
    return 0;
}

extern List<CAurTextureBasic*> RefreshTextures;
template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item);

void AurTextureSetLayered(CAurTexture* pTexture, char* pszResRef, int* panScheme, int nCount)
{
    if (pTexture == 0)
        return;

    CAurTextureBasic* pBasic = pTexture->GetReferencedTexture()->AsCAurTextureBasic();

    if (pszResRef != 0 && *pszResRef != 0)
        __strcpy_chk(pBasic->m_szResRef, pszResRef, 32);

    pBasic->SetScheme(panScheme, nCount);
    pBasic->m_nBPP = 4;
    pBasic->CalculateHashID();
    pBasic->RemoveFromOrderedLists();
    pBasic->AddToOrderedLists();
    RefreshTextures.AddUnique(pBasic);
}

extern int   maxtexmem;
extern unsigned int maxtexid;
extern int   initialized;
extern bool  g_bModeIsDynamic;
extern int   constructimagetime;
extern int   decodeimagetime;
extern int   glimagetime;
extern int   loadimagetime;
extern int   maxtextime;
extern int   currenttextime;
extern int   maxtexloadtime;
extern int   maxdecodetime;
extern int   maxgltime;
extern int   texcurrentframe;
extern float DeltaT;
extern float maxdeltat;
extern bool  g_bIsSuspended;

extern float g_fDynamicMemoryRatio;
extern float memcheckratio;

extern int useddynamictexturememory;
extern int usedstatictexturememory;
extern int usedlightmapmemory;
extern int usedcomptexmemory;
extern int usedcubemapmemory;
extern int usedbumpmapmemory;
extern int usedtexturememory;

static TEXTUREMEMORYSTATS g_dynamicTMS;
static TEXTUREMEMORYSTATS g_staticTMS;
extern List<CAurTextureBasic*> RefreshTextures;

void AurInitFont();
void AurFreeFont();
void AurTextureFreeAll();
void AurTextureBuildAndStoreAll();
int AurCubeMapAvailable();
CAurTexture* AurTextureFind(char* pszName, char* pszFilter, int* pnOffset, int nType);

template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item);

// Applies a texture-memory budget: Reset every texture, split the budget between the dynamic
// and static lists, and fold the per-category counters into the engine-wide totals.
int AurTextureSetMem(int nMem)
{
    maxtexmem = nMem;
    float fDynamicRatio = g_fDynamicMemoryRatio;
    int nDynamic = static_cast<int>(fDynamicRatio * nMem);

#if KOTOR1  // K1 copies the list in one assignment
    RefreshTextures = CAurTextureBasic::m_OrderedTextureList;
#else
    RefreshTextures.m_count = 0;
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
        RefreshTextures.Add(CAurTextureBasic::m_OrderedTextureList.m_pData[i]);
#endif

    for (int i = 0; i < RefreshTextures.m_count; i++)
        RefreshTextures.m_pData[i]->Reset();

    AurTextureMemCheck(CAurTextureBasic::m_DynamicTextureList, nDynamic, &g_dynamicTMS, memcheckratio);
    useddynamictexturememory = g_dynamicTMS.m_anMemory[TMS_TOTAL];
    AurTextureMemCheck(CAurTextureBasic::m_StaticTextureList, maxtexmem - nDynamic, &g_staticTMS, 1.0f);
    usedstatictexturememory = g_staticTMS.m_anMemory[TMS_TOTAL];
    usedbumpmapmemory = g_dynamicTMS.m_anMemory[TMS_BUMPMAP] + g_staticTMS.m_anMemory[TMS_BUMPMAP];
    usedcomptexmemory = g_dynamicTMS.m_anMemory[TMS_COMPRESSED] + g_staticTMS.m_anMemory[TMS_COMPRESSED];
    usedcubemapmemory = g_dynamicTMS.m_anMemory[TMS_CUBEMAP] + g_staticTMS.m_anMemory[TMS_CUBEMAP];
    usedlightmapmemory = g_dynamicTMS.m_anMemory[TMS_LIGHTMAP] + g_staticTMS.m_anMemory[TMS_LIGHTMAP];
    usedtexturememory = useddynamictexturememory + g_staticTMS.m_anMemory[TMS_TOTAL];

#if KOTOR1
    RefreshTextures = CAurTextureBasic::m_OrderedTextureList;
#else
    RefreshTextures.m_count = 0;
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
        RefreshTextures.Add(CAurTextureBasic::m_OrderedTextureList.m_pData[i]);
#endif

    AurTextureBuildAndStoreAll();
    return usedtexturememory <= maxtexmem;
}

void AurTextureRefreshAll()
{
    if (initialized)
    {
        AurFreeFont();
        AurInitFont();
    }
    AurTextureSetMem(maxtexmem);
}

void AurTextureSetModeStatic()
{
    g_bModeIsDynamic = false;
}

void AurTextureSetModeDynamic()
{
    g_bModeIsDynamic = true;
}

CAurTexture* AurTextureGetNormalizationCubeMap()
{
    if (!AurCubeMapAvailable())
        return 0;
    CAurTexture* pTexture = AurTextureFind(const_cast<char*>("NormCubeMap"), const_cast<char*>("NormCubeMap"), 0, 0);
    if (pTexture)
        return pTexture->AsCAurTextureBasic();
    CAurTextureBasic* pCubeMap = new CAurTextureBasic(const_cast<char*>("NormCubeMap"), const_cast<char*>("NormCubeMap"));
    pCubeMap->AsNormalizeVectorCubeMap();
    return pCubeMap;
}

CAurTexture* AurTextureGetReference(char* pszName, char* pszExtension)
{
    return AurTextureGet(0, AurTextureGet(pszName, 0, pszExtension, 0, 0), 0, 0, 0);
}

int AurTextureSetAttributes(CAurTexture* pTexture, char* pszAttributes)
{
    if (!pTexture)
        return 0;
    CAurTextureBasic* pBasic = pTexture->GetReferencedTexture()->AsCAurTextureBasic();
    pBasic->Init(pszAttributes);
    RefreshTextures.AddUnique(pBasic);
    return 1;
}

int AurTextureMessage(CAurTexture* pTexture, char* pszLine)
{
    if (!pTexture)
        return 0;
    CAurTextureBasic* pBasic = pTexture->GetReferencedTexture()->AsCAurTextureBasic();
    if (!pBasic)
        return 0;
    pBasic->ParseField(pszLine);
    RefreshTextures.AddUnique(pBasic);
    return 1;
}

// Stub, always reports failure.
int AurTextureSetDim(int nWidth, int nHeight)
{
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);
    return 0;
}

int AurTextureGetMaxTexID()
{
    return maxtexid;
}

// Kept for the symbol; the clearing work lives in AurTextureFreeAll.
void AurTextureClearAll()
{
}

// Replaces the dirty queue with the whole ordered list, then Resets every entry.
void AurTextureResetAll()
{
#if KOTOR1  // K1 copies the list in one assignment
    RefreshTextures = CAurTextureBasic::m_OrderedTextureList;
#else
    RefreshTextures.m_count = 0;
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
        RefreshTextures.Add(CAurTextureBasic::m_OrderedTextureList.m_pData[i]);
#endif

    for (int i = 0; i < RefreshTextures.m_count; i++)
        RefreshTextures.m_pData[i]->Reset();
}

int AurTextureGetMem()
{
    return maxtexmem;
}

// End-of-frame bookkeeping that drains the dirty queue into the upload pass, folding this frame's
// durations into the running maxima, then advances the frame counters.
void AurTextureMakeCurrent()
{
    if (!g_bIsSuspended)
    {
        constructimagetime = 0;
        decodeimagetime = 0;
        glimagetime = 0;
        if (RefreshTextures.m_count)
        {
            AurTextureBuildAndStoreAll();
            if (maxtextime < currenttextime)
                maxtextime = currenttextime;
            if (maxtexloadtime < loadimagetime)
                maxtexloadtime = loadimagetime;
            if (maxdecodetime < decodeimagetime)
                maxdecodetime = decodeimagetime;
            if (maxgltime < glimagetime)
                maxgltime = glimagetime;
            loadimagetime = 0;
        }
    }
    if (DeltaT > maxdeltat)
        maxdeltat = DeltaT;
    texcurrentframe++;
}

void AurTextureForceControllerUpdate()
{
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_OrderedTextureList.m_pData[i];
        if (pTexture->m_bBound && pTexture->m_pController)
        {
            pTexture->Bind();
            pTexture->m_bBound = 0;
        }
    }
}

// Raster footprint of the live dynamic textures, in bytes.
int AuroraQueryTextureMemoryInUse()
{
    int nTotal = 0;
    int i = 0;
    while (i < CAurTextureBasic::m_DynamicTextureList.m_count)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_DynamicTextureList.m_pData[i];
        int nLevel = pTexture->m_nDownSampleLevelBuilt;
        int nWidth = pTexture->m_nWidth << nLevel;
        int nHeight = pTexture->m_nHeight << nLevel;
        int nBPP = pTexture->m_nBPP;
        int nSize;
        if (pTexture->m_nCompressedSize >= 1)
        {
            int nBlockSize = nBPP == 4 ? 16 : 8;
            nSize = ((nWidth + 3) / 4) * ((nHeight + 3) / 4) * nBlockSize;
        }
        else
        {
            if (nBPP == 3)
                nBPP = 4;
            nSize = nWidth * nHeight * nBPP;
            if (pTexture->m_bMipMap)
                nSize = nSize * 4 / 3;
        }
        nTotal += nSize;
        i++;
    }
    return nTotal;
}

extern List<int> TextureIndices;

int AurTextureGetAvailableTextureMemory()
{
    AurTextureFreeAll();
    int nAvailable = GLRender::GetAvailableTextureMemory();

    RefreshTextures = CAurTextureBasic::m_OrderedTextureList;

    AurTextureBuildAndStoreAll();
    return nAvailable;
}

// Rebuilds the ordering array from the static textures and returns it.
List<int>* AurTextureGetOrdering()
{
    TextureIndices.m_count = 0;
    maxtexid = 0;
    for (int i = 0; i < CAurTextureBasic::m_StaticTextureList.m_count; i++)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_StaticTextureList.m_pData[i];
        if (pTexture->m_bInvalid)
            continue;
        if (pTexture->m_nNumX * pTexture->m_nNumY < 1)
            continue;
        for (int j = 0; j < pTexture->m_nNumX * pTexture->m_nNumY; j++)
        {
#if KOTOR1  // K1 lists only ids 1..4999
            if (pTexture->m_panTIDIndex[j] > 0 && pTexture->m_panTIDIndex[j] < 5000)
#else
            if (pTexture->m_panTIDIndex[j])
#endif
            {
                TextureIndices.Add(pTexture->m_panTIDIndex[j]);
                if (static_cast<unsigned int>(pTexture->m_panTIDIndex[j]) > maxtexid)
                    maxtexid = pTexture->m_panTIDIndex[j];
            }
        }
    }
    return &TextureIndices;
}


extern float g_fDynamicMemoryRatio;

void AurTextureSetDynamicMemoryRatio(float fRatio)
{
    g_fDynamicMemoryRatio = fRatio;
}

#if KOTOR1
// K1: the hash of the texture a reference resolves to, 0 when it is not a basic texture.
unsigned short GetTextureBasicHash(CAurTexture* pTexture)
{
    CAurTextureBasic* pBasic = static_cast<CAurTextureBasic*>(pTexture->GetReferencedTexture()->AsCAurTextureBasic());
    if (pBasic)
        return pBasic->m_nHashID;
    return 0;
}
#endif

// Sort comparator for descending alpha mean, dereferencing the slot pointers.
int texturecmp(const void* pvA, const void* pvB)
{
    CAurTextureBasic* pA = *static_cast<CAurTextureBasic* const*>(pvA);
    CAurTextureBasic* pB = *static_cast<CAurTextureBasic* const*>(pvB);
    return (pA->m_fAlphaMean < pB->m_fAlphaMean) ? 1 : -1;
}

// Three-way block exchange. The scratch buffer is not freed.
void Swap(void* pvA, void* pvB, uint64_t nBytes)
{
    char* pTemp = new char[nBytes];
    memcpy(pTemp, pvA, nBytes);  // idiom-exempt: opaque block exchange, nBytes has no element type
    memcpy(pvA, pvB, nBytes);  // idiom-exempt: opaque block exchange, nBytes has no element type
    memcpy(pvB, pTemp, nBytes);  // idiom-exempt: opaque block exchange, nBytes has no element type
}

void SwapTex(void* pvA, void* pvB)
{
    void* pTemp = *static_cast<void**>(pvA);
    *static_cast<void**>(pvA) = *static_cast<void**>(pvB);
    *static_cast<void**>(pvB) = pTemp;
}

// Insertion sort over 8-byte slots, ordered by pCompare.
void InsertionSort(void* pvData, uint64_t nCount, uint64_t nElemSize,
                   int (*pCompare)(const void*, const void*))
{
    unsigned int nElements = static_cast<unsigned int>(nCount);
    if (static_cast<int>(nElements) < 2)
        return;

    char* pData = static_cast<char*>(pvData);
    for (uint64_t i = 1; i < nElements; i++)
    {
        for (uint64_t j = i + 1; j > 1; j--)
        {
            char* pA = pData + (j - 2) * nElemSize;
            char* pB = pA + nElemSize;
            if (pCompare(pB, pA) >= 0)
                break;
            void* pTempA = *reinterpret_cast<void**>(pA);
            void* pTempB = *reinterpret_cast<void**>(pB);
            *reinterpret_cast<void**>(pB) = pTempA;
            *reinterpret_cast<void**>(pA) = pTempB;
        }
    }
}

void GetNewDimensions(int* panDimensions)
{
    K2_UNUSED(panDimensions);
}

void InsertTexture(CAurTextureBasic* pTexture)
{
    BinaryInsertTexture(pTexture, 0, CAurTextureBasic::m_OrderedTextureList.m_count - 1);
}

// Insertion sort of texture pointers, descending by alpha mean.
void InsertionSortTexture(CAurTextureBasic** papTexture, int nCount)
{
    for (int i = 1; i < nCount; i++)
    {
        CAurTextureBasic* pKey = papTexture[i];
        for (int j = i + 1; j > 1 && pKey->m_fAlphaMean > papTexture[j - 2]->m_fAlphaMean; j--)
        {
            CAurTextureBasic* pTemp = papTexture[j - 1];
            papTexture[j - 1] = papTexture[j - 2];
            papTexture[j - 2] = pTemp;
        }
    }
}


// Searches recursively.
CAurTextureBasic* BinarySearchTexture(char* pszName, int nLow, int nHigh)
{
    if (nHigh < nLow)
        return 0;
    int nMid = (nLow + nHigh) / 2;
    int nResult = _stricmp(pszName, CAurTextureBasic::m_OrderedTextureList.m_pData[nMid]->m_szName);
    if (nResult == 0)
        return CAurTextureBasic::m_OrderedTextureList.m_pData[nMid];
    if (nResult < 0)
        return BinarySearchTexture(pszName, nLow, nMid - 1);
    return BinarySearchTexture(pszName, nMid + 1, nHigh);
}

CAurTextureBasic* BinaryFindTexture(char* pszName)
{
    return BinarySearchTexture(pszName, 0, CAurTextureBasic::m_OrderedTextureList.m_count - 1);
}

// Binary search for the slot the texture belongs in, then Insert there, so the ordered list
// stays name-sorted as well as hash-sorted.
// Searches recursively.
void BinaryInsertTexture(CAurTextureBasic* pTexture, int nLow, int nHigh)
{
    if (nHigh < nLow) {
        CAurTextureBasic::m_OrderedTextureList.Insert(pTexture, nLow);
        return;
    }
    int nMid = (nLow + nHigh) / 2;
    if (_stricmp(pTexture->m_szName, CAurTextureBasic::m_OrderedTextureList.m_pData[nMid]->m_szName) < 0)
        BinaryInsertTexture(pTexture, nLow, nMid - 1);
    else
        BinaryInsertTexture(pTexture, nMid + 1, nHigh);
}

void AurTextureCalculateNewTexMem(List<CAurTextureBasic*>& listTexture, TEXTUREMEMORYSTATS* pStats)
{
    pStats->m_anMemory[TMS_TOTAL] = 0;
    pStats->m_anMemory[TMS_LIGHTMAP] = 0;
    pStats->m_anMemory[TMS_COMPRESSED] = 0;
    pStats->m_anMemory[TMS_CUBEMAP] = 0;
    pStats->m_anMemory[TMS_BUMPMAP] = 0;

    for (int64_t i = 0; i < listTexture.m_count; i++) {
        CAurTextureBasic* pTexture = listTexture.m_pData[i];
        if (!pTexture->m_bFullyLoaded)
            pTexture->ConstructImage();
        if (pTexture->m_bTemporary)
            continue;
        TEXTUREMEMORYSTATS stats;
        pTexture->GetMemoryUsage(&stats);
        pStats->m_anMemory[TMS_BUMPMAP] += stats.m_anMemory[TMS_BUMPMAP];
        pStats->m_anMemory[TMS_TOTAL] += stats.m_anMemory[TMS_TOTAL];
        pStats->m_anMemory[TMS_LIGHTMAP] += stats.m_anMemory[TMS_LIGHTMAP];
        pStats->m_anMemory[TMS_COMPRESSED] += stats.m_anMemory[TMS_COMPRESSED];
        pStats->m_anMemory[TMS_CUBEMAP] += stats.m_anMemory[TMS_CUBEMAP];
    }
}

List<CAurTextureBasic*> RefreshTextures;

extern float DeltaT;
extern float currentoverflow;
extern float maxtimeformemoryoverflow;

template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item);

bool CAurTextureBasic::CanDownSampleFurther()
{
    if (m_nDownSampleLevel >= m_nMaxDownSampleLevel)
        return false;
    int nShift = m_nDownSampleLevel - m_nDownSampleLevelBuilt;
    if ((m_nWidth >> nShift) < 3)
        return false;
    return (m_nHeight >> nShift) > 2;
}

void CAurTextureBasic::SetNewDownSampleLevel(int nAmount)
{
    int nLevel = m_nDownSampleLevel + nAmount;
#if KOTOR1
    nLevel = (nLevel > m_nMaxDownSampleLevel) ? m_nMaxDownSampleLevel : nLevel;
#else
    nLevel = (nLevel < m_nMaxDownSampleLevel) ? nLevel : m_nMaxDownSampleLevel;
#endif
    m_nDownSampleLevel = static_cast<short>(nLevel);
    if (m_nDownSampleLevelBuilt == m_nDownSampleLevel)
        return;
    m_bMemoryDirty = 1;
    RefreshTextures.AddUnique(this);
}

// Checks the memory budget each frame. While the total exceeds the tolerated share of the budget,
// it down-samples whichever texture frees the most bytes, then re-accounts its usage.
void AurTextureMemCheck(List<CAurTextureBasic*>& listTexture, int nMaxSize,
                        TEXTUREMEMORYSTATS* pStats, float fRatio)
{
    AurTextureCalculateNewTexMem(listTexture, pStats);

    float fLimit = static_cast<float>(nMaxSize) * fRatio;
    if (static_cast<float>(pStats->m_anMemory[TMS_TOTAL]) > fLimit) {
        currentoverflow += DeltaT;

        if (pStats->m_anMemory[TMS_TOTAL] > nMaxSize || currentoverflow > maxtimeformemoryoverflow) {
            currentoverflow = 0;

            while (static_cast<float>(pStats->m_anMemory[TMS_TOTAL]) > fLimit) {
                CAurTextureBasic* pBest = 0;
                int nBestTotal = 0;
                int nBestGain = 0;

                for (int64_t i = 0; i < listTexture.m_count; i++) {
                    CAurTextureBasic* pTexture = listTexture.m_pData[i];
                    if (!pTexture->CanDownSampleFurther())
                        continue;

                    int nGain = pTexture->m_nMaxDownSampleLevel - pTexture->m_nDownSampleLevel;
                    TEXTUREMEMORYSTATS stats;
                    pTexture->GetMemoryUsage(&stats);
                    if (stats.m_anMemory[TMS_TOTAL] > nBestTotal ||
                        (stats.m_anMemory[TMS_TOTAL] == nBestTotal && nGain > nBestGain)) {
                        pBest = pTexture;
                        nBestGain = nGain;
                        nBestTotal = stats.m_anMemory[TMS_TOTAL];
                    }
                }

                if (pBest == 0)
                    break;

                TEXTUREMEMORYSTATS stats;
                pBest->GetMemoryUsage(&stats);
                pStats->m_anMemory[TMS_TOTAL] -= stats.m_anMemory[TMS_TOTAL];
                pStats->m_anMemory[TMS_LIGHTMAP] -= stats.m_anMemory[TMS_LIGHTMAP];
                pStats->m_anMemory[TMS_COMPRESSED] -= stats.m_anMemory[TMS_COMPRESSED];
                pStats->m_anMemory[TMS_CUBEMAP] -= stats.m_anMemory[TMS_CUBEMAP];
                pStats->m_anMemory[TMS_BUMPMAP] -= stats.m_anMemory[TMS_BUMPMAP];

                pBest->SetNewDownSampleLevel(1);

                pBest->GetMemoryUsage(&stats);
                pStats->m_anMemory[TMS_TOTAL] += stats.m_anMemory[TMS_TOTAL];
                pStats->m_anMemory[TMS_LIGHTMAP] += stats.m_anMemory[TMS_LIGHTMAP];
                pStats->m_anMemory[TMS_COMPRESSED] += stats.m_anMemory[TMS_COMPRESSED];
                pStats->m_anMemory[TMS_CUBEMAP] += stats.m_anMemory[TMS_CUBEMAP];
                pStats->m_anMemory[TMS_BUMPMAP] += stats.m_anMemory[TMS_BUMPMAP];
            }
        }
    } else {
        currentoverflow = 0;
    }
}

// Rotates an image by nQuarter right angles through a scratch buffer and returns the scratch.
// anX/anY hold the source coordinate of each destination pixel, row-major by nWidth.
unsigned char* ImageQuarterRotated(unsigned char* pImage, int nWidth, int nHeight, int nBPP,
                                   int nQuarter)
{
    unsigned char* pScratch = new unsigned char[nHeight * nWidth * nBPP];

    int nQ = nQuarter % 4;
    if (nQ < 0)
        nQ += 4;

    for (int i = 0; i < nHeight; i++) {
        for (int j = 0; j < nWidth; j++) {
            int anX[4];
            int anY[4];
            anX[0] = j;
            anY[0] = i;
            anX[1] = nHeight - 1 - i;
            anY[1] = j;
            anX[2] = nWidth - 1 - j;
            anY[2] = nHeight - 1 - i;
            anX[3] = i;
            anY[3] = nWidth - 1 - j;

            memcpy(&pScratch[(anY[nQ] * nWidth + anX[nQ]) * nBPP],
                   &pImage[(i * nWidth + j) * nBPP], nBPP);
        }
    }

    return pScratch;
}

List<int> TextureIndices;
extern unsigned int maxtexid;
extern int basictexturecount;
extern bool g_bTextureRefsGotRefreshed;

// Drains the dirty queue into the upload pass, re-sorts the static list by alpha mean and
// rebuilds the ordering array.
void AurTextureBuildAndStoreAll()
{
    if (RefreshTextures.m_count == 0)
        return;
#if !KOTOR1  // K1 has no refreshed-references flag
    g_bTextureRefsGotRefreshed = true;
#endif
    int i = RefreshTextures.m_count;
    while (i--)
    {
        RefreshTextures.m_pData[i]->BuildAndStoreTexture();
        RefreshTextures.m_count = i;
    }

    InsertionSortTexture(CAurTextureBasic::m_StaticTextureList.m_pData,
                         CAurTextureBasic::m_StaticTextureList.m_count);

    basictexturecount = CAurTextureBasic::m_OrderedTextureList.m_count;
    TextureIndices.m_count = 0;
    maxtexid = 0;
    for (int nTex = 0; nTex < CAurTextureBasic::m_StaticTextureList.m_count; nTex++)
    {
        CAurTextureBasic* pTexture = CAurTextureBasic::m_StaticTextureList.m_pData[nTex];
        if (pTexture->m_bInvalid)
            continue;
        if (pTexture->m_nNumX * pTexture->m_nNumY < 1)
            continue;
        for (int j = 0; j < pTexture->m_nNumX * pTexture->m_nNumY; j++)
        {
            TextureIndices.Add(pTexture->m_panTIDIndex[j]);
            if (static_cast<unsigned int>(pTexture->m_panTIDIndex[j]) > maxtexid)
                maxtexid = pTexture->m_panTIDIndex[j];
        }
    }
}

unsigned char* paletteimage;
int            paletteheight;
int            alwaysbind;
int            countbinds;
void           (*AurReleaseRaster)(void*, int, int, int, int, int, int, int);

unsigned int palettewidth = 256;
unsigned int palettebpp = 4;

int  AurCubeMapAvailable();
void ImageCopy(unsigned char* pDst, const unsigned char* pSrc, int nWidth, int nHeight, int nBpp);
void ImageFlipY(unsigned char* pData, int nWidth, int nHeight, int nBpp);



enum {
    kCapTexture2D = 4,
    kCapCubeMap   = 12,
    kCapTexture3D = 20
};

enum {
    kBlockSizeDXT1 = 8,
    kBlockSizeDXT5 = 16
};


void AurClearPalette()
{
    delete[] paletteimage;
    paletteimage = 0;
    paletteheight = 0;
}

// Loads a raster into palette slot nIndex; the palette image grows in 256-row slots.
int AurSetPalette(int nIndex, char* pszName)
{
    int nNewHeight = nIndex * 256 + 256;
    if (nNewHeight > paletteheight)
    {
        unsigned char* pOld = paletteimage;
        int nOldHeight = paletteheight;
        paletteheight = nNewHeight;
        int nNewSize = palettewidth * nNewHeight * palettebpp;
        paletteimage = new unsigned char[nNewSize];
        if (pOld != 0)
        {
            ImageCopy(paletteimage, pOld, palettewidth, nOldHeight, palettebpp);
            delete[] pOld;
        }
    }

    int nWidth;
    int nHeight;
    int nBPP;
    int nCompressedSize;
    int nRasterSize;
    int nTxiSize;
    Aurora::AuroraTextureFormats eFormat;
    float fAlphaMean;
    bool bProcessed;
    short nNumMipMaps;

    unsigned char* pImage = static_cast<unsigned char*>(AurGetImageWrapper(
        pszName, &nWidth, &nHeight, &nBPP, &nCompressedSize, &nRasterSize, &nTxiSize, &eFormat,
        &fAlphaMean, &bProcessed, &nNumMipMaps, 0));
    if (pImage == 0)
        return -1;

    ImageFlipY(pImage, nWidth, nHeight, nBPP);
    int nPageOffset = nIndex << 18;
    ImageCopy(paletteimage + nPageOffset, pImage, nWidth, nHeight, nBPP);

    int nFaces = nHeight / nWidth;
    int nH = (nFaces == 6) ? nHeight / 6 : nHeight;
    int nBlockSize = (nBPP == 4) ? kBlockSizeDXT5 : kBlockSizeDXT1;
    int nSize;
    if (nCompressedSize != 0)
    {
        nSize = nCompressedSize;
        int nW = nWidth;
        for (int i = 0; i < nNumMipMaps - 1; i++)
        {
            nW >>= 1;
            nH >>= 1;
            nSize += ((nW + 3) / 4) * nBlockSize * ((nH + 3) / 4);
        }
        nSize *= (nFaces == 6) ? 6 : 1;
    }
    else
    {
        nSize = 0;
    }

    if (AurReleaseRaster != 0)
        AurReleaseRaster(pImage, nWidth, nHeight, nBPP, nSize, nRasterSize, nTxiSize, bProcessed);
    else
        delete[] pImage;

    return nIndex;
}

TextureController* AurTextureGetController(CAurTexture* pTexture)
{
    return pTexture->GetReferencedTexture()->AsCAurTextureBasic()->m_pController;
}

#if KOTOR1
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
int AurAnisotropicFilterAvailable();
#endif
// K1: no 3D cube-strip target and no anisotropy parameter.
void AurTextureBindTID(unsigned int nTID, bool bCubeMap)
{
    bool bCube = false;
    if (AurCubeMapAvailable())
    {
        if (bCubeMap)
        {
            GLRender::Enable(Aurora::kCapTextureCubeMap);
            bCube = true;
        }
        else
        {
            GLRender::Disable(Aurora::kCapTextureCubeMap);
        }
    }

    if (nTID >= 2 && alwaysbind < 0)
        return;

    unsigned int* pTarget = bCube ? &enableTranslationTable[kCapCubeMap] : &enableTranslationTable[kCapTexture2D];
    ASLgl::glBindTexture(*pTarget, nTID);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // The Advanced Graphics page's anisotropy, applied on bind as KOTOR II does.
    if (AurAnisotropicFilterAvailable() && GLRender::cm_nAnisotropy >= 1)
        ASLgl::glTexParameteri(*pTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT /* GL_TEXTURE_MAX_ANISOTROPY_EXT */,
                               GLRender::cm_nAnisotropy);
#endif
    GLRender::cm_nGLCurrentTID[GLRender::cm_uiCurrentStage] = nTID;
    countbinds++;
}
#else
// Makes nTID the bound texture, selecting the target from the cube-map and 3D-strip flags,
// and sets the anisotropy parameter on the same target.
void AurTextureBindTID(unsigned int nTID, bool bCubeMap, bool bCubeStrip)
{
    bool bCube = false;
    if (AurCubeMapAvailable())
    {
        if (bCubeMap)
        {
            GLRender::Enable(Aurora::kCapTextureCubeMap);
            bCube = true;
        }
        else
        {
            GLRender::Disable(Aurora::kCapTextureCubeMap);
        }
    }

    if (bCubeStrip)
        GLRender::Enable(Aurora::kCapTexture3D);
    else
        GLRender::Disable(Aurora::kCapTexture3D);

    if (nTID >= 2 && alwaysbind < 0)
        return;

        // The table is indexed in bytes here because the slot is chosen at runtime.
    if (bCube)
        GLRender::BindTexture(Aurora::kCapTextureCubeMap, nTID);
    else if (bCubeStrip)
        GLRender::BindTexture(Aurora::kCapTexture3D, nTID);
    else
        GLRender::BindTexture(Aurora::kCapTexture2D, nTID);
    countbinds++;
}
#endif


#if KOTOR1
void AurSnapshot(char* pszFile, bool bWrite);
void AurSaveGameSnapshot(char* pszFile, int nDimension, bool bFlag, bool bSquare);
#else
void AurSnapshot(char* pszFile);
void AurSaveGameSnapshot(char* pszFile, int nDimension, bool bFlag);
#endif
int  AurSetPalette(int nIndex, char* pszName);
int  AurTextureSetMem(int nMem);


int AurTextureSprintf(char* pszBuffer, uint64_t nSize, const char* pszFormat, ...);
void ImageFlipY(unsigned char* pData, int nWidth, int nHeight, int nBpp);
#if KOTOR1
void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData);
#else
void ImageWriteTGA(char* pszName, int nWidth, int nHeight, int nBpp, unsigned char* pData,
                   bool bFlipY);
#endif
unsigned char* ImageScale(unsigned char* pData, int nSrcWidth, int nSrcHeight, int nBpp,
                         int nDestWidth, int nDestHeight);
Gob* FindGob(char* pszName);

int skippauseplaneforscreenshot;
extern const unsigned int textureFormatTranslationTable[];
static int g_nSnapshotCounter;
static int g_nSaveGameSnapshotCounter;
static char szResult[256];
extern int usedtexturememory;

CAurTexture* AurTextureFind(char* pszName, char* pszFilter, int* pnOffset, int nType);
char* skipspace(char* pszLine);
void AurTextureEnableLowEndEnvMapping(Vector vColor);
void AurTextureDisableLowEndEnvMapping();

template <> void List<CAurTextureBasic*>::AddUnique(CAurTextureBasic* item);
extern List<CAurTextureBasic*> RefreshTextures;

extern bool g_bIsSuspended;

// A height-field dimension is usable when it is 0, a power of two, or one more than a power of
// two; the last is cropped down, since a field of N quads carries N+1 samples per row.
static int IsPowerOf2(int n)
{
    return (n & (n - 1)) == 0;
}

// Converts a height field to a packed normal map. Each texel's normal comes from the forward
// differences to its right and down neighbours, wrapping at the field edges.
Normal* convertHeightFieldToNormalMap(unsigned char* pData, int nWidth, int nHeight, int nStride,
                                      int nSrcHeight, float fScale)
{
    Normal* pOut = new Normal[nHeight * nWidth];
    for (int y = 0; y < nHeight; y++) {
        int nRow = y * nStride;
        int nRowDown = ((y + 1) % nSrcHeight) * nStride;
        for (int x = 0; x < nWidth; x++) {
            float fCenter = pData[nRow + x] * (1.0f / 255.0f);
            float fRight = pData[nRow + (x + 1) % nStride] * (1.0f / 255.0f);
            float fDown = pData[nRowDown + x] * (1.0f / 255.0f);
            float fDx = (fCenter - fRight) * fScale;
            float fDy = (fCenter - fDown) * fScale;
            float fInvLen = 1.0f / sqrtf(fDx * fDx + fDy * fDy + 1.0f);
            Normal* pNormal = &pOut[y * nWidth + x];
            pNormal->m_bX = static_cast<unsigned char>(fInvLen * 127.0f + 128.0f);
            pNormal->m_bW = 255;
            pNormal->m_bZ = static_cast<unsigned char>(fDy * fInvLen * 127.0f + 128.0f);
            pNormal->m_bY = static_cast<unsigned char>(128.0f - fDx * fInvLen * 127.0f);
        }
    }
    return pOut;
}

// Halves a normal map: each 2x2 block collapses to one texel, its four normals averaged as
// weighted vectors, renormalised and re-encoded.
Normal* downSampleNormalMap(Normal* pData, int nWidth, int nHeight, int nNewWidth, int nNewHeight)
{
    Normal* pNew = new Normal[nNewWidth * nNewHeight];
    if (nHeight < 1) {
        delete[] pData;
        return pNew;
    }
    for (int nY = 0; nY < nHeight; nY += 2) {
        for (int nX = 0; nX < nWidth; nX += 2) {
            const Normal* p1 = &pData[nY * nWidth + nX];
            const Normal* p2 = &pData[nY * nWidth + (nX + 1) % nHeight];
            const Normal* p3 = &pData[((nY + 1) % nWidth) * nWidth + nX];
            const Normal* p4 = &pData[((nY + 1) % nWidth) * nWidth + (nX + 1) % nHeight];
            float fSumZ = (static_cast<float>(p1->m_bZ) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p1->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p2->m_bZ) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p2->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p3->m_bZ) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p3->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p4->m_bZ) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p4->m_bW) * (1.0f / 255.0f));
            float fSumY = (static_cast<float>(p1->m_bY) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p1->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p2->m_bY) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p2->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p3->m_bY) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p3->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p4->m_bY) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p4->m_bW) * (1.0f / 255.0f));
            float fSumX = (static_cast<float>(p1->m_bX) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p1->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p2->m_bX) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p2->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p3->m_bX) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p3->m_bW) * (1.0f / 255.0f))
                        + (static_cast<float>(p4->m_bX) * (1.0f / 127.0f) - 1.0f) * (static_cast<float>(p4->m_bW) * (1.0f / 255.0f));
            if (nNewWidth == 1 || nNewHeight == 1 || (nNewWidth == 2 && nNewHeight == 2)) {
                fSumX = 1.0f;
                fSumY = 0.0f;
                fSumZ = 0.0f;
            } else if (nNewWidth == 4 && nNewHeight == 4) {
                fSumY *= 0.5f;
                fSumZ *= 0.5f;
            }
            // The degenerate (zero-length) result reuses the encode below.
            float fLength = sqrtf(fSumZ * fSumZ + fSumY * fSumY + fSumX * fSumX);
            if (fLength == 0.0f) {
                fSumX = 1.0f;
                fSumY = 0.0f;
                fSumZ = 0.0f;
            } else {
                float fInv = 1.0f / fLength;
                fSumX *= fInv;
                fSumY *= fInv;
                fSumZ *= fInv;
            }
            float fWeight = fLength * 0.25f;
            float fScaled = fWeight * 255.0f;
            Normal* pOut = &pNew[(nY / 2) * nNewWidth + nX / 2];
            pOut->m_bZ = static_cast<unsigned char>(static_cast<int>(fSumZ * 127.0f + 128.0f));
            pOut->m_bW = static_cast<unsigned char>(fWeight > 1.0f ? 255 : static_cast<int>(fScaled));
            pOut->m_bY = static_cast<unsigned char>(static_cast<int>(fSumY * 127.0f + 128.0f));
            pOut->m_bX = static_cast<unsigned char>(static_cast<int>(fSumX * 127.0f + 128.0f));
        }
    }
    delete[] pData;
    return pNew;
}

// Validates the height-field dimensions (0, a power of two, or one more than one, which is
// cropped down), then converts the field.
int BuildTextureNormalMap(unsigned char* pData, float fScale, int& nWidth, int& nHeight,
                          int nBPP, int nUseSubImage, bool bCopyField)
{
    K2_UNUSED(nBPP);
    int nW = nWidth;
    int nFail = 0;
    if (!IsPowerOf2(nW)) {
        if (IsPowerOf2(nW - 1))
            nWidth = nW - 1;
        else
            nFail = 1;
    }
    int nH = nHeight;
    if (!IsPowerOf2(nH)) {
        if (IsPowerOf2(nH - 1))
            nHeight = nH - 1;
        else
            nFail = 1;
    }
    if (nFail)
        return 0;
    GLRender::ConvertHeightFieldAndLoadNormalMapTexture(pData, nWidth, nHeight, nW, nH,
                                                        fScale, nUseSubImage, bCopyField);
    return 1;
}

// The console command `addpalette <name> <index>`.
const char* addpalette(char* pszArg)
{
    char szName[64];
    int nIndex = 0;
    szName[0] = 0;
    static_cast<void>(sscanf(pszArg, "%s%d", szName, &nIndex));
    if (!szName[0])
        return "no such file";
    return AurSetPalette(nIndex, szName) == -1 ? "ERROR: something didn't work" : "OK";
}

// Takes a screenshot and writes it out as a TGA. A null or "nullptr" filename takes the
// generated `snap<N>.tga` form; any other name gets ".tga" appended when it lacks it.
#if KOTOR1
char* snapshottest(char* pszArgs)
{
    char szFile[32];
    int nRenderGui = 0;
    if (sscanf(pszArgs, "%s %d", szFile, &nRenderGui) != 2)
        return const_cast<char*>("USAGE: snapshottest <filename> <rendergui>");
    AurSnapshot(szFile, nRenderGui == 1);
    return const_cast<char*>("SUCCESS");
}

// K1: writes the pixels back at the 4-byte-padded width unless `bWrite` is false.
void AurSnapshot(char* pszName, bool bWrite)
{
    char szFile[64];
    if (pszName && strcmp(pszName, NULL_NAME) != 0) {
        __strcpy_chk(szFile, pszName, sizeof(szFile));
        int nLen = static_cast<int>(__strlen_chk(szFile, sizeof(szFile)));
        if (nLen < 4 || _stricmp(&szFile[nLen - 4], ".tga") != 0)
            __strcat_chk(szFile, ".tga", sizeof(szFile));
    } else {
        AurTextureSprintf(szFile, sizeof(szFile), "snap%d.tga", ++g_nSnapshotCounter);
    }
    int nViewport[4];
    GLRender::GetViewPort(nViewport);
    int nPitch = nViewport[2] + ((nViewport[2] % 4 != 0) ? 4 - nViewport[2] % 4 : 0);
    if (bWrite) {
        int nHeight = nViewport[3];
        uint64_t nBytes = nPitch * nHeight * 3;
        unsigned char* pPixels = new unsigned char[nBytes];
        memset(pPixels, 0, nBytes);  // idiom-exempt: runtime length (the viewport's pixels)
        GLRender::ReadPixelsFromBuffer(0, 0, nPitch, nHeight, Aurora::kTexFormatRGB, pPixels);
        ImageFlipY(pPixels, nPitch, nHeight, 3);
        ImageWriteTGA(szFile, nPitch, nHeight, 3, pPixels);
        delete[] pPixels;
    } else {
        AurSaveGameSnapshot(pszName, nPitch, false, false);
    }
}
#else
void AurSnapshot(char* pszName)
{
    char szFile[64];
    if (pszName && strcmp(pszName, NULL_NAME) != 0) {
        __strcpy_chk(szFile, pszName, sizeof(szFile));
        int nLen = static_cast<int>(__strlen_chk(szFile, sizeof(szFile)));
        if (nLen < 4 || _stricmp(&szFile[nLen - 4], ".tga") != 0)
            __strcat_chk(szFile, ".tga", sizeof(szFile));
    } else {
        AurTextureSprintf(szFile, sizeof(szFile), "snap%d.tga", ++g_nSnapshotCounter);
    }
    int nViewport[4];
    GLRender::GetViewPort(nViewport);
    int nHeight = nViewport[3];
    int nPitch = nViewport[2] + ((nViewport[2] % 4 != 0) ? 4 - nViewport[2] % 4 : 0);
    int nSize = nPitch * nHeight;
    unsigned char* pPixels = new unsigned char[nSize * 3];
    memset(pPixels, 0, nSize * 3);  // idiom-exempt: 3 is the RGB channel count of a runtime-
    GLRender::ReadPixelsFromBuffer(0, 0, nPitch, nHeight, Aurora::kTexFormatRGB, pPixels);
    ImageFlipY(pPixels, nPitch, nHeight, 3);
    ImageWriteTGA(szFile, nPitch, nHeight, 3, pPixels, true);
    delete[] pPixels;
}
#endif

// The save-game snapshot uses the same filename logic as AurSnapshot with its own sequence number,
// then does a render of the current camera at full resolution, a scale and a write.
#if KOTOR1
// K1: a 512-byte name, the read-back at the 4-byte-padded width, and the padded square only
// when `bSquare` is asked for.
void AurSaveGameSnapshot(char* pszName, int nDimension, bool bFlag, bool bSquare)
{
    char szFile[512];
    if (pszName && strcmp(pszName, NULL_NAME) != 0) {
        __strcpy_chk(szFile, pszName, sizeof(szFile));
        int nLen = static_cast<int>(__strlen_chk(szFile, sizeof(szFile)));
        if (nLen < 4 || _stricmp(&szFile[nLen - 4], ".tga") != 0)
            __strcat_chk(szFile, ".tga", sizeof(szFile));
    } else {
        AurTextureSprintf(szFile, sizeof(szFile), "snap%d.tga", ++g_nSaveGameSnapshotCounter);
    }
    int nViewport[4];
    GLRender::GetViewPort(nViewport);
    int nHeight = nViewport[3];
    int nWidth = nViewport[2] + ((nViewport[2] % 4 != 0) ? 4 - nViewport[2] % 4 : 0);
    int nDim = nDimension;
    if (nWidth > nHeight && !bFlag)
        nDim = static_cast<int>(static_cast<float>(nHeight) * static_cast<float>(nDimension) / static_cast<float>(nWidth));
    Camera* pCamera = static_cast<Camera*>(FindGob(const_cast<char*>("camera")));
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT /* GL_COLOR|GL_DEPTH|GL_STENCIL_BUFFER_BIT */);
    ASLgl::glLoadIdentity();
    skippauseplaneforscreenshot = 1;
    pCamera->RenderScene();
    skippauseplaneforscreenshot = 0;
    uint64_t nBytes = nHeight * nWidth * 3;
    unsigned char* pPixels = new unsigned char[nBytes];
    memset(pPixels, 0, nBytes);  // idiom-exempt: runtime length (the viewport's pixels)
    GLRender::ReadPixelsFromBuffer(0, 0, nWidth, nHeight, Aurora::kTexFormatRGB, pPixels);
    ImageFlipY(pPixels, nWidth, nHeight, 3);
    unsigned char* pScaled = ImageScale(pPixels, nWidth, nHeight, 3, nDimension, nDim);
    if (!bSquare || bFlag) {
        ImageWriteTGA(szFile, nDimension, nDim, 3, pScaled);
    } else {
        int nSquare = nDim > nDimension ? nDim : nDimension;
        unsigned int nSquareBytes = nSquare * nSquare * 3;
        unsigned char* pSquare = new unsigned char[nSquareBytes];
        memset(pSquare, 0x7f, nSquareBytes);  // idiom-exempt: runtime length (the square canvas)
        memcpy(pSquare, pScaled, nDimension * nDim * 3);  // idiom-exempt: runtime length (the scaled image)
        ImageWriteTGA(szFile, nSquare, nSquare, 3, pSquare);
        delete[] pSquare;
    }
    delete[] pScaled;
    delete[] pPixels;
}
#else
void AurSaveGameSnapshot(char* pszName, int nDimension, bool bFlag)
{
    char szFile[260];
    if (pszName && strcmp(pszName, NULL_NAME) != 0) {
        __strcpy_chk(szFile, pszName, sizeof(szFile));
        int nLen = static_cast<int>(__strlen_chk(szFile, sizeof(szFile)));
        if (nLen < 4 || _stricmp(&szFile[nLen - 4], ".tga") != 0)
            __strcat_chk(szFile, ".tga", sizeof(szFile));
    } else {
        AurTextureSprintf(szFile, sizeof(szFile), "snap%d.tga", ++g_nSaveGameSnapshotCounter);
    }
    int nViewport[4];
    GLRender::GetViewPort(nViewport);
    int nWidth = nViewport[2];
    int nHeight = nViewport[3];
    int nDim = nDimension;
    if (nWidth > nHeight && !bFlag)
        nDim = static_cast<int>(static_cast<float>(nHeight) * static_cast<float>(nDimension) / static_cast<float>(nWidth));
    Camera* pCamera = static_cast<Camera*>(FindGob(const_cast<char*>("camera")));
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT /* GL_COLOR|GL_DEPTH|GL_STENCIL_BUFFER_BIT */);
    ASLgl::glLoadIdentity();
    skippauseplaneforscreenshot = 1;
    pCamera->RenderScene();
    skippauseplaneforscreenshot = 0;
    int nPixels = nHeight * nWidth;
    unsigned char* pPixels = new unsigned char[nPixels * 3];
    memset(pPixels, 0, nPixels * 3);  // idiom-exempt: 3 = RGB channels of a runtime-sized buffer
    GLRender::ReadPixelsFromBuffer(0, 0, nWidth, nHeight, Aurora::kTexFormatRGB, pPixels);
    ImageFlipY(pPixels, nWidth, nHeight, 3);
    unsigned char* pScaled = ImageScale(pPixels, nWidth, nHeight, 3, nDimension, nDim);
    if (bFlag) {
        ImageWriteTGA(szFile, nDimension, nDim, 3, pScaled, true);
    } else {
        int nSquare = nDim < nDimension ? nDimension : nDim;
        unsigned int nBytes = nSquare * nSquare * 3;
        unsigned char* pSquare = new unsigned char[nBytes];
        memset(pSquare, 0x7f, nBytes);  // idiom-exempt: 0x7f fill of a runtime-sized RGB byte count
        memcpy(pSquare, pScaled, nDimension * nDim * 3);  // idiom-exempt: same RGB byte count
        ImageWriteTGA(szFile, nSquare, nSquare, 3, pSquare, true);
        delete[] pSquare;
    }
    delete[] pScaled;
    delete[] pPixels;
}
#endif

// The console command `snapshot [<filename>]`; an unparseable argument is passed through as a null
// filename so the default name applies.
const char* snapshot(char* pszArg)
{
    char szArg[64];
#if KOTOR1
    if (sscanf(pszArg, "%s", szArg) == 1)
        AurSnapshot(szArg, true);
    else
        AurSnapshot(0, true);
#else
    if (sscanf(pszArg, "%s", szArg) == 1)
        AurSnapshot(szArg);
    else
        AurSnapshot(0);
#endif
    return "Attempted to save file";
}

// The console command `savegamesnapshot <imagename> <dimension>`.
const char* savegamesnapshot(char* pszArg)
{
    char szBuf[64];
    int nDimension = 0;
    if (sscanf(pszArg, "%s %d", szBuf, &nDimension) != 2)
        return "Usage: savegamesnapshot imagename dimension";
#if KOTOR1
    AurSaveGameSnapshot(szBuf, nDimension, true, true);
#else
    AurSaveGameSnapshot(szBuf, nDimension, true);
#endif
    return "Attempted to save save game file";
}

// The console command `settexturemem <bytes>`.
const char* settexturemem(char* pszArg)
{
    return AurTextureSetMem(atoi(pszArg)) ? "Success" : "Failure";
}

// The console command `setpalettes`.
const char* setpalettes(char* pszArg)
{
    K2_UNUSED(pszArg);
    AurSetPalette(0, const_cast<char*>("pal_skin01"));
    AurSetPalette(1, const_cast<char*>("pal_skin01"));
    AurSetPalette(2, const_cast<char*>("pal_hair01"));
    AurSetPalette(3, const_cast<char*>("pal_armor01"));
    AurSetPalette(4, const_cast<char*>("pal_cloth01"));
    return "Palettes are set";
}

// The console command `texmessage <name> <rest-of-line>` hands the remainder of the line to the
// named texture's .txi parser.
const char* texmessage(char* pszArg)
{
    char szName[64];
    szName[0] = 0;
    int nOffset = 0;
    static_cast<void>(sscanf(pszArg, "%s%n", szName, &nOffset));
    CAurTexture* pTexture = AurTextureFind(szName, szName, 0, 0);
    if (!pTexture)
        return "no texture by that name";
    char* pszRest = skipspace(pszArg + nOffset);
    CAurTextureBasic* pBasic = pTexture->GetReferencedTexture()->AsCAurTextureBasic();
    if (pBasic) {
        pBasic->ParseField(pszRest);
        RefreshTextures.AddUnique(pBasic);
    }
    return "OK";
}

// The console command `replacetexture <gob> <name> [<high low> ...]` retargets a model's textures
// by gob name, either wholesale or as a packed high/low pair array.
const char* replacetexture(char* pszArg)
{
    unsigned short aValues[256];
    char szTexture[64];
    szTexture[0] = 0;
    char* pszSpace = strchr(pszArg, ' ');
    memset(aValues, 0, sizeof(aValues));
    AurTextureSprintf(szResult, 256, "Failure");
    if (!pszSpace)
        return szResult;
    *pszSpace = 0;
    Gob* pGob = FindGob(pszArg);
    if (!pGob) {
        AurTextureSprintf(szResult, 256, "ERROR:  Unable to find gob with name %s", pszArg);
        return szResult;
    }
    static_cast<void>(sscanf(pszSpace, "%s", szTexture));
    if (!_stricmp(szTexture, NULL_NAME))
        szTexture[0] = 0;
    pszSpace = strchr(pszSpace + 1, ' ');
    if (!pszSpace)
        return szResult;
    *pszSpace = 0;
    char* pszName = pszSpace + 1;
    pszSpace = strchr(pszName, ' ');
    int nResult;
    if (!pszSpace) {
        nResult = pGob->ReplaceTexture(szTexture, pszName);
    } else {
        *pszSpace = 0;
        char* pszToken = pszSpace + 1;
        pszSpace = strchr(pszToken, ' ');
        if (!pszSpace)
            return szResult;
        unsigned char nTokens = 2;
        unsigned char nValues = 1;
        char* pszValue = pszSpace;
        do {
            pszValue = strchr(pszValue + 1, ' ');
            nValues++;
            nTokens++;
        } while (pszValue != 0);
        nValues >>= 1;
        if (!nValues)
            return szResult;
        nTokens--;
        uint64_t nPairs = static_cast<uint64_t>(nTokens) >> 1;
        unsigned short* pPacked = aValues;
        do {
            char* pszEnd = strchr(pszToken, ' ');
            *pszEnd = 0;
            int nHigh = atoi(pszToken);
            char* pszNext = strchr(pszEnd + 1, ' ');
            if (pszNext)
                *pszNext = 0;
            int nLow = atoi(pszEnd + 1);
            pszToken = pszNext + 1;
            *pPacked++ = static_cast<unsigned short>((nLow & 0xff) | ((nHigh & 0xff) << 8));
        } while (--nPairs);
        nResult = pGob->ReplaceTexture(szTexture, pszName, nValues, aValues, 0);
    }
    if (nResult)
        AurTextureSprintf(szResult, 256, "Success: %i", usedtexturememory);
    else
        AurTextureSprintf(szResult, 256, "WARNING: no parts on model use texture %s", szTexture);
    return szResult;
}

void AurTextureSuspendUpdates()
{
    g_bIsSuspended = true;
}

void AurTextureResumeUpdates()
{
    g_bIsSuspended = false;
}

// Turns the low-end environment-map fallback on and re-queues every environment-mapped texture
// in both lists for a re-upload.
void AurTextureEnableLowEndEnvMapping(Vector vColor)
{
    CAurTextureBasic::m_vLowEndEnvMapColor = vColor;
    CAurTextureBasic::m_bLowEndEnvMapEnabled = true;
    for (int i = 0; i < CAurTextureBasic::m_StaticTextureList.m_count; i++) {
        if (CAurTextureBasic::m_StaticTextureList.m_pData[i]->m_bEnvironmentMapped) {
            CAurTextureBasic::m_StaticTextureList.m_pData[i]->Reset();
            RefreshTextures.Add(CAurTextureBasic::m_StaticTextureList.m_pData[i]);
        }
    }
    for (int i = 0; i < CAurTextureBasic::m_DynamicTextureList.m_count; i++) {
        if (CAurTextureBasic::m_DynamicTextureList.m_pData[i]->m_bEnvironmentMapped) {
            CAurTextureBasic::m_DynamicTextureList.m_pData[i]->Reset();
            RefreshTextures.Add(CAurTextureBasic::m_DynamicTextureList.m_pData[i]);
        }
    }
}

// Turns the fallback off first, then re-queues every environment-mapped texture in both lists
// for a re-upload.
void AurTextureDisableLowEndEnvMapping()
{
    CAurTextureBasic::m_bLowEndEnvMapEnabled = false;
    for (int i = 0; i < CAurTextureBasic::m_StaticTextureList.m_count; i++) {
        if (CAurTextureBasic::m_StaticTextureList.m_pData[i]->m_bEnvironmentMapped) {
            CAurTextureBasic::m_StaticTextureList.m_pData[i]->Reset();
            RefreshTextures.Add(CAurTextureBasic::m_StaticTextureList.m_pData[i]);
        }
    }
    for (int i = 0; i < CAurTextureBasic::m_DynamicTextureList.m_count; i++) {
        if (CAurTextureBasic::m_DynamicTextureList.m_pData[i]->m_bEnvironmentMapped) {
            CAurTextureBasic::m_DynamicTextureList.m_pData[i]->Reset();
            RefreshTextures.Add(CAurTextureBasic::m_DynamicTextureList.m_pData[i]);
        }
    }
}

bool AurTextureEnvMapDisabled()
{
    return CAurTextureBasic::m_bLowEndEnvMapEnabled;
}

// The console command `enablelowendenvmapping <r> <g> <b>`.
const char* enablelowendenvmapping(char* pszArg)
{
    Vector vColor;
    if (sscanf(pszArg, "%f %f %f", &vColor.x, &vColor.y, &vColor.z) != 3)
        return "Usage: enablelowendenvmapping red green blue";
    AurTextureEnableLowEndEnvMapping(vColor);
    return "Success.";
}

// The console command `disablelowendenvmapping`; the argument is unused.
const char* disablelowendenvmapping(char* pszArg)
{
    K2_UNUSED(pszArg);
    AurTextureDisableLowEndEnvMapping();
    return "Success.";
}

// qsort comparator for a name list.
int sorttexturelist(const void* pA, const void* pB)
{
    return strcmp(static_cast<const char*>(pA), static_cast<const char*>(pB));
}

// Console command that writes every known texture to `texturelist.txt`, one `name.ext - <bytes>`
// line per texture, sorted by name, then a count and total footer.
const char* listalltextures(char* pszArg)
{
    K2_UNUSED(pszArg);
    char aNames[1000][50];
    char szPad[] = "                                ";
    TEXTUREMEMORYSTATS stats;
#if KOTOR1
    FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/AURORA/aurtexman.cpp",
                         "listalltextures", 5629, "texturelist.txt", "wa");
#else
    FILE* pFile = fopen("texturelist.txt", "wa");
#endif
    if (!pFile)
        return "Failure.";
    int nCount = 0;
    int nTotal = 0;
    for (int i = 0; i < CAurTextureBasic::m_OrderedTextureList.m_count; i++) {
#ifdef K2_FIX_ENGINE_BUGS
        if (nCount == static_cast<int>(sizeof(aNames) / sizeof(aNames[0])))
            break;
#endif
        CAurTextureBasic* pTexture = CAurTextureBasic::m_OrderedTextureList.m_pData[i];
        if (strstr(pTexture->m_szResRef, "_lm") || strstr(pTexture->m_szResRef, "_a00"))
            continue;
        pTexture->GetMemoryUsage(&stats);
        int nCompressed = pTexture->GetCompressedSize();
        AurTextureSprintf(aNames[nCount], static_cast<uint64_t>(-1), nCompressed == 0 ? "%s.tga - %d\n" : "%s.dds - %d\n",
                          pTexture->m_szResRef, stats.m_anMemory[TMS_TOTAL]);
        nCount++;
        nTotal += stats.m_anMemory[TMS_TOTAL];
    }
    qsort(aNames, nCount, sizeof(aNames[0]), sorttexturelist);
    for (int i = 0; i < nCount; i++) {
        unsigned int nJ;
        for (nJ = 0; nJ < strlen(aNames[i]); nJ++) {
            if (aNames[i][nJ] == '.' && (aNames[i][nJ + 1] | 0x10) == 't')
                break;
        }
        fwrite(szPad, 1, 32 - nJ, pFile);
        fwrite(aNames[i], 1, strlen(aNames[i]), pFile);
    }
    AurTextureSprintf(aNames[0], sizeof(aNames[0]),
                      "\n\n                                %d - %d\n", nCount, nTotal);
    fwrite(aNames[0], 1, __strlen_chk(aNames[0], sizeof(aNames)), pFile);
    fclose(pFile);
    return "Success.";
}

int ImageGetS3TCSize(int, int, int);

// Fills the memory record for this texture, classifying the usage as lightmap, cubemap,
// bumpmap or compressed.
void CAurTextureBasic::GetMemoryUsage(TEXTUREMEMORYSTATS* pStats)
{
    if (m_bMemoryDirty) {
        if (m_bFullyLoaded && m_pNullTexture != this) {
            int nShift = m_nDownSampleLevel - m_nDownSampleLevelBuilt;
            if (m_nCompressedSize >= 1) {
                m_nMemoryUsage = ImageGetS3TCSize(m_nWidth >> nShift, m_nHeight >> nShift, m_nBPP);
            } else {
                int nBytes = m_nBPP;
                int nWidth = m_nWidth >> nShift;
                int nHeight = m_nHeight >> nShift;
                if (nWidth < 2) nWidth = 2;
                if (nHeight < 2) nHeight = 2;
                if (CAurInternal::m_nColorDepth == 16 && nBytes > 2)
                    nBytes = 2;
                if (nBytes == 3)
                    nBytes = 4;
                m_nMemoryUsage = nHeight * nWidth * nBytes;
                if (m_bMipMap)
                    m_nMemoryUsage = m_nMemoryUsage * 4 / 3;
            }
        } else {
            pStats->m_anMemory[TMS_TOTAL] = 0;
            pStats->m_anMemory[TMS_LIGHTMAP] = 0;
            pStats->m_anMemory[TMS_COMPRESSED] = 0;
            pStats->m_anMemory[TMS_CUBEMAP] = 0;
            pStats->m_anMemory[TMS_BUMPMAP] = 0;
            return;
        }
        m_bMemoryDirty = 0;
    }
    pStats->m_anMemory[TMS_TOTAL] = 0;
    pStats->m_anMemory[TMS_LIGHTMAP] = 0;
    pStats->m_anMemory[TMS_COMPRESSED] = 0;
    pStats->m_anMemory[TMS_CUBEMAP] = 0;
    pStats->m_anMemory[TMS_BUMPMAP] = 0;
    pStats->m_anMemory[TMS_TOTAL] = m_nMemoryUsage;
    int nUsage = pStats->m_anMemory[TMS_TOTAL];
    if (strstr(m_szResRef, "_lm") || strstr(m_szResRef, "_a00")) {
        pStats->m_anMemory[TMS_LIGHTMAP] = nUsage;
        return;
    }
    if (m_bCubeMap) {
        pStats->m_anMemory[TMS_CUBEMAP] = nUsage;
        return;
    }
    if (IsBumpMap()) {
        pStats->m_anMemory[TMS_BUMPMAP] = m_nMemoryUsage;
        return;
    }
    if (m_nCompressedSize >= 1)
        pStats->m_anMemory[TMS_COMPRESSED] = m_nMemoryUsage;
}

// The image loaders start as the renderer's file readers, until CAuroraInterface installs its own.
unsigned char* (*AurGetRaster)(char*, int*, int*, int*) = ImageRead;
int            (*AurGetRasterHeader)(char*, int*, int*, int*) = ImageReadHeader;
unsigned char* (*AurGetS3TCImage)(char*, int*, int*, int*, int*, float*) = ImageReadS3TC;
int            (*AurGetS3TCHeader)(char*, int*, int*, int*, int*, float*) = ImageReadS3TCHeader;
unsigned char* (*AurGet16bitImage)(char*, int*, int*, float*, int*) = ImageRead16bit;
int            (*AurGet16bitImageHeader)(char*, int*, int*, float*, int*) = ImageRead16bitHeader;
unsigned char* (*AurGetLayeredRaster)(char*, int*, int*, int*) = ImageReadPLT;
unsigned char* (*AurGetProcessedRaster)(char*, int*, int*, char*, int*, int*, int*,
                                        float*, char*) = ImageReadProcessed;
int            (*AurGetProcessedRasterHeader)(char*, int*, int*, char*, int*, int*,
                                              float*, char*) = ImageReadProcessedHeader;
void           (*AurReleaseLayeredRaster)(void*, int, int);

extern int nocompression;

int  AurTextureCompressionAvailable();
void ImageCopy(unsigned char* pDst, const unsigned char* pSrc, int nWidth, int nHeight, int nBpp);
void ImageFlipY(unsigned char* pData, int nWidth, int nHeight, int nBpp);
int  ImageReadS3TCHeader(char* pszName, int* pWidth, int* pHeight, int* pFormat, int* pUnknown,
                         float* pF);
int  ImageRead16bitHeader(char* pszName, int* pA, int* pB, float* pF, int* pC);

// The loader entry point tries the processed reader first, then S3TC when compression is both
// wanted and available, then the 16-bit reader, then the raw one.
__attribute__((noinline))
void* AurGetImageWrapper(char* pszFile, int* pnWidth, int* pnHeight, int* pnBPP,
                         int* pnCompressedSize, int* pnRasterSize, int* pnTxiSize,
                         Aurora::AuroraTextureFormats* pFormat, float* pfAlphaMean,
                         bool* pbProcessed, short* pnNumMipMaps, int nFlags)
{
    int nNoCompression = nocompression;

    *pnCompressedSize = 0;
    *pnRasterSize = 0;
    *pnTxiSize = 0;

    char nCode;
    char nMipMaps;
    unsigned char* pImage = AurGetProcessedRaster(pszFile, pnWidth, pnHeight, &nCode,
                                                  pnCompressedSize, pnRasterSize, pnTxiSize,
                                                  pfAlphaMean, &nMipMaps);
    if (pImage != 0)
    {
        *pnBPP = nCode;
        *pnNumMipMaps = nMipMaps;
        *pbProcessed = true;

        if (*pnCompressedSize != 0)
        {
#if KOTOR1  // K1 reports processed formats 5 and 6 as format 10
            if (static_cast<signed char>(nCode) == 5 || static_cast<signed char>(nCode) == 6)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(10);
            else if (*pnBPP == 4)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(8);
            else if (*pnBPP == 3)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(7);
#else
            if (*pnBPP == 3)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(7);
            else if (*pnBPP == 4)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(8);
#endif
            return pImage;
        }
    }
    else
    {
        if (nFlags != 0 && nNoCompression == 0 && AurTextureCompressionAvailable() != 0)
            pImage = AurGetS3TCImage(pszFile, pnWidth, pnHeight, pnBPP, pnCompressedSize,
                                     pfAlphaMean);

        if (pImage != 0)
        {
            if (*pnBPP == 3)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(7);
            else if (*pnBPP == 4)
                *pFormat = static_cast<Aurora::AuroraTextureFormats>(8);
            return pImage;
        }

        pImage = AurGet16bitImage(pszFile, pnWidth, pnHeight, pfAlphaMean, reinterpret_cast<int*>(pFormat));
        *pnBPP = 2;
        if (pImage == 0)
        {
            pImage = AurGetRaster(pszFile, pnWidth, pnHeight, pnBPP);
            if (pImage == 0)
                return 0;
        }
    }

    if (*pnBPP == 1)
        *pFormat = static_cast<Aurora::AuroraTextureFormats>(3);
    else if (*pnBPP == 3)
        *pFormat = static_cast<Aurora::AuroraTextureFormats>(1);
    else if (*pnBPP == 4)
        *pFormat = static_cast<Aurora::AuroraTextureFormats>(0);

    return pImage;
}

// The header-only twin of the loader fills in the dimensions, bpp and format without allocating
// anything.
int AurGetImageHeaderWrapper(char* pszFile, int* pnWidth, int* pnHeight, int* pnBPP,
                             int* pnUnknown, Aurora::AuroraTextureFormats* pFormat,
                             float* pfAlphaMean, int nFlags)
{
    *pnUnknown = 0;

    char nTrailing;   // reader scratch, unread here
    char nCode;       // format byte reported by the reader
    int  nScratch;    // reader scratch, unread here
    int nRaster = AurGetProcessedRasterHeader(pszFile, pnWidth, pnHeight, &nCode, pnUnknown,
                                              &nScratch, pfAlphaMean, &nTrailing);
    *pnBPP = nCode;

    if (nFlags != 0 && nRaster == 0 && AurTextureCompressionAvailable() != 0)
        nRaster = ImageReadS3TCHeader(pszFile, pnWidth, pnHeight, pnBPP, pnUnknown, pfAlphaMean);

    if (nRaster == 0)
    {
        *pnBPP = 2;
        nRaster = ImageRead16bitHeader(pszFile, pnWidth, pnHeight, pfAlphaMean, reinterpret_cast<int*>(pFormat));
        if (nRaster == 0)
            nRaster = AurGetRasterHeader(pszFile, pnWidth, pnHeight, pnBPP);
    }

    return nRaster;
}

unsigned char* AurGetLayeredRasterWrapper(char* pszFile, int* pnWidth, int* pnHeight,
                                          int* pnNumLayers)
{
    int nLayers;
    unsigned char* pRaster = AurGetLayeredRaster(pszFile, pnWidth, pnHeight, &nLayers);
    *pnNumLayers = 2;
    return pRaster;
}

// The release partner of AurGetImageWrapper, using the layered releaser, the plain one, or a plain
// delete[] for an image the loader did not mark processed.
void AurTextureFreeImage(unsigned char* pImage, int nWidth, int nHeight, int nBPP,
                         int nCompressedSize, int nRasterSize, int nTxiSize,
                         bool* pbProcessed, bool bCompressed, bool bLayered)
{
    if (*pbProcessed)
    {
        if (bLayered)
            AurReleaseLayeredRaster(pImage, nWidth, nHeight);
        else
            AurReleaseRaster(pImage, nWidth, nHeight, nBPP, nCompressedSize, nRasterSize, nTxiSize,
                             bCompressed);
        *pbProcessed = false;
    }
    else
    {
        delete[] pImage;
    }
}
