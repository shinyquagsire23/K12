// The renderer's free-function surface, with the `Aur*` GL switches every other module toggles, the
// debug-draw square family, the model dependency walker, and the console handlers those bring. The
// switches read and write engine globals, which are declared here and defined elsewhere.

#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library copy
#endif
#include "Vector.h"
#include "Plane.h"
#include "aurinternal.h"
#include "include/aurora.h"
#include "aurglcore.h"
#include "List.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "aurtexman.h"
#include "aurmaterial.h"
#include "auremitter.h"
#include "aurscene.h"
#include "aurscenemanager.h"
#include "aurora.h"
#include "aurgui.h"

int g_Letterbox_top;
int g_Letterbox_bottom;
bool g_bLetterBoxMode;

float memoryfree;

extern int enableshadows;
extern int enablesoftshadows;
extern int enabletexturecontrollers;
extern int enablegrass;
extern int enableemitters;
extern int doframebuffer;
int g_nCurrentAnisotropy;
extern float largest_supported_anisotropy;
extern int g_nAliasingMode;
extern int g_nOldAliasingMode;
int g_rgbaPauseOverlayColor = 2130706432;
float g_fGlobalAlpha = 1.0f;
extern bool g_gameWantsSpeedBlur;
extern bool g_allowForceSpeedEffect;
extern unsigned char g_ForceSight;
bool g_bDisableVertexBufferObjects;
bool g_bVertexBufferObjectsWriteOnly = true;
extern bool gHasRevComps_Mac;
extern int g_nUnlockATISoftShadowsForHotFix;
char versionstring[32];

float CAurGUIDraw::cm_fViewportAlpha[10];

#if KOTOR1
static char gszDrawableSquareHandle[10];
#else
static char gszDrawableSquareHandle[12];
#endif
bool disablepostprocessing;
#if KOTOR1
extern int preferPerfToVisual;
extern int g_IosAntiAlias;   // anti-alias setting
int AurAnisotropicFilterAvailable();
#endif

Model* FindModel(char* pszName);
Gob* FindGob(char* pszName);
void AurAddNodeDependencies(MdlNode& node, List<char*>& list);
void AurAddDependency(char* pszName, List<char*>& list);
// Returns a signed distance, not a bool, for comparison against 0.0f.
float SphereAbovePlane(Vector const& vSphere, float fRadius, Plane const& plane);
// Componentwise comparison, unlike `operator==`.
int equal(const Vector& a, const Vector& b);
Scene* FindScene(char* pszName);
// The point is passed by value, and the context argument is null at every call site here. The node
// it returns is used as a `CAurRoom*`: the room is the node in this engine's model.
BSPNode* BSPSearch(BSPNode* pNode, Vector vPoint, int (*pfnTest)(BSPNode*, void*), void* pData);

// The CRT and the platform shims.
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"
#include "port/crtapi.h"
extern "C" const char* gluErrorString(unsigned int nError);

// Advances the cursor it is given, hence `char**`.
char* GetToken(char** pszCursor);
#if KOTOR1
// K1's takes no source file and line.
uint64_t AurAddDrawableSquare(float* paFloats, uint64_t nHandle);
#else
uint64_t AurAddDrawableSquare(float* paFloats, uint64_t nHandle, const char* pszName, int nFlags);
#endif
#if !KOTOR1
// The fortified sprintf wrapper, file-local, taking (dst, length, fmt, ...).
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}
#endif
bool AurSwapControlAvailable();
// A tri-state, -1 until the driver has been probed, so callers normalise the result themselves.
int AurIsAAliasingAvailable();
void AuroraSetTileFoggedByWar(Vector& vTile, bool bFogged);

#if KOTOR1
#include <stdio.h>
#endif

char* AurGetVersionString()
{
#if KOTOR1
    sprintf(versionstring, "SWPC 0027");
    return versionstring;
#else
    // idiom-exempt: the LITERAL's length is the byte target — deleting the constant would make this a
    // 32-byte zero fill instead of a 7-byte two-store copy.
    __builtin_memcpy(versionstring, "OEI-K2", 7);  // idiom-exempt: fixed 7-byte version literal
    return versionstring;
#endif
}

