
#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurinternal.h"
#include "aurglcore.h"
#include "aurvertexprog.h"


#if !KOTOR1
bool gMac_ShadersNeedInit = true;
bool gMac_GammaShaderNeedsInit = true;
#endif

short CAurInternal::m_nHeadLightEnabled = -1;
short CAurInternal::m_nColorDepth;
short CAurInternal::m_nDepthBits;
int CAurInternal::AuroraCount;
CAurInternal* CAurInternal::AuroraInstance;

extern HWND__* g_hRenderWnd;
float g_fGammaExponent = 1.0f;
extern int   g_nScreenWidth;
extern int   g_nScreenHeight;
#include "port/win32api.h"
char* untrace(char* pszExpr);
void  AurStringsFree();
void  AurFreeFont();
void  AurDeleteAllFonts();


void AurTextureClearAll();

// The vertex-program slots the renderer fills in and frees at shutdown. A plain scalar delete is
// correct here: VertexProgram's destructor is not virtual.
extern VertexProgram* g_pBumpyShinySkinPass1;
extern VertexProgram* g_pBumpyShinySkinPass2;
extern VertexProgram* g_pBumpyShinySkinPass2_3bone;
extern VertexProgram* g_pBumpyShinyPass1;
extern VertexProgram* g_pBumpyShinyPass2;
extern VertexProgram* g_pBumpyShinyPass3;
extern VertexProgram* g_pBumpyShinyPass4;
extern VertexProgram* g_pBumpyShinyPass5;
extern VertexProgram* g_pSkinBumpedOutVP;
extern VertexProgram* g_pSkinBumpedOutWithTangentVP;
extern VertexProgram* g_pBumpedOutVP;
extern char* szBumpyShinyVPARB;
extern char* szBumpyShinySkinVPARB;

// Turns the head light on and latches the mode. The mode is compared against 1 rather than tested
// for truth, because the -1 "not known yet" seed still has to reach GL the first time.
void AurEnableHeadLight()
{
    if (CAurInternal::m_nHeadLightEnabled != 1)
        ASLgl::glEnable(GL_LIGHT0);
    CAurInternal::m_nHeadLightEnabled = 1;
}

void AurDisableHeadLight()
{
    if (CAurInternal::m_nHeadLightEnabled != 0)
        ASLgl::glDisable(GL_LIGHT0);
    CAurInternal::m_nHeadLightEnabled = 0;
}

CAurInternal::CAurInternal()
{
    m_bDisplayModeSet = 0;
    m_bFullScreen = 0;
    m_fGamma = 1.0f;
    m_nColorDepth = 0;
}

CAurInternal::~CAurInternal() {}

CAurInternalGL::CAurInternalGL()
{
    m_pGLContext = 0;
}

CAurInternal* CAurInternal::NewCAuroraInstance()
{
    if (AuroraInstance == 0)
        AuroraInstance = new CAurInternalGL();
    AuroraCount++;
    return AuroraInstance;
}

CAurInternal* CAurInternalGL::NewCAuroraInstance()
{
    if (AuroraInstance == 0)
        AuroraInstance = new CAurInternalGL();
    AuroraCount++;
    return AuroraInstance;
}

void CAurInternal::DestroyCAuroraInstance()
{
    if (AuroraInstance != 0 && --AuroraCount == 0)
    {
        delete AuroraInstance;
        AuroraInstance = 0;
    }
}

int CAurInternal::FullScreen()
{
    return m_bFullScreen;
}

// Re-latches the gamma at 1.0 and, only when the port installed its own mode, hands the display
// back to the default.
void CAurInternal::RestoreDesktopDisplayMode()
{
    ResetGammaRamp(1.0f, 1);
    if (m_bDisplayModeSet != 0)
        ChangeDisplaySettingsA(0, 0);
}

// Re-latches the gamma and asks for the port's own fullscreen mode again.
void CAurInternal::RestoreAuroraDisplayMode()
{
    ResetGammaRamp(m_fGamma, 1);
#if KOTOR1
    if (m_bDisplayModeSet != 0 && ChangeDisplaySettingsA(&m_devMode, 2) == 0)
        ChangeDisplaySettingsA(&m_devMode, 4);
#else
    if (m_bDisplayModeSet != 0)
        ChangeDisplaySettingsA(&m_devMode, 4);
#endif
}

