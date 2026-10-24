// The renderer facade. Its constructor installs this class's static renderer entry points into the
// `Aur*` function-pointer globals the resource loaders call, so the class itself holds no data.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "aurorainterface.h"

#include "aurorainterface/aurmodel.h"
#include "aurtexture.h"
#include "aurmodelx.h"
#include "aur16bittexture.h"
#include "aurlayeredtexture.h"
#include "aurcompressedtexture.h"
#include "aurprocessedtexture.h"
#include "aurtxi.h"
#include "aurvisibility.h"
#include "CExoArrayListImpl.h"
#include "aurfonttexture.h"

// A texture wrapper keeps its res-helper-info block either before its pixel payload or, for a
// layered texture and a processed image, a fixed gap past the end of it.
enum {
    LAYERED_BYTES_PER_PIXEL = 2,
    RES_TAG_CHARS = 4,
    COMPRESSED_RESHELPERINFO_SIZE = 0x1e,
    BIT16_RESHELPERINFO_SIZE = 0x1a
};

#include "port/libcstring.h"
#include "port/crtapi.h"
#include "restype.h"

// A font raster's layout: a 24-byte header, then one 16-byte record per glyph.
enum {
    FNT_HEADER_SIZE = 24,
    FNT_GLYPH_SIZE = 16,
};

unsigned char* (*AurGetResource)(char*, char*, int*);
void           (*AurReleaseResource)(void*, int);
// The renderer's image-loader hooks (aurtexman.cpp), pointed at this interface's loaders.
extern unsigned char* (*AurGetRaster)(char*, int*, int*, int*);
extern int            (*AurGetRasterHeader)(char*, int*, int*, int*);
// The renderer passes the processed flag as a plain integer.
extern void           (*AurReleaseRaster)(void*, int, int, int, int, int, int, int);
extern unsigned char* (*AurGetLayeredRaster)(char*, int*, int*, int*);
extern void           (*AurReleaseLayeredRaster)(void*, int, int);
extern unsigned char* (*AurGetS3TCImage)(char*, int*, int*, int*, int*, float*);
extern int            (*AurGetS3TCHeader)(char*, int*, int*, int*, int*, float*);
extern unsigned char* (*AurGet16bitImage)(char*, int*, int*, float*, int*);
extern int            (*AurGet16bitImageHeader)(char*, int*, int*, float*, int*);
extern unsigned char* (*AurGetProcessedRaster)(char*, int*, int*, char*, int*, int*, int*, float*, char*);
extern int            (*AurGetProcessedRasterHeader)(char*, int*, int*, char*, int*, int*, float*, char*);

#ifdef K2_FIX_ENGINE_BUGS
// AurReleaseRaster's "processed" argument is an int (the texture's flag byte); ReleaseRaster takes a bool.
static void ReleaseRasterThroughInt(void* pImage, int nWidth, int nHeight, int nBPP, int nCompressedSize,
                                    int nRasterSize, int nTxiSize, int nCompressed)
{
    CAuroraInterface::ReleaseRaster(pImage, nWidth, nHeight, nBPP, nCompressedSize, nRasterSize, nTxiSize,
                                    nCompressed != 0);
}
#endif

CAuroraInterface::CAuroraInterface()
{
    AurGetResource = GetResource;
    AurReleaseResource = ReleaseResource;
    AurGetRaster = GetRaster;
    AurGetRasterHeader = ReadRasterHeader;
#ifdef K2_FIX_ENGINE_BUGS
    AurReleaseRaster = ReleaseRasterThroughInt;
#else
    AurReleaseRaster = reinterpret_cast<void (*)(void*, int, int, int, int, int, int, int)>(ReleaseRaster);
#endif
    AurGetLayeredRaster = GetLayeredRaster;
    AurReleaseLayeredRaster = ReleaseLayeredRaster;
    AurGetS3TCImage = GetCompressedRaster;
    AurGetS3TCHeader = ReadCompressedRasterHeader;
    AurGet16bitImage = Get16BitRaster;
    AurGet16bitImageHeader = Get16BitRasterHeader;
    AurGetProcessedRaster = GetProcessedRaster;
    AurGetProcessedRasterHeader = ReadProcessedRasterHeader;
    AurTextureSetDim(0, 0);
    AurTextureSetMem(16 * 1024 * 1024);
}