// Debug draw squares.

// Builds a five-vertex triangle strip from the coordinate stream it is given: `paFloats` carries
// four corners of three floats each, and the strip repeats the first corner, which is why the shape
// reads as a bowtie. Every vertex is {x, y, z, handle}. The float payload is copied as raw 32-bit
// words, so the destination is viewed as `unsigned int*`.
#if KOTOR1
uint64_t AurAddDrawableSquare(float* paFloats, uint64_t nHandle)
{
    // K1 formats no debug line.
#else
uint64_t AurAddDrawableSquare(float* paFloats, uint64_t nHandle, const char* pszName, int nFlags)
{
    char szBuffer[256];
    sprintf_s(szBuffer, 256, "BOWTIE [%s line %d] (%.2f, %.2f, %.2f)\n",
              pszName, nFlags, static_cast<double>(paFloats[3]), static_cast<double>(paFloats[4]), static_cast<double>(paFloats[5]));
#endif

    unsigned int nPoolId;
    nPoolId = GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), 80, 0, 0x11, &nPoolId);

    unsigned int* pDst = static_cast<unsigned int*>(GLRender::LockPool(nPoolId, 0, 0, 1));
    unsigned int* pSrc = reinterpret_cast<unsigned int*>(paFloats);
    for (int i = 0; i < 4; i++) {
        pDst[i * 4 + 0] = pSrc[i * 3 + 0];
        pDst[i * 4 + 1] = pSrc[i * 3 + 1];
        pDst[i * 4 + 2] = pSrc[i * 3 + 2];
        pDst[i * 4 + 3] = static_cast<unsigned int>(nHandle);
    }
    pDst[16] = pSrc[0];
    pDst[17] = pSrc[1];
    pDst[18] = pSrc[2];
    pDst[19] = static_cast<unsigned int>(nHandle);

    GLRender::UnlockPool(nPoolId);
    GLRender::AddSquareToRender(nPoolId);
    return nPoolId;
}

// The sentinel is an all-ones 32-bit value, not a 64-bit -1.
void AurRemoveDrawableSquare(uint64_t nHandle)
{
    if (nHandle == 0xffffffffUL)
        return;
    GLRender::RemoveSquareToRender(static_cast<unsigned int>(nHandle));
}

void AurRemoveAllDrawableSquares()
{
    GLRender::RemoveAllDrawableSquares();
}

// The debug console's square spawner parses twelve coordinates straight into a local array, then
// the square is created and its handle formatted back into the shared handle buffer.
const char* addsquaretorender(char* pszArgs)
{
    char* pszCursor = pszArgs;

    float afFloats[12];
    afFloats[0] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[1] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[2] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[3] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[4] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[5] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[6] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[7] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[8] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[9] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[10] = static_cast<float>(atof(GetToken(&pszCursor)));
    afFloats[11] = static_cast<float>(atof(GetToken(&pszCursor)));

#if KOTOR1
    uint64_t nHandle = AurAddDrawableSquare(afFloats, static_cast<uint64_t>(-1));
#else
    uint64_t nHandle = AurAddDrawableSquare(afFloats, static_cast<uint64_t>(-1), __FILE__, 196);
#endif

#if KOTOR1
    sprintf(gszDrawableSquareHandle, "%u", static_cast<unsigned int>(nHandle));
#else
    sprintf_s(gszDrawableSquareHandle, 10, "%u", static_cast<unsigned int>(nHandle));
#endif
    return gszDrawableSquareHandle;
}

void AurInitExtensions()
{
    GLRender::InitExtensions();
}

// The `Aur*` feature switches each read or write a single engine latch.

bool AurQueryShadowsEnabled()
{
    return enableshadows != 0;
}

void AurEnableShadows()
{
    enableshadows = 1;
}

void AurDisableShadows()
{
    enableshadows = 0;
    enablesoftshadows = 0;
}