// Recomputes the gamma ramp and installs it. Returns 1 on success.
int CAurInternal::ResetGammaRamp(float fGamma, int nFlag)
{
#if KOTOR1
    K2_UNUSED(nFlag);
    g_fGammaExponent = fGamma;
    return 1;
#else
    if (nFlag == 0)
        m_fGamma = fGamma;

    void* hDC = GetDC(g_hRenderWnd);
    if (hDC == 0)
        return 0;

    g_fGammaExponent = fGamma;
    ReleaseDC(g_hRenderWnd, hDC);
    return 1;
#endif
}

// Sets the viewport, clamped to at least one pixel per axis.
void CAurInternalGL::Reshape(int nWidth, int nHeight)
{
    if (m_pGLContext == 0)
        return;

    m_nWidth = nWidth;
    m_nHeight = nHeight;
    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    ASLgl::glViewport(0, 0, (nWidth != 0) ? nWidth : 1, (nHeight != 0) ? nHeight : 1);
}

void* CAurInternalGL::GetGLContext()
{
    return m_pGLContext;
}

CAurInternalGL::~CAurInternalGL()
{
    untrace(0);
    AurStringsFree();
    AurFreeFont();
    AurDeleteAllFonts();
    ShutDown(1, 1);
}

#if KOTOR1
void CAurInternalGL::MakeCurrentGLContext(void*, void*) {}
#else
void CAurInternalGL::MakeCurrentGLContext(HDC__*, HGLRC__*) {}
#endif

void* CAurInternal::GetGLContext()
{
    return 0;
}

#if !KOTOR1
int CAurInternal::InitEditor(HWND__*, HWND__*, unsigned int, unsigned int, unsigned char,
                            unsigned int, int)
{
    return 0;
}
#endif

// Releases the renderer: textures, GL state, the vertex-program slots, the GL context and the window.
// bReleaseResources gates the whole teardown, bRestoreDisplayMode only the display-mode restore.
void CAurInternalGL::ShutDown(int bReleaseResources, int bRestoreDisplayMode)
{
    VertexProgramManager::ResetVertexPrograms();

    if (g_pBumpyShinySkinPass1 != 0) {
        delete g_pBumpyShinySkinPass1;
        g_pBumpyShinySkinPass1 = 0;
    }
    if (g_pBumpyShinySkinPass2 != 0) {
        delete g_pBumpyShinySkinPass2;
        g_pBumpyShinySkinPass2 = 0;
    }
    if (g_pBumpyShinySkinPass2_3bone != 0) {
        delete g_pBumpyShinySkinPass2_3bone;
        g_pBumpyShinySkinPass2_3bone = 0;
    }
    if (g_pBumpyShinyPass1 != 0) {
        delete g_pBumpyShinyPass1;
        g_pBumpyShinyPass1 = 0;
    }
    if (g_pBumpyShinyPass2 != 0) {
        delete g_pBumpyShinyPass2;
        g_pBumpyShinyPass2 = 0;
    }
    if (g_pBumpyShinyPass3 != 0) {
        delete g_pBumpyShinyPass3;
        g_pBumpyShinyPass3 = 0;
    }
    if (g_pBumpyShinyPass4 != 0) {
        delete g_pBumpyShinyPass4;
        g_pBumpyShinyPass4 = 0;
    }
    if (g_pBumpyShinyPass5 != 0) {
        delete g_pBumpyShinyPass5;
        g_pBumpyShinyPass5 = 0;
    }
    if (g_pSkinBumpedOutVP != 0) {
        delete g_pSkinBumpedOutVP;
        g_pSkinBumpedOutVP = 0;
    }
    if (g_pSkinBumpedOutWithTangentVP != 0) {
        delete g_pSkinBumpedOutWithTangentVP;
        g_pSkinBumpedOutWithTangentVP = 0;
    }
    if (g_pBumpedOutVP != 0) {
        delete g_pBumpedOutVP;
        g_pBumpedOutVP = 0;
    }

    if (bReleaseResources != 0)
        AurTextureClearAll();
    GLRender::Shutdown(bReleaseResources);

    if (szBumpyShinyVPARB != 0) {
        delete szBumpyShinyVPARB;
        szBumpyShinyVPARB = 0;
    }
    if (szBumpyShinySkinVPARB != 0) {
        delete szBumpyShinySkinVPARB;
        szBumpyShinySkinVPARB = 0;
    }

#if !KOTOR1
    if (m_pGLContext != 0)
    {
        wglMakeCurrent(0, 0);
        wglDeleteContext(m_pGLContext);
        m_pGLContext = 0;
    }
#endif

    if (m_bDisplayModeSet != 0)
    {
        if (bRestoreDisplayMode != 0)
            ChangeDisplaySettingsA(0, 0);
        m_bDisplayModeSet = 0;
        m_bFullScreen = 0;
    }

    if (bReleaseResources != 0)
        ResetGammaRamp(1.0f, 0);

    if (g_hRenderWnd != 0)
    {
        DestroyWindow(g_hRenderWnd);
        g_hRenderWnd = 0;
    }
}