CAuroraInterface::~CAuroraInterface()
{
    while (m_lstRequestedModel.m_pInternal->m_pHead)
    {
        CAuroraModel* pModel = static_cast<CAuroraModel*>(m_lstRequestedModel.m_pInternal->RemoveHead());
        if (pModel)
        {
            pModel->Unload();
            delete pModel;
        }
    }
    AurClearPalette();
}

unsigned short CAuroraInterface::GetHighestPriorityTextureFormat(CResRef resRef, int bProcessed,
                                                                 int bCompressed, int b16Bit,
                                                                 int bTGA)
{
    // Exists() reports the key table that matched, not a size: 1 base-game key file, 2 override
    // directory, 3 encapsulated archive (ERF/MOD), 4 module RIM. In priority order.
    unsigned int nProcessedSource = 0;
    unsigned int nCompressedSource = 0;
    unsigned int n16BitSource = 0;
    unsigned int nTGASource = 0;

    if (bProcessed && !g_pExoResMan->Exists(resRef, 3007, &nProcessedSource))
        nProcessedSource = 0;
    if (bCompressed && !g_pExoResMan->Exists(resRef, 2033, &nCompressedSource))
        nCompressedSource = 0;
    if (b16Bit && !g_pExoResMan->Exists(resRef, 2059, &n16BitSource))
        n16BitSource = 0;
    if (bTGA && !g_pExoResMan->Exists(resRef, 3, &nTGASource))
        nTGASource = 0;
#if defined(K2_UI_PC) || defined(K1_UI_XBOX)
    // An override TGA (source 2) outranks the PC original inside the texture pack (source 3), and
    // the PC UI layouts want the PC art, so the touch redraws fall through to the archive.
    if (nTGASource == 2 && bProcessed && nProcessedSource != 0 && nProcessedSource != 2) {
#if KOTOR1
        // K1: every override TGA with a PC original is a touch redraw except the bitmap fonts,
        // which are the same glyph sheets at a higher resolution.
        char szName[17];
        const char* pName = resRef.GetResRef();
        for (int i = 0; i < 16; i++)
            szName[i] = (pName[i] >= 'A' && pName[i] <= 'Z') ? static_cast<char>(pName[i] + 32) : pName[i];
        szName[16] = 0;
        if (__builtin_strstr(szName, "font") == 0 && __builtin_strstr(szName, "fnt") == 0)
            nTGASource = 0;
#else
        static const char* const s_aPcArt[] = {
            "i_pause", "i_pause1", "i_pause2", "i_pause3",          // pause key
            "lbl_miarr_1", "lbl_miarr_2",                           // action-box arrows
            "uibit_brdr_map_p",                                     // minimap frame
            "pnl_pause_pc", "pnl_pazaakgam_pc", "pnl_pazaakset_pc", // panel backdrops
        };
        for (unsigned int i = 0; i < sizeof(s_aPcArt) / sizeof(s_aPcArt[0]); i++)
            if (resRef == const_cast<char*>(s_aPcArt[i]))
                nTGASource = 0;
#endif
    }
#endif

    // Best source first, and within one source the formats in priority order.
    if (bProcessed && nProcessedSource == 2)
        return 3007;
    if (bCompressed && nCompressedSource == 2)
        return 2033;
    if (b16Bit && n16BitSource == 2)
        return 2059;
    if (bTGA && nTGASource == 2)
        return 3;

    if (bProcessed && nProcessedSource == 4)
        return 3007;
    if (bCompressed && nCompressedSource == 4)
        return 2033;
    if (b16Bit && n16BitSource == 4)
        return 2059;
    if (bTGA && nTGASource == 4)
        return 3;

    if (bProcessed && nProcessedSource == 3)
        return 3007;
    if (bCompressed && nCompressedSource == 3)
        return 2033;
    if (b16Bit && n16BitSource == 3)
        return 2059;
    if (bTGA && nTGASource == 3)
        return 3;

    if (bProcessed && nProcessedSource == 1)
        return 3007;
    if (bCompressed && nCompressedSource == 1)
        return 2033;
    if (b16Bit && n16BitSource == 1)
        return 2059;
    if (bTGA && nTGASource == 1)
        return 3;

    return static_cast<unsigned short>(-1);
}