void AurDisableSoftShadows()
{
    enablesoftshadows = 0;
}

bool AurQueryTexControllersEnabled()
{
    return enabletexturecontrollers != 0;
}

void AurEnableTexControllers()
{
    enabletexturecontrollers = 1;
}

void AurDisableTexControllers()
{
    enabletexturecontrollers = 0;
}

bool AurQueryGrassEnabled()
{
    return enablegrass != 0;
}

void AurEnableGrass()
{
    enablegrass = 1;
}

void AurDisableGrass()
{
    enablegrass = 0;
}

// Bitwise and of the two flags, so both are read.
bool AurQuerySoftShadowsEnabled()
{
    return (enablesoftshadows != 0) & (enableshadows != 0);
}

void AurEnableSoftShadows()
{
    enablesoftshadows = 1;
    enableshadows = 1;
}

bool AurQueryFrameBufferEffectsEnabled()
{
    return doframebuffer != 0;
}

void AurEnableFrameBufferEffects()
{
    doframebuffer = 1;
#if KOTOR1  // K1: the effects switch is the perf-over-visual preference
    preferPerfToVisual = 0;
#endif
}

void AurDisableFrameBufferEffects()
{
#if KOTOR1  // K1: enabling the effects switch unsets the preference
    doframebuffer = 1;
    preferPerfToVisual = 1;
#elif defined(K2_QOL_FRAME_BUFFER_EFFECTS)
    doframebuffer = 0;   // Advanced Graphics' switch (FrameBufferModificationsEndIos reads it)
#endif
}

bool AurQueryEmittersEnabled()
{
    return enableemitters != 0;
}

void AurEnableEmitters()
{
    enableemitters = 1;
}

void AurDisableEmitters()
{
    enableemitters = 0;
}

// Anisotropic filtering.

int AurQueryCurrentAnisotropy()
{
    return g_nCurrentAnisotropy;
}

int AurQueryMaximumAnisotropy()
{
    return static_cast<int>(largest_supported_anisotropy);
}

float QueryMaxAnisotropicFilter()
{
    return largest_supported_anisotropy;
}

// An over-large request is ignored and the current setting is returned.
int AurSetAnisotropy(int nLevel)
{
#if KOTOR1  // K1: gated on the extension, and always hands back the current setting
    if (AurAnisotropicFilterAvailable() && largest_supported_anisotropy >= static_cast<float>(nLevel))
    {
        GLRender::cm_nAnisotropy = nLevel;
        g_nCurrentAnisotropy = nLevel;
    }
    return g_nCurrentAnisotropy;
#else
    if (largest_supported_anisotropy >= static_cast<float>(nLevel))
    {
        GLRender::cm_nAnisotropy = nLevel;
        g_nCurrentAnisotropy = nLevel;
        return nLevel;
    }
    return g_nCurrentAnisotropy;
#endif
}

bool SetAnisotropicFilter(int nLevel)
{
#if KOTOR1  // K1: gated on the extension
    if (AurAnisotropicFilterAvailable())
    {
        if (largest_supported_anisotropy >= static_cast<float>(nLevel))
        {
            GLRender::cm_nAnisotropy = nLevel;
            g_nCurrentAnisotropy = nLevel;
            return true;
        }
    }
    return false;
#else
    if (largest_supported_anisotropy >= static_cast<float>(nLevel))
    {
        GLRender::cm_nAnisotropy = nLevel;
        g_nCurrentAnisotropy = nLevel;
        return true;
    }
    return false;
#endif
}

// VSync, clear colour, alpha.

#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
extern "C" void k2host_set_vsync(int bEnabled);   // platforms/k1/k1sdl.cpp
#endif
void AurSetVSyncState(bool bEnabled)
{
#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
    // desktop K1: the Advanced Graphics row drives the host's swap interval
    k2host_set_vsync(bEnabled);
#elif !KOTOR1  // K1: an empty stub (no swap-interval control)
    if (AurSwapControlAvailable())
    {
        wglSwapIntervalEXT(bEnabled);
        GLRender::cm_bVSync = bEnabled;
    }
#else
    K2_UNUSED(bEnabled);
#endif
}