void AurInitExtensions();
int  AurSetAntiAliasingMode(int nMode);
int  AurVertexProgramsAvailable();
void TrackMatrices();
void AurTextureRefreshAll();

extern int g_nAliasingMode;
extern int g_nOldAliasingMode;

extern int  usearbvertexprograms;
extern bool gMac_ShadersNeedInit;
extern bool gMac_GammaShaderNeedsInit;

extern unsigned int enableTranslationTable[];

extern char  cBumpyShinySkinVP[];
extern char  cSkinVP[];
extern char  cStandardVP[];
extern char  cBumpyShinyVP[];
extern char  cSkinBumpedOutVP[];
extern char  cBumpedOutVP[];
extern char  szSkinVPARB[];
extern char  szStandardVPARB[];
extern char  szStandardFogVPARB[];
extern char  szSkinBumpedOutVPARB[];
extern char  szBumpedOutVPARB[];
extern unsigned int* cBumpyShinySkinVPDeclaration;
extern unsigned int* cStandardVPDeclaration;
extern unsigned int* cStandardVP2Declaration;
extern unsigned int* cBumpyShinyRegularVPDeclaration;
extern unsigned int* cBumpyShinyLightMappedVPDeclaration;
extern unsigned int* cSkinBumpedOutVPDeclaration;

static bool s_bAntiAliasingInitialized = false;

// The WGL pixel-format attributes, pixel-format descriptor flags and SetWindowPos flags the window
// setup passes, as wglext.h and winuser.h/wingdi.h spell them (the engine never includes those).
#define WGL_DRAW_TO_WINDOW_ARB                   0x2001
#define WGL_ACCELERATION_ARB                     0x2003
#define WGL_DOUBLE_BUFFER_ARB                    0x2011
#define WGL_COLOR_BITS_ARB                       0x2014
#define WGL_ALPHA_BITS_ARB                       0x201B
#define WGL_DEPTH_BITS_ARB                       0x2022
#define WGL_STENCIL_BITS_ARB                     0x2023
#define WGL_FULL_ACCELERATION_ARB                0x2027
#define WGL_SAMPLE_BUFFERS_ARB                   0x2041
#define WGL_SAMPLES_ARB                          0x2042
#define PFD_DOUBLEBUFFER                         0x00000001
#define PFD_DRAW_TO_WINDOW                       0x00000004
#define PFD_SUPPORT_OPENGL                       0x00000020
#define SWP_FRAMECHANGED                         0x0020
#define SWP_NOCOPYBITS                           0x0100

enum {
    kWindowedStyle  = 0x02cf0000,
    kFullScreenStyle = 0x82000000,
    kGWL_STYLE      = -16,
};

