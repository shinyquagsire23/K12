#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library
#endif
#include "aurglext.h"
#include "aurglcore.h"


int existingextensions;
unsigned int lookup;

int EXT_TEXTURE_ENV_COMBINE_BIT = 4;
int EXT_FOG_COORD_BIT = 32;
int EXT_SECONDARY_COLOR_BIT = 64;
int NV_REGISTER_COMBINERS2_BIT = 8388608;
int WGL_NV_RENDER_TO_TEXTURE_RECTANGLE_BIT = 33554432;
int WGL_ARB_MULTISAMPLE_BIT = 134217728;

int EXT_BGRA_BIT = 1;
int ARB_MULTITEXTURE_BIT = 2;
int NV_REGISTER_COMBINERS_BIT = 8;
int EXT_TEXTURE_CUBE_MAP_BIT = 16;
int NV_VERTEX_PROGRAMS_BIT = 128;
int EXT_TEXTURE_FILTER_ANISOTROPIC_BIT = 256;
int NV_PIXEL_SHADERS_BIT = 1024;
int NV_VERTEX_ARRAY_RANGE_BIT = 2048;
int NV_VERTEX_ARRAY_RANGE2_BIT = 4096;
int NV_FENCE_BIT = 8192;
int ARB_TEXTURE_COMPRESSION_BIT = 16384;
int EXT_TEXTURE_COMPRESSION_S3TC_BIT = 32768;
int ATIX_PN_TRIANGLES_BIT = 65536;
int EXT_COMPILED_VERTEX_ARRAY_BIT = 131072;
int WGL_ARB_RENDER_TO_TEXTURE_BIT = 262144;
int EXT_SGIS_MIPMAP_BIT = 524288;
int ATI_FRAGMENT_SHADER_BIT = 1048576;
int GL_ARB_VERTEX_PROGRAM_BIT = 2097152;
int GL_ARB_VERTEX_BUFFER_OBJECT_BIT = 4194304;
int GL_STENCIL_TWO_SIDED_BIT = 16777216;
int WGL_ARB_BUFFER_REGION_BIT = 67108864;
int EXT_SWAP_CONTROL_BIT = 268435456;
int GL_ARB_DEPTH_TEXTURE_BIT = 536870912;
int GL_ARB_FRAGMENT_PROGRAM_BIT = 1073741824;
int useatifragmentshaders = 1;
int renderlevel = 2;

int g_nUnlockATISoftShadowsForHotFix = 1;

#include "port/libcstring.h"

int AurCompiledVertexArraysAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (EXT_COMPILED_VERTEX_ARRAY_BIT & existingextensions)
                    == EXT_COMPILED_VERTEX_ARRAY_BIT;
    return returnval;
}

int AurPBufferAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (WGL_ARB_RENDER_TO_TEXTURE_BIT & existingextensions)
                    == WGL_ARB_RENDER_TO_TEXTURE_BIT;
    return returnval;
}

int AurTruformAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (ATIX_PN_TRIANGLES_BIT & existingextensions) == ATIX_PN_TRIANGLES_BIT;
    return returnval;
}

int AurATIFragmentShaderAvailable()
{
    return ((ATI_FRAGMENT_SHADER_BIT & existingextensions) == ATI_FRAGMENT_SHADER_BIT)
           & useatifragmentshaders;
}

int AurBumpMapAvailable()
{
    if (((EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT | EXT_TEXTURE_CUBE_MAP_BIT
          | GL_ARB_VERTEX_PROGRAM_BIT) & existingextensions)
        == (EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT | EXT_TEXTURE_CUBE_MAP_BIT
            | GL_ARB_VERTEX_PROGRAM_BIT))
        return 1;
    if (!AurVertexProgramsARBAvailable())
        return 0;
    return AurATIFragmentShaderAvailable();
}