bool AurGetVSyncState()
{
    return false;
}

// Camera derives from both CAurCamera and Gob, so the downcast from the Gob pointer adjusts by 8.
// The view-volume array is read as a `Plane*` although the member is spelled `Quaternion*`, since
// the two are the same size and the predicate reads it as a plane.
bool QueryPointInFrustrum(Vector& vPoint, float fRadius)
{
    Camera* pCamera = static_cast<Camera*>(FindGob(const_cast<char*>("camera")));

    for (int i = 0; i < pCamera->m_viewState.m_aViewVolumes.m_count; i++)
    {
        if (SphereAbovePlane(vPoint, fRadius,
                             pCamera->m_viewState.m_aViewVolumes[i]) > 0.0f)
            return false;
    }
    return true;
}

// Logs the error string; the result is discarded.
void AurGetGLError()
{
    unsigned int nError = ASLgl::glGetError();
    if (nError != 0)
        gluErrorString(nError);
}

void AurSetGlobalAlpha(float fAlpha)
{
    g_fGlobalAlpha = fAlpha;
}

void AurUnloadFrameBufferMemory()
{
}

void AurLoadFrameBufferMemory()
{
}

void AurSetClearColor(float fRed, float fGreen, float fBlue, float fAlpha)
{
    ASLgl::glClearColor(fRed, fGreen, fBlue, fAlpha);
}

void AurGetClearColor(float* pfColor)
{
    GLRender::GetClearColor(pfColor);
}

// The movie-style post effects, speed blur, saturation, scan and film noise.

bool AurIsSpeedBlurEnabled()
{
    return GLRender::m_bSpeedBlurEnabled;
}

// The user's request and the effect's actual state are two different latches. The request is what
// the options screen writes back, and it only reaches the renderer through the force-effect gate.
void AurEnableSpeedBlur()
{
#if KOTOR1  // K1: no request latch, no force-effect gate
    GLRender::m_bSpeedBlurEnabled = true;
#else
    g_gameWantsSpeedBlur = true;
    GLRender::m_bSpeedBlurEnabled = g_allowForceSpeedEffect;
#endif
}

void AurDisableSpeedBlur()
{
#if !KOTOR1
    g_gameWantsSpeedBlur = false;
#endif
    GLRender::m_bSpeedBlurEnabled = false;
}

const char* startblur(char* pszArgs)
{
    K2_UNUSED(pszArgs);
#if KOTOR1
    GLRender::m_bSpeedBlurEnabled = true;
#else
    g_gameWantsSpeedBlur = true;
    GLRender::m_bSpeedBlurEnabled = g_allowForceSpeedEffect;
#endif
    return "blah";
}

const char* endblur(char* pszArgs)
{
    K2_UNUSED(pszArgs);
#if !KOTOR1
    g_gameWantsSpeedBlur = false;
#endif
    GLRender::m_bSpeedBlurEnabled = false;
    return "blah";
}

void AurEnableSaturation()
{
    GLRender::m_bSaturationEnabled = true;
}

void AurDisableSaturation()
{
    GLRender::m_bSaturationEnabled = false;
}

void AurEnableScanNoise()
{
    GLRender::m_bScanNoiseEnabled = true;
}

void AurDisableScanNoise()
{
    GLRender::m_bScanNoiseEnabled = false;
}

void AurEnableFilmNoise()
{
    GLRender::m_bFilmNoiseEnabled = true;
}

void AurDisableFilmNoise()
{
    GLRender::m_bFilmNoiseEnabled = false;
}

void AurEnableClairvoyance(bool bEnabled)
{
    GLRender::EnableDreamDistortion(bEnabled);
}

void AurDisableClairvoyance()
{
    GLRender::DisableDreamDistortion();
}

void AurEnableForceSight()
{
    g_ForceSight = true;
}