// Formats the resource tag the entry points dispatch on: a three-character extension plus its
// terminator, written into a 256-byte buffer.
static void AurMakeResTag(char* pOut, const char* pType, int* pnSize, ...)
{
    K2_UNUSED(pType);
    __builtin_va_list ap;
    __builtin_va_start(ap, pnSize);
    __builtin___vsprintf_chk(pOut, 0, 256, "%s", ap);
    __builtin_va_end(ap);
}

unsigned char* CAuroraInterface::GetResource(char* sName, char* sType, int* pnSize)
{
    CExoString sLower;

    if (sType == 0 || pnSize == 0)
        return 0;

    char szTag[256];
    // A type string may carry a leading '.'; the tag is what follows it.
    AurMakeResTag(szTag, sType, pnSize, sType[0] == '.' ? sType + 1 : sType);
    _strupr(szTag);

    sLower = sName;
    sLower = sLower.LowerCase();

    // The tag is compared as one word: three uppercase characters plus the terminator.
    if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('M' | ('D' << 8) | ('L' << 16)))
    {
        CAuroraModel* pModel = GetModelInternal(sLower, 0, 0, 0);
        if (pModel != 0)
        {
            *pnSize = pModel->GetModelDataSize();
            return static_cast<unsigned char*>(pModel->GetModelDataPtr());
        }
    }
    else if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('M' | ('D' << 8) | ('X' << 16)))
    {
        CAuroraModelX* pModelX = GetModelXInternal(sLower);
        if (pModelX != 0)
        {
            *pnSize = pModelX->GetModelDataSize();
            return static_cast<unsigned char*>(pModelX->GetModelDataPtr());
        }
    }
    else if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('T' | ('X' << 8) | ('I' << 16)))
    {
        CAuroraTXI* pTXI = GetTXIInternal(sLower);
        if (pTXI != 0)
        {
            *pnSize = pTXI->GetTXIDataSize();
            return static_cast<unsigned char*>(pTXI->GetTXIDataPtr());
        }
        // A TXI that is not a resource of its own lives inside a TPC, and the processed-texture
        // wrapper answers for it with a bitwise-negated size: the sign marks it as embedded.
        CAuroraProcessedTexture* pProcessed = GetProcessedTextureInternal(sLower);
        if (pProcessed != 0)
        {
            *pnSize = ~pProcessed->GetProcessedTextureTXISize();
            return static_cast<unsigned char*>(pProcessed->GetProcessedTextureTXIPtr());
        }
    }
    else if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('T' | ('G' << 8) | ('A' << 16)))
    {
        CAuroraTexture* pTexture = GetTextureInternal(sLower);
        if (pTexture != 0)
        {
            *pnSize = pTexture->GetTextureDataSize();
            return static_cast<unsigned char*>(pTexture->GetTextureDataPtr());
        }
    }
    else if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('T' | ('P' << 8) | ('C' << 16)))
    {
        CAuroraProcessedTexture* pProcessed = GetProcessedTextureInternal(sLower);
        if (pProcessed != 0)
        {
            *pnSize = pProcessed->GetProcessedTextureDataSize();
            return static_cast<unsigned char*>(pProcessed->GetProcessedTextureDataPtr());
        }
    }
    else if (*reinterpret_cast<volatile unsigned int*>(szTag) == ('V' | ('I' << 8) | ('S' << 16)))
    {
        CAuroraVisibility* pVisibility = GetVisibilityInternal(sLower);
        if (pVisibility != 0)
        {
            *pnSize = pVisibility->GetVisibilityDataSize();
            return static_cast<unsigned char*>(pVisibility->GetVisibilityDataPtr());
        }
    }
    return 0;
}

void CAuroraInterface::ReleaseResource(void* pData, int nExtra)
{
    // The tag lives in the res-helper-info block, which sits in the allocation's slack before the
    // data pointer. A negative `nExtra` selects the other layout, computed from the tail.
    if (nExtra >= 0)
    {
        CResHelperInfo* pInfo = static_cast<CResHelperInfo*>(pData) - 1;
        switch (pInfo->m_nResType)
        {
        case RESTYPE_MDL:
            ReleaseModelInternal(static_cast<CAuroraModel*>(pInfo->m_pResHelper));
            break;
        case RESTYPE_TXI:
            ReleaseTXIInternal(static_cast<CAuroraTXI*>(pInfo->m_pResHelper));
            break;
        case RESTYPE_VIS:
            ReleaseVisibilityInternal(static_cast<CAuroraVisibility*>(pInfo->m_pResHelper));
            break;
        case RESTYPE_MDX:
            ReleaseModelXInternal(static_cast<CAuroraModelX*>(pInfo->m_pResHelper));
            break;
        default:
            break;
        }
        return;
    }

    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pData) + ~nExtra + RESHELPERINFO_TRAILING_GAP);
    // The embedded-TXI tag carries its low bit as a flag.
    if ((pInfo->m_nResType & 0xfffe) == RESTYPE_TXB)
        ReleaseProcessedTextureInternal(static_cast<CAuroraProcessedTexture*>(pInfo->m_pResHelper));
}