#if KOTOR1
int CAurInternalGL::Initialize(void*, void*, unsigned int, unsigned int, unsigned char, unsigned int, int,
                               int, int)
{
    if (!s_bAntiAliasingInitialized)
    {
        s_bAntiAliasingInitialized = true;
        GLRender::InitializeAntiAliasing();
    }

    CAurInternal::m_nHeadLightEnabled = static_cast<short>(0xffff);
    m_bFullScreen = 1;
    CAurInternal::m_nColorDepth = 32;
    CAurInternal::m_nDepthBits  = 24;
    if (g_nAliasingMode != 0)
        AurSetAntiAliasingMode(g_nAliasingMode);

    AurInitExtensions();
    if (AurVertexProgramsAvailable() != 0)
        TrackMatrices();
    GLRender::InitRenderSystem();
    AurTextureRefreshAll();
    GLRender::InitializeImageSpaceSoftShadows();
    GLRender::InitializeFrameBufferModifications();

    if (AurVertexProgramsAvailable() != 0)
    {
        ASLgl::glEnable(enableTranslationTable[9]);
        VertexProgram* pVP;
        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinySkinVPARB
                                                          : cBumpyShinySkinVP,
                                cBumpyShinySkinVPDeclaration);
        g_pBumpyShinySkinPass1 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinVPARB : cSkinVP,
                                cBumpyShinySkinVPDeclaration);
        g_pBumpyShinySkinPass2 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                cStandardVP2Declaration);
        g_pBumpyShinyPass1 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardFogVPARB : cStandardVP,
                                cStandardVPDeclaration);
        g_pBumpyShinyPass2 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                cBumpyShinyRegularVPDeclaration);
        g_pBumpyShinyPass3 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                cBumpyShinyRegularVPDeclaration);
        g_pBumpyShinyPass4 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                cBumpyShinyLightMappedVPDeclaration);
        g_pBumpyShinyPass5 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinBumpedOutVPARB
                                                          : cSkinBumpedOutVP,
                                cSkinBumpedOutVPDeclaration);
        g_pSkinBumpedOutVP = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinBumpedOutVPARB
                                                          : cSkinBumpedOutVP,
                                cBumpyShinySkinVPDeclaration);
        g_pSkinBumpedOutWithTangentVP = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpedOutVPARB : cBumpedOutVP,
                                cStandardVPDeclaration);
        g_pBumpedOutVP = pVP;
        pVP->Enable(0, false);

        ASLgl::glDisable(enableTranslationTable[9]);
    }
    return 1;
}
#else
// Brings up the window and the GL context: a display-mode search, the Win32 window style, rect and
// size dance, two pixel-format paths (WGL_ARB_pixel_format first, ChoosePixelFormat as the
// fallback), then the vertex programs and the anti-aliasing latch.
int CAurInternalGL::Initialize(HWND__*, HWND__* hWnd, unsigned int nWidth, unsigned int nHeight,
                               unsigned char nBitsPerPel, unsigned int nFrequency, int bSetMode,
                               int nColorBits, int nDepthBits)
{
    if (!s_bAntiAliasingInitialized)
    {
        s_bAntiAliasingInitialized = true;
        GLRender::InitializeAntiAliasing();
    }

    CAurInternal::m_nHeadLightEnabled = static_cast<short>(0xffff);
    PortDevMode devMode;

    bool          bFullScreen;
    HWND__*       hWndInsertAfter;
    unsigned int  nStyle;

    if (bSetMode != 0)
    {
        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);

        if (EnumDisplaySettingsA(0, 0, &devMode))
        {
            unsigned char nBits = nBitsPerPel;
            int i = 1;
            for (;;)
            {
                if (devMode.m_pelsWidth == nWidth && devMode.m_pelsHeight == nHeight
                    && devMode.m_bitsPerPel == nBits && devMode.m_displayFrequency == nFrequency)
                {
                    m_devMode = devMode;
                    ChangeDisplaySettingsA(&devMode, 4);
                    m_bDisplayModeSet = 1;
                    break;
                }

                __builtin_memset(&devMode, 0, sizeof(devMode));
                devMode.m_size = sizeof(devMode);
                if (!EnumDisplaySettingsA(0, i, &devMode))
                    break;
                i++;
            }
        }

        if (m_bDisplayModeSet != 0)
        {
            bFullScreen    = true;
            hWndInsertAfter = reinterpret_cast<HWND__*>(-1);
            nStyle         = static_cast<unsigned int>(kFullScreenStyle);
            goto haveWindowStyle;
        }
    }

    bFullScreen     = false;
    hWndInsertAfter = reinterpret_cast<HWND__*>(-2);
    nStyle          = kWindowedStyle;