void AurDisableForceSight()
{
    g_ForceSight = false;
}

void AurEnableForceFury(short nLevel)
{
    GLRender::EnableFury(nLevel);
}

void AurSetSaturationParams(Vector vMultiplier, float fSaturation)
{
    GLRender::m_vSaturationMultiplier = vMultiplier;
    GLRender::m_fSaturation = fSaturation;
}

void AurSetSpeedBlurRatio(float fRatio)
{
    GLRender::m_fAccumulationRatio = fRatio;
}

void AurPersistDisplay()
{
}

// Sets the pause-overlay colour, red, green, blue and alpha packed into one word. The conversions
// are unsigned, so the casts are to `unsigned int`.
void AurSetPauseOverlayColor(float fRed, float fGreen, float fBlue, float fAlpha)
{
    g_rgbaPauseOverlayColor = static_cast<unsigned int>(fRed * 255.0f)
                            | (static_cast<unsigned int>(fGreen * 255.0f) << 8)
                            | (static_cast<unsigned int>(fBlue * 255.0f) << 16)
                            | (static_cast<unsigned int>(fAlpha * 255.0f) << 24);
}

// Unlike AurSetPauseOverlayColor this parses doubles and scales them as doubles.
const char* testsetpausecolor(char* pszArgs)
{
    char* pszCursor = pszArgs;
    double fRed = atof(GetToken(&pszCursor));
    double fGreen = atof(GetToken(&pszCursor));
    double fBlue = atof(GetToken(&pszCursor));
    double fAlpha = atof(GetToken(&pszCursor));

    g_rgbaPauseOverlayColor = static_cast<unsigned int>(fRed * 255.0)
                            | (static_cast<unsigned int>(fGreen * 255.0) << 8)
                            | (static_cast<unsigned int>(fBlue * 255.0) << 16)
                            | (static_cast<unsigned int>(fAlpha * 255.0) << 24);
    return "SUCCESS";
}

// Anti-aliasing.

// The previous mode is stashed before the new one lands, as `setaliasing` also does.
int AurSetAntiAliasingMode(int nMode)
{
#if KOTOR1  // K1: records the setting, then validates it against the driver's support
    g_IosAntiAlias = nMode;
    if (nMode < 0)
        return 0;
    if (nMode != 0) {
        int bAvailable = AurIsAAliasingAvailable();
        if (nMode > 7 || bAvailable == 0)
            return 0;
    }
#endif
    g_nOldAliasingMode = g_nAliasingMode;
    g_nAliasingMode = nMode;
    return 1;
}

// Answers whether the mode is supported. The availability probe runs before the range check, and
// all three arms share one return.
int AurQueryAntiAliasingMode(int nMode)
{
    bool bSupported;

    if (nMode < 0)
        bSupported = false;
    else if (nMode == 0)
        bSupported = true;
    else
        bSupported = (AurIsAAliasingAvailable() != 0) & (nMode < 8);
#if defined(K2_QOL_IMPROVEMENTS) || defined(K2_QOL_K1_ANTIALIAS)
    // Supersampling's present pass resolves at most 2x per axis: 2 and 4 samples.
    bSupported = bSupported && nMode <= 4;
#endif

    return bSupported;
}

// Soft shadows and vertex-buffer objects.

void AurSetImageSpaceSoftShadowState(bool bEnable)
{
    GLRender::cm_bSetUpImageSpaceShadows = bEnable;
}

void AurFrameBufferSetUpState(bool bEnable)
{
    GLRender::cm_bSetUpFrameBuffer = bEnable;
}

void AurDisableVertexBufferObjects()
{
    g_bDisableVertexBufferObjects = true;
    gHasRevComps_Mac = false;
}

void AurDisableVertexBufferObjectsWriteOnly()
{
    g_bVertexBufferObjectsWriteOnly = false;
}

void AurEnableVertexBufferObjectsWriteOnly()
{
    g_bVertexBufferObjectsWriteOnly = true;
}