unsigned char* CAuroraInterface::GetRaster(char* sName, int* pnWidth, int* pnHeight, int* pnDepth)
{
    unsigned int nWidth, nHeight;
    unsigned char nBitsPerPixel;

    CAuroraTexture* pTexture = GetTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    pTexture->GetTextureAttrib(&nWidth, &nHeight, &nBitsPerPixel);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pnDepth = nBitsPerPixel / 8;
    return static_cast<unsigned char*>(pTexture->GetTextureDataPtr());
}

int CAuroraInterface::ReadRasterHeader(char* sName, int* pnWidth, int* pnHeight, int* pnDepth)
{
    // A resref is at most 16 characters; an overlong name is a failure, not a truncation.
    if (strlen(sName) > 16)
        return 0;

    CAuroraTexture texture(CResRef(sName), 0);
    return texture.ReadTextureHeader(pnWidth, pnHeight, pnDepth);
}

// The parameter list is the loader's own. `bCompressed` is the loader's "processed" flag: a
// processed image keeps its block after the payload, every other kind before it.
void CAuroraInterface::ReleaseRaster(void* pImage, int nWidth, int nHeight, int nBPP,
                                    int nCompressedSize, int nRasterSize, int nTxiSize,
                                    bool bCompressed)
{
    if (bCompressed)
    {
        char* pBlock = static_cast<char*>(pImage) +
                       (nCompressedSize != 0 ? nCompressedSize : nRasterSize) + nTxiSize;
        CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(pBlock + RESHELPERINFO_TRAILING_GAP);
        if (pInfo->m_nResType == RESTYPE_TPC)
            ReleaseProcessedTextureInternal(static_cast<CAuroraProcessedTexture*>(pInfo->m_pResHelper));
        return;
    }

    if (nCompressedSize != 0)
    {
        CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pImage) - COMPRESSED_RESHELPERINFO_SIZE);
        if (pInfo->m_nResType == RESTYPE_DDS)
            ReleaseCompressedTextureInternal(static_cast<CAuroraCompressedTexture*>(pInfo->m_pResHelper));
        return;
    }

    if (nBPP == 2)
    {
        CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pImage) - BIT16_RESHELPERINFO_SIZE);
        if (pInfo->m_nResType == RESTYPE_4PC)
            Release16BitTextureInternal(static_cast<CAurora16BitTexture*>(pInfo->m_pResHelper));
        return;
    }

    CResHelperInfo* pInfo =
        reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pImage) + nWidth * nHeight * nBPP + RESHELPERINFO_TRAILING_GAP);
    if (pInfo->m_nResType == RESTYPE_TGA)
        ReleaseTextureInternal(static_cast<CAuroraTexture*>(pInfo->m_pResHelper));
}

unsigned char* CAuroraInterface::GetLayeredRaster(char* sName, int* pnWidth, int* pnHeight,
                                                 int* pnNumLayers)
{
    unsigned int nWidth, nHeight, nNumLayers;

    CAuroraLayeredTexture* pTexture = GetLayeredTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    pTexture->GetTextureAttrib(&nWidth, &nHeight, &nNumLayers);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pnNumLayers = nNumLayers;
    return static_cast<unsigned char*>(pTexture->GetTextureDataPtr());
}

void CAuroraInterface::ReleaseLayeredRaster(void* pData, int nWidth, int nHeight)
{
    // A layered texture keeps its block past the payload: two bytes per pixel plus a four-byte gap.
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pData) + nWidth * nHeight * LAYERED_BYTES_PER_PIXEL +
                                              RESHELPERINFO_TRAILING_GAP);
    ReleaseLayeredTextureInternal(static_cast<CAuroraLayeredTexture*>(pInfo->m_pResHelper));
}