haveWindowStyle:
    m_bFullScreen = bFullScreen;
    SetWindowLongA(hWnd, kGWL_STYLE, nStyle);

    RECT rect;
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = static_cast<int>(nWidth);
    rect.bottom = static_cast<int>(nHeight);
    AdjustWindowRect(&rect, nStyle, 0);

    if (!bFullScreen)
        SetWindowPos(hWnd, hWndInsertAfter, 0, 0, rect.right - rect.left,
                     rect.bottom - rect.top, SWP_NOCOPYBITS | SWP_FRAMECHANGED);
    SendMessageA(hWnd, 5, 0,
                 static_cast<unsigned int>(static_cast<unsigned short>(rect.right - rect.left))
                 | (static_cast<unsigned int>(static_cast<unsigned short>(rect.bottom - rect.top)) << 16));
    ShowWindow_Win32(hWnd, 1);

    void* hDC = GetDC(hWnd);
    if (hDC == 0)
        return 0;

    CAurInternal::m_nColorDepth = 32;
    CAurInternal::m_nDepthBits  = 24;

    bool bAliased;

    if (g_nAliasingMode != 0)
    {
        if (AurSetAntiAliasingMode(g_nAliasingMode) == 0)
        {
            bAliased = false;
            goto choosePixelFormat;
        }

        if (g_nAliasingMode != 0)
        {
            int aAttribs[32];
            aAttribs[0] = WGL_DRAW_TO_WINDOW_ARB;  aAttribs[1] = 1;
            aAttribs[2] = WGL_ACCELERATION_ARB;  aAttribs[3] = WGL_FULL_ACCELERATION_ARB;
            aAttribs[4] = WGL_COLOR_BITS_ARB;  aAttribs[5] = static_cast<int>(nColorBits);
            aAttribs[6] = WGL_ALPHA_BITS_ARB;  aAttribs[7] = 8;
            aAttribs[8] = WGL_DEPTH_BITS_ARB;  aAttribs[9] = (nDepthBits < 24) ? nDepthBits : 24;
            aAttribs[10] = WGL_DOUBLE_BUFFER_ARB; aAttribs[11] = 1;
            aAttribs[12] = WGL_SAMPLE_BUFFERS_ARB; aAttribs[13] = 1;
            aAttribs[14] = WGL_SAMPLES_ARB; aAttribs[15] = g_nAliasingMode;
            aAttribs[16] = WGL_STENCIL_BITS_ARB; aAttribs[17] = 8;
            aAttribs[18] = 0;      aAttribs[19] = 0;
            PIXELFORMATDESCRIPTOR pfdArb;
            float aFAttribs[2] = { 0.0f, 0.0f };
            int          iPixelFormat;
            unsigned int nNumFormats;

            int bArbFailed = 1;
            if (wglChoosePixelFormatARB(hDC, aAttribs, aFAttribs, 1, &iPixelFormat, &nNumFormats) != 0
                && nNumFormats != 0)
            {
                bArbFailed = (SetPixelFormat(hDC, iPixelFormat, &pfdArb) == 0);
                CAurInternal::m_nColorDepth = static_cast<short>(nColorBits);
                CAurInternal::m_nDepthBits  = 24;
            }

            bAliased = !bArbFailed;
            if (!bArbFailed && g_nAliasingMode != 0)
                goto haveGLContext;
        }
        else
        {
            bAliased = true;
        }
    }
    else
    {
        bAliased = true;
    }

choosePixelFormat:
    {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            0, 32,
            0, 0, 0, 0, 0, 0,
            8, 0,
            0, 0, 0, 0, 0,
            24, 8, 0,
            0, 0,
            0, 0, 0
        };

        int iPixelFormat = ChoosePixelFormat(hDC, &pfd);
        if (iPixelFormat == 0)
            goto release;
        if (SetPixelFormat(hDC, iPixelFormat, 0) == 0)
            goto release;

        DescribePixelFormat(hDC, iPixelFormat, sizeof(pfd), &pfd);
        CAurInternal::m_nColorDepth = static_cast<short>(pfd.cColorBits);
        CAurInternal::m_nDepthBits  = static_cast<short>(pfd.cDepthBits);
    }

