#pragma once

#include "CExoLinkedList.h"
#include "exostring.h"
#include "PortDevMode.h"

struct Vector;
class CResRef;
template <class T> class CExoArrayList;
class CAuroraModel;
class CAuroraModelX;
class CAuroraTexture;
class CAurora16BitTexture;
class CAuroraFontTexture;
class CAuroraLayeredTexture;
class CAuroraCompressedTexture;
class CAuroraProcessedTexture;
class CAuroraTXI;
class CAuroraVisibility;
class CExoString;

// Static facade over the renderer's resource, texture-pack and video-mode entry points.

class CAuroraInterface {
public:
    CAuroraInterface();
    ~CAuroraInterface();
    static int  IsResident(char* s);
    static void RequestModel(char* s);
    static int GetExactVideoMode(unsigned int a, unsigned int b, unsigned char c,
                                    unsigned int d);
    static int GetNearestVideoMode(unsigned int a, unsigned int b, unsigned char c,
                                    unsigned int d);
    static int IsSupportedResolution(unsigned int a, unsigned int b);
    static int IsValidResolution(unsigned int a, unsigned int b);
    static int IsWithinDesktopResolution(unsigned int a, unsigned int b);

    static CExoString m_pCurrentTexturePack[4];

    static CExoString GetCurrentTexturePack(unsigned char nPack);
    static int        LoadTexturePack(unsigned char nPack, const CExoString& sPack);
    static int        UnloadTexturePack(unsigned char nPack);
    static unsigned char* GetFontRaster(char*, void**, int*, int*, int*);
    static void       ReleaseFontRaster(void* pData);

    // Matches a filename extension against the requested formats and writes the 4-character
    // resource tag of the best one available.
    static void GetHighestPriorityTextureFormatTag(char* pOut, const char* pType, int* pnSize,
                                                   ...);
    // Best format the resource manager can deliver for resRef among the four requested.
    // Returns a 16-bit resource type, -1 when none is available.
    static unsigned short GetHighestPriorityTextureFormat(CResRef resRef, int bProcessed,
                                                          int bCompressed, int b16Bit,
                                                          int bTGA);
    static int  GetVideoModes(CExoArrayList<CExoString>& lst, CExoArrayList<unsigned int>& lst2,
                             unsigned char n);
    static void GetDesktopResolution(int& nWidth, int& nHeight);
    static int  SetTexturePack(unsigned char nPack, const CExoString& sPack);

    // Renderer entry points, installed into the matching Aur* function-pointer globals.
    static unsigned char* GetResource(char*, char*, int*);
    static void           ReleaseResource(void*, int);
    static unsigned char* GetRaster(char*, int*, int*, int*);
    static int            ReadRasterHeader(char*, int*, int*, int*);
    static void           ReleaseRaster(void*, int, int, int, int, int, int, bool);
    static unsigned char* GetLayeredRaster(char*, int*, int*, int*);
    static void           ReleaseLayeredRaster(void*, int, int);
    static unsigned char* GetCompressedRaster(char*, int*, int*, int*, int*, float*);
    static int            ReadCompressedRasterHeader(char*, int*, int*, int*, int*, float*);
    static unsigned char* Get16BitRaster(char*, int*, int*, float*, int*);
    static int            Get16BitRasterHeader(char*, int*, int*, float*, int*);
    static unsigned char* GetProcessedRaster(char*, int*, int*, char*, int*, int*, int*,
                                             float*, char*);
    static int            ReadProcessedRasterHeader(char*, int*, int*, char*, int*, int*,
                                                    float*, char*);

    // Shared "look up or create, validate, load" resource helpers.
    static CAuroraTexture*          GetTextureInternal(const CExoString& sName);
    static CAurora16BitTexture*     Get16BitTextureInternal(const CExoString& sName);
    static CAuroraFontTexture*      GetFontTextureInternal(const CExoString& sName);
    static CAuroraLayeredTexture*   GetLayeredTextureInternal(const CExoString& sName);
    static CAuroraCompressedTexture* GetCompressedTextureInternal(const CExoString& sName);
    static CAuroraProcessedTexture* GetProcessedTextureInternal(const CExoString& sName);
    static CAuroraTXI*              GetTXIInternal(const CExoString& sName);
    static CAuroraVisibility*       GetVisibilityInternal(const CExoString& sName);
    static CAuroraModelX*           GetModelXInternal(const CExoString& sName);
    static CAuroraModel*            GetModelInternal(const CExoString& sName, int bNoLoad,
                                                     int* pnCreated,
                                                     CExoLinkedListNode** ppNode);
    static CAuroraModel*            GetModelResHelper(const CExoString& sName, int bNoLoad,
                                                      int* pnCreated,
                                                      CExoLinkedListNode** ppNode);

    // Disown helpers; each validates that the res-helper-info block is this wrapper's own.
    static void ReleaseTextureInternal(CAuroraTexture* pTexture);
    static void Release16BitTextureInternal(CAurora16BitTexture* pTexture);
    static void ReleaseFontTextureInternal(CAuroraFontTexture* pTexture);
    static void ReleaseLayeredTextureInternal(CAuroraLayeredTexture* pTexture);
    static void ReleaseCompressedTextureInternal(CAuroraCompressedTexture* pTexture);
    static void ReleaseProcessedTextureInternal(CAuroraProcessedTexture* pTexture);
    static void ReleaseTXIInternal(CAuroraTXI* pTXI);
    static void ReleaseVisibilityInternal(CAuroraVisibility* pVisibility);
    static void ReleaseModelInternal(CAuroraModel* pModel);
    static void ReleaseModelXInternal(CAuroraModelX* pModel);

    // Models requested and not yet released; the destructor unloads and deletes each entry.
    static CExoLinkedList<CAuroraModel> m_lstRequestedModel;
};

int AurTextureSetDim(int, int);
void AurTextureResetAll();
void AurTextureSetDynamicMemoryRatio(float);
// Returns non-zero on success.
int  AurTextureSetMem(int);
void AurDisableSaturation();
void AurDisableScanNoise();
void AurDisableClairvoyance();
void AurDisableForceSight();
void AurEnableForceFury(short n);
void AurEnableScanNoise();
void AurEnableSaturation();
void AurSetSaturationParams(Vector v, float f);
void AurEnableSpeedBlur();
void AurDisableSpeedBlur();
bool AurIsSpeedBlurEnabled();
void AurSetSpeedBlurRatio(float ratio);
void AurEnableClairvoyance(bool b);
void AurEnableForceSight();