unsigned char* CAuroraInterface::GetCompressedRaster(char* sName, int* pnWidth, int* pnHeight,
                                                    int* pnFormat, int* pnSize, float* pfAlphaMean)
{
    float fAlphaMean;
    unsigned int nWidth, nHeight, nSize;
    unsigned char nFormat;

    // The format probe runs on the resref alone: a device that cannot take a DDS never builds the
    // wrapper at all.
    CResRef resRef(sName);
    if (GetHighestPriorityTextureFormat(resRef, 0, 1, 1, 1) != RESTYPE_DDS)
        return 0;

    CAuroraCompressedTexture* pTexture = GetCompressedTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    pTexture->GetCompressedTextureAttrib(&nWidth, &nHeight, &nFormat, &nSize, &fAlphaMean);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pnFormat = nFormat;
    *pnSize = nSize;
    *pfAlphaMean = fAlphaMean;
    return static_cast<unsigned char*>(pTexture->GetCompressedTextureDataPtr());
}

int CAuroraInterface::ReadCompressedRasterHeader(char* sName, int* pnWidth, int* pnHeight,
                                                int* pnFormat, int* pnSize, float* pfAlphaMean)
{
    if (strlen(sName) > 16)
        return 0;

    CAuroraCompressedTexture texture(CResRef(sName), 0);
    return texture.ReadTextureHeader(pnWidth, pnHeight, pnFormat, pnSize, pfAlphaMean);
}

unsigned char* CAuroraInterface::Get16BitRaster(char* sName, int* pnWidth, int* pnHeight,
                                               float* pfAlphaMean, int* pnFormat)
{
    float fAlphaMean;
    int nFormat;
    unsigned int nWidth, nHeight;

    CResRef resRef(sName);
    if (GetHighestPriorityTextureFormat(resRef, 0, 0, 1, 1) != RESTYPE_4PC)
        return 0;

    CAurora16BitTexture* pTexture = Get16BitTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    pTexture->GetTextureAttrib(&nWidth, &nHeight, &fAlphaMean, &nFormat);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pfAlphaMean = fAlphaMean;
    *pnFormat = nFormat;
    return static_cast<unsigned char*>(pTexture->GetTextureDataPtr());
}

int CAuroraInterface::Get16BitRasterHeader(char* sName, int* pnWidth, int* pnHeight,
                                          float* pfAlphaMean, int* pnFormat)
{
    if (strlen(sName) > 16)
        return 0;

    CAurora16BitTexture texture(CResRef(sName), 0);
    return texture.ReadTextureHeader(pnWidth, pnHeight, pfAlphaMean, pnFormat);
}

unsigned char* CAuroraInterface::GetProcessedRaster(char* sName, int* pnWidth, int* pnHeight,
                                                   char* pnFormat, int* pnCompressedSize, int* pnUncompressedSize,
                                                   int* pnTXISize, float* pfAlphaMean,
                                                   char* pnTrailing)
{
    CResRef resRef(sName);
    if (GetHighestPriorityTextureFormat(resRef, 1, 1, 1, 1) != RESTYPE_TPC)
        return 0;

    CAuroraProcessedTexture* pTexture = GetProcessedTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    pTexture->GetProcessedTextureAttrib(pnWidth, pnHeight, pnFormat, pnCompressedSize, pnUncompressedSize, pnTXISize,
                                        pfAlphaMean, pnTrailing);
    return static_cast<unsigned char*>(pTexture->GetProcessedTextureDataPtr());
}

int CAuroraInterface::ReadProcessedRasterHeader(char* sName, int* pnWidth, int* pnHeight,
                                               char* pnFormat, int* pnCompressedSize, int* pnUncompressedSize,
                                               float* pfAlphaMean, char* pnTrailing)
{
    if (strlen(sName) > 16)
        return 0;

    CAuroraProcessedTexture texture(CResRef(sName), 0);
    return texture.ReadProcessedTextureHeader(pnWidth, pnHeight, pnFormat, pnCompressedSize, pnUncompressedSize,
                                              pfAlphaMean, pnTrailing);
}

// The rest of the facade follows: the font raster pair, the texture-pack slots, the resolution
// predicates, IsResident and the video-mode wrappers.