haveGLContext:
    m_pGLContext = wglCreateContext(hDC);
    if (m_pGLContext == 0)
        goto release;
    if (wglMakeCurrent(hDC, m_pGLContext) == 0)
    {
        wglDeleteContext(m_pGLContext);
        goto release;
    }

    AurInitExtensions();
    if (AurVertexProgramsAvailable() != 0)
        TrackMatrices();
    GLRender::InitRenderSystem();
    AurTextureRefreshAll();
    GLRender::InitializeImageSpaceSoftShadows();
    GLRender::InitializeFrameBufferModifications();

    if (AurVertexProgramsAvailable() != 0)
    {
        ASLgl::glEnable(enableTranslationTable[9]);

        VertexProgram* pVP;

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinySkinVPARB
                                                          : cBumpyShinySkinVP,
                                cBumpyShinySkinVPDeclaration);
        g_pBumpyShinySkinPass1 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinVPARB : cSkinVP,
                                cBumpyShinySkinVPDeclaration);
        g_pBumpyShinySkinPass2 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                cStandardVP2Declaration);
        g_pBumpyShinyPass1 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardFogVPARB : cStandardVP,
                                cStandardVPDeclaration);
        g_pBumpyShinyPass2 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                cBumpyShinyRegularVPDeclaration);
        g_pBumpyShinyPass3 = pVP;
        pVP->Enable(0, true);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                cBumpyShinyRegularVPDeclaration);
        g_pBumpyShinyPass4 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                cBumpyShinyLightMappedVPDeclaration);
        g_pBumpyShinyPass5 = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinBumpedOutVPARB
                                                          : cSkinBumpedOutVP,
                                cSkinBumpedOutVPDeclaration);
        g_pSkinBumpedOutVP = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinBumpedOutVPARB
                                                          : cSkinBumpedOutVP,
                                cBumpyShinySkinVPDeclaration);
        g_pSkinBumpedOutWithTangentVP = pVP;
        pVP->Enable(0, false);

        pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpedOutVPARB : cBumpedOutVP,
                                cStandardVPDeclaration);
        g_pBumpedOutVP = pVP;
        pVP->Enable(0, false);

        ASLgl::glDisable(enableTranslationTable[9]);
    }

    if ((bAliased == false) | (g_nAliasingMode < 1))
    {
        if (g_nOldAliasingMode > 0)
            ASLgl::glDisable(GL_MULTISAMPLE_ARB);
    }
    else
    {
        ASLgl::glEnable(GL_MULTISAMPLE_ARB);
    }

    wglSwapIntervalEXT(GLRender::cm_bVSync);
    gMac_ShadersNeedInit       = 1;
    gMac_GammaShaderNeedsInit  = 1;
    return 1;

release:
    ReleaseDC(hWnd, hDC);
    return 0;
}
#endif

extern char  cSkin3BoneVP[];

extern unsigned int* cSkin3BoneVPDeclaration;

enum {
    kEditorWindowedStyle   = 0x40000000,
    kEditorFullScreenStyle = 0x80000000,
};

#if KOTOR1
int CAurInternalGL::InitEditor(void*, void*, unsigned int, unsigned int, unsigned char, unsigned int, int)
{
    return 1;
}
#else
static bool s_bVertexProgramsInitialized = false;

// Brings up the editor viewport with an optional display-mode search that tests the mode before
// committing it, the same window dance with the editor styles, then eight vertex-program slots.
int CAurInternalGL::InitEditor(HWND__*, HWND__* hWnd, unsigned int nWidth, unsigned int nHeight,
                               unsigned char nBitsPerPel, unsigned int nFrequency, int bSetMode)
{
    ShowWindow_Win32(hWnd, 0);

    PortDevMode devMode;

    unsigned int nStyle;
    HWND__*      hWndInsertAfter;
    bool         bFullScreen;

    if (bSetMode != 0)
    {
        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);

        if (EnumDisplaySettingsA(0, 0, &devMode))
        {
            unsigned char nBits = nBitsPerPel;
            int i = 1;
            for (;;)
            {
                if (devMode.m_pelsWidth == nWidth && devMode.m_pelsHeight == nHeight
                    && devMode.m_bitsPerPel == nBits && devMode.m_displayFrequency == nFrequency)
                {
                    if (ChangeDisplaySettingsA(&devMode, 2) != 0)
                        return 0;
                    m_devMode = devMode;
                    ChangeDisplaySettingsA(&devMode, 4);
                    m_bDisplayModeSet = 1;
                    break;
                }

                __builtin_memset(&devMode, 0, sizeof(devMode));
                devMode.m_size = sizeof(devMode);
                if (!EnumDisplaySettingsA(0, i, &devMode))
                    break;
                i++;
            }
        }

        if (m_bDisplayModeSet == 0)
        {
            ShowWindow_Win32(hWnd, 1);
            return 0;
        }

        nStyle          = static_cast<unsigned int>(kEditorFullScreenStyle);
        hWndInsertAfter = reinterpret_cast<HWND__*>(-1);
        bFullScreen     = true;
        goto haveWindowStyle;
    }

    bFullScreen     = false;
    nStyle          = kEditorWindowedStyle;
    hWndInsertAfter = reinterpret_cast<HWND__*>(-2);