void AurATIUnlockSoftShadowsForHotFix()
{
    g_nUnlockATISoftShadowsForHotFix = 1;
    enablesoftshadows = 0;
}

// Console handlers that only shuffle the anti-aliasing and anisotropy globals.

// Same guard as AurSetAnisotropy, with the success arm first.
const char* setanisotropy(char* pszArgs)
{
    int nLevel = atoi(pszArgs);

#if KOTOR1  // K1: gated on the extension
    if (AurAnisotropicFilterAvailable() && largest_supported_anisotropy >= static_cast<float>(nLevel))
#else
    if (largest_supported_anisotropy >= static_cast<float>(nLevel))
#endif
    {
        GLRender::cm_nAnisotropy = nLevel;
        g_nCurrentAnisotropy = nLevel;
        return "SUCCESS";
    }
    return "FAILURE";
}

// Reads one GL sample count back before parsing its argument, and discards the result.
const char* setaliasing(char* pszArgs)
{
#if KOTOR1  // K1: no GL sample query; the same validation as AurSetAntiAliasingMode
    int nMode = atoi(pszArgs);
    g_IosAntiAlias = nMode;
    if (nMode < 0)
        return "FAILURE";
    if (nMode != 0) {
        int bAvailable = AurIsAAliasingAvailable();
        if (nMode > 7 || bAvailable == 0)
            return "FAILURE";
    }
    g_nOldAliasingMode = g_nAliasingMode;
    g_nAliasingMode = nMode;
    return "SUCCESS";
#else
    int nUnused;
    ASLgl::glGetIntegerv(GL_SAMPLES, &nUnused);

    int nMode = atoi(pszArgs);
    g_nOldAliasingMode = g_nAliasingMode;
    g_nAliasingMode = nMode;
    return "SUCCESS";
#endif
}

// Fog of war.

// Removes one debug square from the render list.
const char* removesquaretorender(char* pszArgs)
{
    int nHandle = atoi(GetToken(&pszArgs));
    if (nHandle != -1)
        GLRender::RemoveSquareToRender(nHandle);
    return "SUCKS-EGGS";
}

// The leaf predicate for the fog-of-war point search; a room matches when it carries tile data.
static int FogOfWarNodeHasTileData(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    CAurRoom* pRoom = static_cast<CAurRoom*>(pNode);
    return pRoom->m_pNodeData->m_pTileData != 0;
}

void AuroraSetTileFoggedByWar(Vector& vTile, bool bFogged)
{
    Scene* pScene = FindScene(const_cast<char*>("mainscene"));

    CAurRoom* pRoom = static_cast<CAurRoom*>(BSPSearch(pScene->m_pBSPRoot, vTile, FogOfWarNodeHasTileData, 0));

    TileData* pTileData = pRoom->m_pNodeData->m_pTileData;
    pTileData->m_bFogged = bFogged;

    if (bFogged)
    {
        // A newly-fogged tile starts fully faded (fade factor 0 = nothing drawn), and the room
        // joins the per-frame overlay set.
        pTileData->m_fFade = 0.0f;
        pScene->m_fogOfWarRooms.Add(pRoom);
    }
    else
    {
        pScene->m_fogOfWarRooms.Remove(pRoom);
    }
}

const char* setfogofwar(char* pszArgs)
{
    char* pszCursor = pszArgs;

    Vector vPosition;
    vPosition.x = static_cast<float>(atof(GetToken(&pszCursor)));
    vPosition.y = static_cast<float>(atof(GetToken(&pszCursor)));
    vPosition.z = static_cast<float>(atof(GetToken(&pszCursor)));

    AuroraSetTileFoggedByWar(vPosition, atoi(GetToken(&pszCursor)) == 1);
    return "FOGGED";
}