class Model;
Model* BinaryFindModel(char* sName);   // aurmodelparse.cpp

#include "port/crtapi.h"

unsigned char* CAuroraInterface::GetFontRaster(char* sName, void** ppData, int* pnWidth,
                                               int* pnHeight, int* pnBytesPerPixel)
{
    unsigned int nWidth, nHeight;
    unsigned char nBitsPerPixel;

    CAuroraFontTexture* pTexture = GetFontTextureInternal(CExoString(sName));
    if (pTexture == 0)
        return 0;

    *ppData = pTexture->GetFontDataPtr();
    pTexture->GetFontTextureAttrib(&nWidth, &nHeight, &nBitsPerPixel);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    *pnBytesPerPixel = nBitsPerPixel / 8;
    return static_cast<unsigned char*>(pTexture->GetFontTextureDataPtr());
}

void CAuroraInterface::ReleaseFontRaster(void* pData)
{
    // The res-helper-info block trails the glyph table: a glyph count followed by one 16-byte
    // record per glyph.
    unsigned int nGlyphBlockOffset = *(static_cast<unsigned int*>(pData)) * FNT_GLYPH_SIZE + FNT_HEADER_SIZE;
    CResHelperInfo* pInfo = reinterpret_cast<CResHelperInfo*>(static_cast<char*>(pData) + nGlyphBlockOffset);
    if (pInfo->m_nResType == RESTYPE_FNT)
        ReleaseFontTextureInternal(static_cast<CAuroraFontTexture*>(pInfo->m_pResHelper));
}

CExoString CAuroraInterface::GetCurrentTexturePack(unsigned char nPack)
{
    if (nPack > 3)
        return CExoString(0);
    return m_pCurrentTexturePack[nPack];
}

int CAuroraInterface::LoadTexturePack(unsigned char nPack, const CExoString& sPack)
{
    if (nPack > 3)
        return 0;

#if KOTOR1  // K1's CExoString::GetLength is out of line
    if (sPack.GetLength() == 0)
#else
    if (sPack.m_sString == 0 || static_cast<int>(strlen(sPack.m_sString)) == 0)
#endif
        return 0;

    if (!g_pExoResMan->AddEncapsulatedResourceFile(sPack, 2))
        return 0;

    m_pCurrentTexturePack[nPack] = sPack;
    return 1;
}

int CAuroraInterface::UnloadTexturePack(unsigned char nPack)
{
    CExoString sPack;

    if (nPack > 3)
        return 0;

    sPack = m_pCurrentTexturePack[nPack];
#if KOTOR1  // K1's CExoString::GetLength is out of line
    if (sPack.GetLength() != 0)
#else
    if (sPack.m_sString != 0 && static_cast<int>(strlen(sPack.m_sString)) != 0)
#endif
    {
        if (!g_pExoResMan->RemoveEncapsulatedResourceFile(sPack))
            return 0;
        m_pCurrentTexturePack[nPack] = "";
    }
    return 1;
}

int CAuroraInterface::SetTexturePack(unsigned char nPack, const CExoString& sPack)
{
    if (nPack > 3)
        return 0;

    // The right operand is a temporary copy, which is destroyed as soon as the comparison ends.
    if (sPack == CExoString(m_pCurrentTexturePack[nPack]))
        return 1;

    if (!UnloadTexturePack(nPack))
        return 0;

#if KOTOR1  // K1's CExoString::GetLength is out of line
    if (sPack.GetLength() == 0)
#else
    if (sPack.m_sString == 0 || static_cast<int>(strlen(sPack.m_sString)) == 0)
#endif
        return 0;

    if (!g_pExoResMan->AddEncapsulatedResourceFile(sPack, 2))
        return 0;

    m_pCurrentTexturePack[nPack] = sPack;
    return 1;
}
void CAuroraInterface::RequestModel(char* sName)
{
    CExoLinkedListNode* pNode;
    int bCreated = 0;
    // bNoLoad: the model is queued, not read.
    CAuroraModel* pModel = GetModelInternal(CExoString(sName), 1, &bCreated, &pNode);

    // Only a newly created entry belongs in the queue; an existing one is already ordered.
    if (pModel != 0 && bCreated != 0)
    {
        if (pNode != 0)
            m_lstRequestedModel.m_pInternal->AddBefore(pModel, pNode);
        else
            m_lstRequestedModel.m_pInternal->AddTail(pModel);
    }
}