haveWindowStyle:
    m_bFullScreen = bFullScreen;

    void* hDC = GetDC(hWnd);
    if (hDC == 0)
        return 0;
    ReleaseDC(hWnd, hDC);

    RECT rect;
    rect.right  = static_cast<int>(nWidth);
    rect.bottom = static_cast<int>(nHeight);
    rect.left   = 0;
    rect.top    = 0;
    AdjustWindowRect(&rect, nStyle, 0);
    SetWindowLongA(hWnd, kGWL_STYLE, nStyle);

    SetWindowPos(hWnd, hWndInsertAfter, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOCOPYBITS | SWP_FRAMECHANGED);
    SetWindowPos(hWnd, 0, 0, 0, static_cast<int>(nWidth), static_cast<int>(nHeight), SWP_NOCOPYBITS | SWP_FRAMECHANGED);
    SendMessageA(hWnd, 5, 0,
                 static_cast<unsigned int>(static_cast<unsigned short>(nWidth))
                 | (static_cast<unsigned int>(static_cast<unsigned short>(nHeight)) << 16));
    ShowWindow_Win32(hWnd, 1);

    if (!s_bVertexProgramsInitialized)
    {
        s_bVertexProgramsInitialized = true;
        GLRender::InitExtensions();

        if (AurVertexProgramsAvailable() != 0)
        {
            ASLgl::glEnable(enableTranslationTable[9]);

            VertexProgram* pVP;

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinySkinVPARB
                                                              : cBumpyShinySkinVP,
                                    cBumpyShinySkinVPDeclaration);
            g_pBumpyShinySkinPass1 = pVP;
            pVP->Enable(0, false);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szSkinVPARB : cSkinVP,
                                    cBumpyShinySkinVPDeclaration);
            g_pBumpyShinySkinPass2 = pVP;
            pVP->Enable(0, true);

            pVP = new VertexProgram(cSkin3BoneVP, cSkin3BoneVPDeclaration);
            g_pBumpyShinySkinPass2_3bone = pVP;
            pVP->Enable(0, true);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                    cStandardVP2Declaration);
            g_pBumpyShinyPass1 = pVP;
            pVP->Enable(0, true);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                    cStandardVPDeclaration);
            g_pBumpyShinyPass2 = pVP;
            pVP->Enable(0, false);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                    cBumpyShinyRegularVPDeclaration);
            g_pBumpyShinyPass3 = pVP;
            pVP->Enable(0, true);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                    cBumpyShinyRegularVPDeclaration);
            g_pBumpyShinyPass4 = pVP;
            pVP->Enable(0, false);

            pVP = new VertexProgram(usearbvertexprograms != 0 ? szBumpyShinyVPARB : cBumpyShinyVP,
                                    cBumpyShinyLightMappedVPDeclaration);
            g_pBumpyShinyPass5 = pVP;
            pVP->Enable(0, false);

            ASLgl::glDisable(enableTranslationTable[9]);
        }
    }

    return 1;
}
#endif