// Walks the model's supermodel chain, adding `<name>.mdl` for the model and every ancestor, then
// the node tree's own dependencies. `disablepostprocessing` is held high across the walk so the
// texture sweeps cannot recurse back into it. The ref pair brackets the model rather than passing
// it.
int AurGetModelDependencies(char* pszName, List<char*>& list)
{
    disablepostprocessing = true;

    Model* pModel = FindModel(pszName);
    if (pModel == 0)
    {
        disablepostprocessing = false;
        return 0;
    }

    pModel->AddRef(pModel);

    char* pszPath = new char[32];
    __builtin___strcpy_chk(pszPath, pModel->m_szName, 32);
    __builtin___strcat_chk(pszPath, ".mdl", 32);
    AurAddDependency(pszPath, list);

    // The chain is walked through a cursor so every iteration re-reads the supermodel pointer.
    Model* pSuper = pModel;
    Model** ppSuper = &pModel->m_pSuperModel;
    while (*ppSuper != 0)
    {
        char* pszSuper = new char[32];
        __builtin___strcpy_chk(pszSuper, (*ppSuper)->m_szName, 32);
        __builtin___strcat_chk(pszSuper, ".mdl", 32);
        AurAddDependency(pszSuper, list);

        pSuper = *ppSuper;
        ppSuper = &pSuper->m_pSuperModel;
    }

    if (pSuper->m_pMdlNode != 0)
        AurAddNodeDependencies(*pSuper->m_pMdlNode, list);

    disablepostprocessing = false;
    pModel->DelRef(pSuper);
    return 1;
}

// Model dependency walking finds which .mdl, .tga and .txi files a model still needs.

// Adds the name unless it is already in the list. The scan does not break early: every comparison
// is accumulated and the flag is tested once after the loop.
void AurAddDependency(char* pszName, List<char*>& list)
{
    bool bAlreadyListed = false;
    for (int i = 0; i < list.m_count; i++)
        bAlreadyListed |= (strcmp(pszName, list.m_pData[i]) == 0);

    if (!bAlreadyListed)
        list.Add(pszName);
}