// Both predicates below are constant-true in this build: the Android renderer accepts whatever the
// OS reports. The video-mode wrappers use them as the final filter.
int CAuroraInterface::IsSupportedResolution(unsigned int nWidth, unsigned int nHeight)
{
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);
    return 1;
}

int CAuroraInterface::IsValidResolution(unsigned int nWidth, unsigned int nHeight)
{
#if !KOTOR1
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);
#endif
#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
    // desktop K1: the modes are the host's window sizes (k1/k1port.cpp), all of them usable
    K2_UNUSED(nWidth);
    K2_UNUSED(nHeight);
    return 1;
#elif KOTOR1  // K1 accepts only the five PC modes
    if (nWidth == 800 && nHeight == 600)
        return 1;
    if (nWidth == 1024 && nHeight == 768)
        return 1;
    if (nWidth == 1280 && (nHeight == 960 || nHeight == 1024))
        return 1;
    return nWidth == 1600 && nHeight == 1200;
#else
    return 1;
#endif
}

int CAuroraInterface::IsResident(char* s)
{
    if (s == 0)
        return 0;

    if (_stricmp(s, "NULL") == 0)
        return 0;

    return BinaryFindModel(s) != 0;
}


// The desktop display mode cache. Both predicates below and the video-mode wrappers resolve the
// desktop through this one lazy pair.

static int g_nDesktopWidth = 0;
static int g_nDesktopHeight = 0;

#if defined(K2_QOL_FULLSCREEN) && !KOTOR1
// The desktop builds' desktop is the host's largest fullscreen size, its native resolution on a
// HiDPI display (platforms/shim/shim_window.cpp), which SDL's mode, in points, is smaller than.
extern "C" void k2host_desktop_size(int* pW, int* pH);
static int K2QolGetDesktopMode(SDL_DisplayMode* pMode)
{
    k2host_desktop_size(&pMode->w, &pMode->h);
    return 0;
}
#define K2_DESKTOP_MODE(pMode) K2QolGetDesktopMode(pMode)
#else
#define K2_DESKTOP_MODE(pMode) SDL_GetDesktopDisplayMode(0, pMode)
#endif

void CAuroraInterface::GetDesktopResolution(int& nWidth, int& nHeight)
{
    if (g_nDesktopWidth == 0)
    {
        SDL_DisplayMode mode;
        if (K2_DESKTOP_MODE(&mode) == 0)
        {
            g_nDesktopWidth = mode.w;
            g_nDesktopHeight = mode.h;
        }
    }
    nWidth = g_nDesktopWidth;
    nHeight = g_nDesktopHeight;
}

int CAuroraInterface::IsWithinDesktopResolution(unsigned int nWidth, unsigned int nHeight)
{
    if (g_nDesktopWidth == 0)
    {
        SDL_DisplayMode mode;
        if (K2_DESKTOP_MODE(&mode) == 0)
        {
            g_nDesktopWidth = mode.w;
            g_nDesktopHeight = mode.h;
        }
    }
    // Both comparisons are always evaluated, never short-circuited.
    return (static_cast<unsigned int>(g_nDesktopWidth) >= nWidth) & (static_cast<unsigned int>(g_nDesktopHeight) >= nHeight);
}

// The enumerated video modes. `nMode` is an index into the OS's mode list, or -1.
int CAuroraInterface::GetVideoModes(CExoArrayList<CExoString>& lst,
                                    CExoArrayList<unsigned int>& lst2, unsigned char n)
{
    CExoString sName;
    int nMode = 0;
    int nCount = 0;

    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(devMode);

    while (EnumDisplaySettingsA(0, nMode, &devMode) != 0)
    {
        // Everything below 800x600 is unusable, and a 2..59 Hz report is a bogus rate (a mode with
        // no rate reports 0 or 1 and is allowed through).
        if (devMode.m_pelsWidth >= 800 && devMode.m_pelsHeight >= 600 &&
            (devMode.m_displayFrequency < 2 || devMode.m_displayFrequency >= 60))
        {
            // A caller-supplied depth is an exact request; without one, anything at 16bpp or better.
            if (!(n != 0 ? devMode.m_bitsPerPel == n : devMode.m_bitsPerPel > 15))
                goto next;

            if (devMode.m_displayFrequency == 0)
                sName.Format(const_cast<char*>("%ix%i"), devMode.m_pelsWidth, devMode.m_pelsHeight);
            else
                sName.Format(const_cast<char*>("%ix%i %ihz"), devMode.m_pelsWidth, devMode.m_pelsHeight,
                             devMode.m_displayFrequency);

            lst.Add(sName);
            lst2.Add(static_cast<unsigned int>(nMode));
            nCount++;
        }

    next:
        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);
        nMode++;
    }
    return nCount;
}