// The largest mode no bigger than the request, preferring the highest refresh rate on a tie.
bool CAurInternal::GetValidMode(unsigned int* pWidth, unsigned int* pHeight,
                                unsigned char* pBitsPerPel, unsigned int* pFrequency, bool bSet)
{
#if KOTOR1
    PortDevMode devMode;
    __builtin_memset(&devMode, 0, sizeof(devMode));
    devMode.m_size = sizeof(devMode);
    PortDevMode best;
    __builtin_memset(&best, 0, sizeof(best));
    best.m_size = sizeof(best);

    if (!EnumDisplaySettingsA(0, 0, &devMode))
        return false;

    bool bFound = false;
    for (unsigned int i = 1; ; i++)
    {
        if (devMode.m_pelsWidth <= *pWidth && devMode.m_pelsHeight <= *pHeight
            && devMode.m_bitsPerPel <= *pBitsPerPel && devMode.m_displayFrequency <= *pFrequency
            && devMode.m_pelsWidth >= best.m_pelsWidth && devMode.m_pelsHeight >= best.m_pelsHeight
            && devMode.m_bitsPerPel >= best.m_bitsPerPel
            && (devMode.m_bitsPerPel != best.m_bitsPerPel || devMode.m_pelsHeight != best.m_pelsHeight
                || devMode.m_pelsWidth != best.m_pelsWidth
                || devMode.m_displayFrequency >= best.m_displayFrequency))
        {
            best = devMode;
            bFound = true;
        }

        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);
        if (!EnumDisplaySettingsA(0, i, &devMode))
            break;
    }

    if (!bFound)
        return false;
    if (ChangeDisplaySettingsA(&best, 2) != 0)
        return false;

    if (bSet)
    {
        *pWidth = best.m_pelsWidth;
        *pHeight = best.m_pelsHeight;
        *pBitsPerPel = static_cast<unsigned char>(best.m_bitsPerPel);
        *pFrequency = best.m_displayFrequency;
        return true;
    }

    return best.m_pelsWidth == *pWidth && best.m_pelsHeight == *pHeight
        && best.m_bitsPerPel == *pBitsPerPel && best.m_displayFrequency == *pFrequency;
#else
    PortDevMode devMode;
    __builtin_memset(&devMode, 0, sizeof(devMode));
    devMode.m_size = sizeof(devMode);

    if (!EnumDisplaySettingsA(0, 0, &devMode))
        return false;

    unsigned int bestWidth = 0;
    unsigned int bestHeight = 0;
    unsigned int bestBitsPerPel = 0;
    unsigned int bestFrequency = 0;
    bool bFound = false;

    for (unsigned int i = 1; ; i++)
    {
        if (devMode.m_pelsWidth <= *pWidth && devMode.m_pelsHeight <= *pHeight
            && devMode.m_bitsPerPel <= *pBitsPerPel)
        {
            bool bTake = false;
            if (*pFrequency == 0)
            {
                bTake = devMode.m_pelsWidth >= bestWidth && devMode.m_pelsHeight >= bestHeight
                     && devMode.m_bitsPerPel >= bestBitsPerPel;
            }
            else
            {
                bTake = devMode.m_bitsPerPel >= bestBitsPerPel
                     && devMode.m_pelsHeight >= bestHeight
                     && devMode.m_pelsWidth >= bestWidth
                     && devMode.m_displayFrequency <= *pFrequency;
            }

            if (bTake
                && (devMode.m_displayFrequency >= bestFrequency
                    || devMode.m_pelsWidth != bestWidth
                    || devMode.m_pelsHeight != bestHeight
                    || devMode.m_bitsPerPel != bestBitsPerPel))
            {
                bestWidth = devMode.m_pelsWidth;
                bestHeight = devMode.m_pelsHeight;
                bestBitsPerPel = devMode.m_bitsPerPel;
                bestFrequency = devMode.m_displayFrequency;
                bFound = true;
            }
        }

        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);
        if (!EnumDisplaySettingsA(0, i, &devMode))
            break;
    }

    if (!bFound)
        return false;

    if (bSet)
    {
        *pWidth = bestWidth;
        *pHeight = bestHeight;
        *pBitsPerPel = static_cast<unsigned char>(bestBitsPerPel);
        *pFrequency = bestFrequency;
        return true;
    }

    return bestWidth == *pWidth && bestHeight == *pHeight && bestBitsPerPel == *pBitsPerPel
        && bestFrequency == *pFrequency;
#endif
}