// The recursive .tga/.txi collector. For a tri-mesh node it adds the diffuse texture, the lightmap
// under both suffixes, whatever the material's own textures name, and one `<name><frame>.tga` per
// frame of a ranged texture; for an emitter node it adds the texture and depth-texture names.
//
// The buffers are declared at function scope and reused, so the collector opens one
// exception-handling scope rather than one per block.
void AurAddNodeDependencies(MdlNode& node, List<char*>& list)
{
    char szFrame[32];
    char* psz;

    if (node.IsMdlNodeTriMesh())
    {
        MdlNodeTriMesh& mesh = static_cast<MdlNodeTriMesh&>(node);

        psz = new char[32];
        __builtin___strcpy_chk(psz, mesh.m_szTexture, 32);
        __builtin___strcat_chk(psz, ".tga", 32);
        AurAddDependency(psz, list);

        if (mesh.m_szLightmap[0] != 0)
        {
            psz = new char[32];
            __builtin___strcpy_chk(psz, mesh.m_szLightmap, 32);
            __builtin___strcat_chk(psz, ".tga", 32);
            AurAddDependency(psz, list);

            psz = new char[32];
            __builtin___strcpy_chk(psz, mesh.m_szLightmap, 32);
            __builtin___strcat_chk(psz, ".txi", 32);
            AurAddDependency(psz, list);
        }

        Material material;
        material.InitializeTextures(mesh.m_szTexture, 0, 0, 0);

        if (material.Textures(0).m_pObject->IsTxiLoaded())
        {
            psz = new char[32];
            __builtin___strcpy_chk(psz, mesh.m_szTexture, 32);
            __builtin___strcat_chk(psz, ".txi", 32);
            AurAddDependency(psz, list);
        }

        if (material.Textures(4).m_pObject != 0)
        {
            psz = new char[32];
            __builtin___strcpy_chk(psz, material.Textures(4).m_pObject->GetName(), 32);
            __builtin___strcat_chk(psz, ".tga", 32);
            AurAddDependency(psz, list);

            psz = new char[32];
            __builtin___strcpy_chk(psz, material.Textures(4).m_pObject->GetName(), 32);
            __builtin___strcat_chk(psz, ".txi", 32);
            AurAddDependency(psz, list);
        }

        if (material.Textures(5).m_pObject != 0)
        {
            int nFrames = material.Textures(5).m_pObject->GetFileRange();
            if (nFrames == 0)
            {
                psz = new char[32];
                __builtin___strcpy_chk(psz, material.Textures(5).m_pObject->GetName(), 32);
                __builtin___strcat_chk(psz, ".tga", 32);
                AurAddDependency(psz, list);
            }
            else
            {
                for (int i = 0; i < nFrames; i++)
                {
                    _itoa(i, szFrame, 10);
                    psz = new char[32];
                    __builtin___strcpy_chk(psz, material.Textures(5).m_pObject->GetName(), 32);
                    __builtin___strcat_chk(psz, szFrame, 32);
                    __builtin___strcat_chk(psz, ".tga", 32);
                    AurAddDependency(psz, list);
                }
            }

            psz = new char[32];
            __builtin___strcpy_chk(psz, material.Textures(5).m_pObject->GetName(), 32);
            __builtin___strcat_chk(psz, ".txi", 32);
            AurAddDependency(psz, list);
        }
    }
    else if (node.IsMdlNodeEmitter())
    {
        MdlNodeEmitter& emitter = static_cast<MdlNodeEmitter&>(node);

        if (emitter.m_szTexture[0] != 0)
        {
            psz = new char[32];
            __builtin___strcpy_chk(psz, emitter.m_szTexture, 32);
            __builtin___strcat_chk(psz, ".tga", 32);
            AurAddDependency(psz, list);

            Material material;
            material.InitializeTextures(emitter.m_szTexture, 0, 0, 0);

            if (material.Textures(0).m_pObject->IsTxiLoaded())
            {
                psz = new char[32];
                __builtin___strcpy_chk(psz, emitter.m_szTexture, 32);
                __builtin___strcat_chk(psz, ".txi", 32);
                AurAddDependency(psz, list);
            }
        }

        if (emitter.m_szDepthTextureName[0] != 0)
        {
            psz = new char[32];
            __builtin___strcpy_chk(psz, emitter.m_szDepthTextureName, 32);
            __builtin___strcat_chk(psz, ".tga", 32);
            AurAddDependency(psz, list);
        }
    }

    for (int64_t i = 0; i < node.m_listChildren.m_count; i++)
        AurAddNodeDependencies(*node.m_listChildren.m_pData[i], list);
}

// Renderer singleton wrappers.
CAurInternal* NewCAurora()
{
    return CAurInternal::NewCAuroraInstance();
}

void DestroyCAurora()
{
    CAurInternal::DestroyCAuroraInstance();
}

// The once-per-frame renderer hand-off. `sleepseconds` is a debug throttle, busy-waited rather
// than slept: the end point is computed once from `clock()` plus the throttle, then `clock()` is
// re-read until it is reached.
float sleepseconds;
extern float memoryfree;

#include "port/win32api.h"
#include "port/libctime.h"

// A microsecond busy-wait, the same shape AurMakeCurrent needs. The end point is computed once and
// then `clock()` is re-read until it is reached.
void sleep(int64_t nMicroseconds)
{
    int64_t nWake = static_cast<int64_t>(clock()) + nMicroseconds;
    while (nWake > static_cast<int64_t>(clock())) {
    }
}

void AurTextureMakeCurrent();
void AurMakeCurrent()
{
    if (sleepseconds != 0.0f) {
        int64_t nWake = static_cast<int64_t>(sleepseconds * 1000000.0f) + static_cast<int64_t>(clock());
        while (nWake > static_cast<int64_t>(clock()))
            ;
    }

    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    memoryfree = static_cast<float>(ms.dwAvailPhys) / 1024.0f;

    AurTextureMakeCurrent();
}

// Gob::Gob calls this across translation units, so it must stay out of line.
CAurPathInfo::CAurPathInfo()
{
    m_pPathFloats = 0;
    m_nPathFloats = 0;
}