int CAuroraInterface::GetNearestVideoMode(unsigned int nWidth, unsigned int nHeight,
                                          unsigned char nBPP, unsigned int nRefreshRate)
{
    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(devMode);

    if (EnumDisplaySettingsA(0, 0, &devMode) == 0)
        return -1;

    // The best refresh rate is pinned to 60 Hz whenever a mode is taken, so the refresh comparison
    // below always measures the request against 60.
    int nRefreshDelta = static_cast<int>(nRefreshRate) > 60 ? static_cast<int>(nRefreshRate) - 60 : 60 - static_cast<int>(nRefreshRate);

    int nBestMode = -1;
    unsigned int nBestWidth = 0;
    unsigned int nBestHeight = 0;
    unsigned int nBestBPP = 0;
    unsigned int nBestRefresh = 0;
    unsigned int nMode = 1;

    for (;;)
    {
        if (devMode.m_bitsPerPel >= 16)
        {
            // The tuple is replaced only when the candidate is strictly closer at the first field
            // where the two differ; an equal field falls through to the next one.
            int nBPPDeltaCur = __builtin_abs(static_cast<int>(devMode.m_bitsPerPel) - static_cast<int>(nBPP));
            int nBPPDeltaBest = __builtin_abs(static_cast<int>(nBestBPP) - static_cast<int>(nBPP));
            if (nBPPDeltaCur >= nBPPDeltaBest)
            {
                if (nBPPDeltaCur != nBPPDeltaBest)
                    goto next;

                int nWidthDeltaCur = __builtin_abs(static_cast<int>(devMode.m_pelsWidth) - static_cast<int>(nWidth));
                int nWidthDeltaBest = __builtin_abs(static_cast<int>(nBestWidth) - static_cast<int>(nWidth));
                if (nWidthDeltaCur >= nWidthDeltaBest)
                {
                    if (nWidthDeltaCur != nWidthDeltaBest)
                        goto next;

                    int nHeightDeltaCur = __builtin_abs(static_cast<int>(devMode.m_pelsHeight) - static_cast<int>(nHeight));
                    int nHeightDeltaBest = __builtin_abs(static_cast<int>(nBestHeight) - static_cast<int>(nHeight));
                    if (nHeightDeltaCur >= nHeightDeltaBest)
                    {
                        if (nHeightDeltaCur != nHeightDeltaBest)
                            goto next;

                        if (nRefreshDelta >= __builtin_abs(static_cast<int>(nBestRefresh) - static_cast<int>(nRefreshRate)))
                            goto next;
                    }
                }
            }

            nBestMode = nMode - 1;
            nBestWidth = devMode.m_pelsWidth;
            nBestHeight = devMode.m_pelsHeight;
            nBestBPP = devMode.m_bitsPerPel;
            nBestRefresh = 60;
        }

    next:
        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);
        if (EnumDisplaySettingsA(0, nMode++, &devMode) == 0)
            break;
    }
    return nBestMode;
}

int CAuroraInterface::GetExactVideoMode(unsigned int nWidth, unsigned int nHeight,
                                        unsigned char nBPP, unsigned int nRefreshRate)
{
    int nMode = 0;
    for (;;)
    {
        PortDevMode devMode = PortDevMode();
        devMode.m_size = sizeof(devMode);

        if (EnumDisplaySettingsA(0, nMode, &devMode) == 0)
            return -1;

        // The refresh rate is the one field with a wildcard: a mode reporting no rate at all
        // counts as a match.
        if (devMode.m_pelsWidth == nWidth && devMode.m_pelsHeight == nHeight &&
            devMode.m_bitsPerPel == nBPP &&
            (devMode.m_displayFrequency == nRefreshRate || devMode.m_displayFrequency < 2))
            return nMode;

        nMode++;
    }
}