// Same feature-set test as AurBumpMapAvailable, gated on the render level.
bool AurNV1xLevelShadersAvailable()
{
    if (renderlevel < 1)
        return 0;
    int bATI = AurATIFragmentShaderAvailable();
    if (renderlevel == 1 && bATI)
        return 0;
    if (((EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT | EXT_TEXTURE_CUBE_MAP_BIT
          | GL_ARB_VERTEX_PROGRAM_BIT) & existingextensions)
        == (EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT | EXT_TEXTURE_CUBE_MAP_BIT
            | GL_ARB_VERTEX_PROGRAM_BIT))
        return 1;
    return (bATI != 0) & (AurVertexProgramsARBAvailable() != 0);
}

bool AurNV2xLevelShadersAvailable()
{
    if (renderlevel < 2)
        return 0;
    return AurBumpyShinyAvailable() != 0;
}

// EFFECTIVE (K1+K2): ATI_FRAGMENT_SHADER_BIT's GOT load issues before the `| GL_ARB_VERTEX_PROGRAM_BIT` in
// the orig and after it in ours (registers rename from there); same loads and logic, armv7 exact in both games.
int AurBumpyShinyAvailable()
{
    static int returnval = -1;
    if (returnval == -1) {
        returnval = ((EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT
                      | EXT_TEXTURE_CUBE_MAP_BIT | NV_PIXEL_SHADERS_BIT | GL_ARB_VERTEX_PROGRAM_BIT)
                     & existingextensions)
                    == (EXT_BGRA_BIT | ARB_MULTITEXTURE_BIT | NV_REGISTER_COMBINERS_BIT
                        | EXT_TEXTURE_CUBE_MAP_BIT | NV_PIXEL_SHADERS_BIT | GL_ARB_VERTEX_PROGRAM_BIT);
        if (!returnval)
            returnval = (((ARB_MULTITEXTURE_BIT | EXT_BGRA_BIT | EXT_TEXTURE_CUBE_MAP_BIT
                          | GL_ARB_VERTEX_PROGRAM_BIT | ATI_FRAGMENT_SHADER_BIT) & existingextensions)
                        == (ARB_MULTITEXTURE_BIT | EXT_BGRA_BIT | EXT_TEXTURE_CUBE_MAP_BIT
                            | GL_ARB_VERTEX_PROGRAM_BIT | ATI_FRAGMENT_SHADER_BIT))
                        & useatifragmentshaders & usearbvertexprograms;
    }
    return returnval;
}

bool AurSwapControlAvailable()
{
    return (EXT_SWAP_CONTROL_BIT & existingextensions) == EXT_SWAP_CONTROL_BIT;
}

// Tests the card rather than the driver. The result is inverted: 1 means the card is rejected.
bool AurBumpMapCardValidate()
{
#if KOTOR1  // K1: the card is never rejected
    return 0;
#else
    static int returnval = -1;
    if (returnval == -1)
        returnval = !(((ATI_FRAGMENT_SHADER_BIT & existingextensions) == ATI_FRAGMENT_SHADER_BIT)
                      & useatifragmentshaders);
    return returnval == 1;
#endif
}

// The Radeon 8500 needs the depth-texture bit.
bool AurNonRadeon8500Validate()
{
    static int returnval = -1;
    if (returnval == -1) {
        if (AurATIFragmentShaderAvailable() && !(existingextensions & GL_ARB_DEPTH_TEXTURE_BIT))
            returnval = 0;
        else
            returnval = 1;
    }
    return returnval == 1;
}

bool AurATIValidate()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = AurATIFragmentShaderAvailable();
    return returnval == 1;
}

// The only query here that asks the driver instead of the extension word.
int AurOnePassEnvMapAvailable()
{
    static int returnval = -1;
    if (returnval == -1) {
        if (renderlevel < 1)
            returnval = 0;
        else {
            int bATI = AurATIFragmentShaderAvailable();
            if (renderlevel == 1 && bATI)
                returnval = 0;
            else if (((ARB_MULTITEXTURE_BIT | EXT_BGRA_BIT | NV_REGISTER_COMBINERS_BIT
                       | EXT_TEXTURE_CUBE_MAP_BIT | GL_ARB_VERTEX_PROGRAM_BIT) & existingextensions)
                     == (ARB_MULTITEXTURE_BIT | EXT_BGRA_BIT | NV_REGISTER_COMBINERS_BIT
                         | EXT_TEXTURE_CUBE_MAP_BIT | GL_ARB_VERTEX_PROGRAM_BIT)) {
                if (bATI)
                    returnval = AurNonRadeon8500Validate();
                else {
                    GLRender::QueryIntegerv(Aurora::kQueryUnmapped, &returnval);
                    returnval = returnval > 1;
                }
            } else {
                int bARB = AurVertexProgramsARBAvailable();
                if (bATI && bARB)
                    returnval = AurNonRadeon8500Validate();
                else
                    returnval = 0;
            }
        }
    }
    return returnval;
}

int AurVertexProgramsARBAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (GL_ARB_VERTEX_PROGRAM_BIT & existingextensions) == GL_ARB_VERTEX_PROGRAM_BIT;
    return returnval & usearbvertexprograms;
}

int AurMultiTextureAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (ARB_MULTITEXTURE_BIT & existingextensions) == ARB_MULTITEXTURE_BIT;
    return returnval;
}

int AurVertexProgramsNVAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (NV_VERTEX_PROGRAMS_BIT & existingextensions) == NV_VERTEX_PROGRAMS_BIT;
    return returnval;
}

int AurVertexBufferObjectARBAvailable()
{
#if KOTOR1  // K1: always available, no disable switch and no extension test
    return 1;
#else
    if (g_bDisableVertexBufferObjects)
        return 0;
    static int returnval = -1;
    if (returnval == -1)
        returnval = (GL_ARB_VERTEX_BUFFER_OBJECT_BIT & existingextensions)
                    == GL_ARB_VERTEX_BUFFER_OBJECT_BIT;
    return returnval;
#endif
}

int AurVertexProgramsAvailable()
{
    return (AurVertexProgramsARBAvailable() & usearbvertexprograms) != 0;
}

// Stub: always unavailable.
int AurSoftShadowsAvailable()
{
#ifdef K2_QOL_SOFT_SHADOWS
    // The port's image-space pass (aurglcore.cpp, QolSoftShadowsSetUp) runs on the program layer.
    return AuroraUseARBShaders() != 0;
#else
    static int returnval = -1;
    if (returnval == -1)
        returnval = 0;
    return returnval;
#endif
}

int AurATISoftShadowsAvailable()
{
    if (!g_nUnlockATISoftShadowsForHotFix)
        return 0;
    static int returnval = -1;
#if KOTOR1  // K1: no extension test, the latch answers 0
    if (returnval == -1)
        returnval = 0;
#else
    if (returnval == -1)
        returnval = ((ATI_FRAGMENT_SHADER_BIT | WGL_ARB_RENDER_TO_TEXTURE_BIT)
                     & existingextensions)
                    == (ATI_FRAGMENT_SHADER_BIT | WGL_ARB_RENDER_TO_TEXTURE_BIT);
#endif
    return returnval;
}

// Stub: always unavailable.
int AurFrameBufferModificationsAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = 0;
    return returnval;
}

int AurATIFrameBufferModificationsAvailable()
{
    static int returnval = -1;
    if (returnval == -1) {
        returnval = ((WGL_ARB_RENDER_TO_TEXTURE_BIT | ATI_FRAGMENT_SHADER_BIT
                      | GL_ARB_VERTEX_PROGRAM_BIT) & existingextensions)
                    == (WGL_ARB_RENDER_TO_TEXTURE_BIT | ATI_FRAGMENT_SHADER_BIT
                        | GL_ARB_VERTEX_PROGRAM_BIT);
        if (!AurNonRadeon8500Validate())
            returnval = 0;
    }
    return returnval;
}

int AurCubeMapAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = ((EXT_TEXTURE_CUBE_MAP_BIT | ARB_MULTITEXTURE_BIT) & existingextensions)
                    == (EXT_TEXTURE_CUBE_MAP_BIT | ARB_MULTITEXTURE_BIT);
    return returnval;
}

int AurAnisotropicFilterAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (EXT_TEXTURE_FILTER_ANISOTROPIC_BIT & existingextensions)
                    == EXT_TEXTURE_FILTER_ANISOTROPIC_BIT;
    return returnval;
}

int AurRangeFenceAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = ((NV_VERTEX_ARRAY_RANGE_BIT | NV_VERTEX_ARRAY_RANGE2_BIT | NV_FENCE_BIT)
                     & existingextensions)
                    == (NV_VERTEX_ARRAY_RANGE_BIT | NV_VERTEX_ARRAY_RANGE2_BIT | NV_FENCE_BIT);
    return returnval;
}

// ATI fragment shaders are preferred, so their presence answers 0 without consulting the SGI bit.
int AurSGIMipmapAvailable()
{
    static int returnval = -1;
    if (returnval == -1) {
        if (ATI_FRAGMENT_SHADER_BIT & existingextensions)
            returnval = 0;
        else
            returnval = (EXT_SGIS_MIPMAP_BIT & existingextensions) == EXT_SGIS_MIPMAP_BIT;
    }
    return returnval;
}

int AurTextureCompressionAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = ((ARB_TEXTURE_COMPRESSION_BIT | EXT_TEXTURE_COMPRESSION_S3TC_BIT)
                     & existingextensions)
                    == (ARB_TEXTURE_COMPRESSION_BIT | EXT_TEXTURE_COMPRESSION_S3TC_BIT);
    return returnval;
}

// The driver's extension string is space-separated; this walks it token by token. Only the token's
// length and bytes are compared, so "GL_NV_fence" never matches a prefix of "GL_NV_fence_foo".
char CheckExtension(char* pszName, const unsigned char* pszExtensions)
{
    int nNameLen = static_cast<int>(strlen(pszName));
    const char* psz = reinterpret_cast<const char*>(pszExtensions);
    const char* pszEnd = psz + strlen(psz);
    if (psz >= pszEnd)
        return 0;
    while (psz < pszEnd) {
        int nLen = static_cast<int>(strcspn(psz, " "));
        if (nNameLen == nLen && strncmp(pszName, psz, nLen) == 0)
            return 1;
        psz += nLen + 1;
    }
    return 0;
}

int AurGetAvailableStencilBits()
{
    static int stencilbits = -1;
    if (stencilbits == -1)
        GLRender::GetStencilBits(&stencilbits);
    return stencilbits;
}

int AurGetStencilClearValue()
{
    int nBits = AurGetAvailableStencilBits();
    if (!nBits)
        return 0;
    static int stencilclear = -1;
    if (stencilclear == -1)
        stencilclear = (1 << (nBits - 1)) - 1;
    return stencilclear;
}

int AurTwoSidedStencilAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (GL_STENCIL_TWO_SIDED_BIT & existingextensions) != 0;
    return returnval;
}

int AurBufferRegionAvailable()
{
    static int returnval = -1;
    if (returnval == -1)
        returnval = (WGL_ARB_BUFFER_REGION_BIT & existingextensions) != 0;
    return returnval;
}

// The cached stencil answer if it is set, otherwise the ATI path.
bool AurCheckSoftShadowsAvailable()
{
    if (AurSoftShadowsAvailable())
        return 1;
    return AurATISoftShadowsAvailable() != 0;
}

bool AurCheckFrameBufferModificationsAvailable()
{
#if KOTOR1 && defined(K2_QOL_K1_DISTORTION)
    // The port's force-distortion pass runs on the scene pbuffer the ARB-shader setup builds.
    return AuroraUseARBShaders() != 0;
#elif KOTOR1  // K1: never available
    return 0;
#elif defined(K2_QOL_FRAME_BUFFER_EFFECTS)
    // The switch gates the bloom and the Force Speed blur of the post-process that always runs
    // (aurglcore.cpp FrameBufferModificationsEndIos).
    return AuroraUseARBShaders() != 0;
#else
    if (AurFrameBufferModificationsAvailable())
        return 1;
    return AurATIFrameBufferModificationsAvailable() != 0;
#endif
}

bool AuroraUseARBShaders()
{
    return (GL_ARB_FRAGMENT_PROGRAM_BIT & existingextensions) == GL_ARB_FRAGMENT_PROGRAM_BIT;
}
