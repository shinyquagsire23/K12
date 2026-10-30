#ifndef KOTORPROGRAM_H
#define KOTORPROGRAM_H

// The OpenGL ES shader-program wrapper, holding the program object, its uniform locations, and the
// caches that decide whether a block of values has changed since the last upload. Not related to
// VertexProgram, the ARB/NV vertex-program wrapper in libsource/aurora.

// The attribute-slot selector; the slots are index-named.

namespace KotorProgramLocations {
enum Location {
    LOC_0 = 0,
    LOC_1,
    LOC_2,
    LOC_3,
    LOC_4,
    LOC_5,
    LOC_6,
    LOC_COUNT
};
}

// One vertex attribute's description, as fed to glVertexAttribPointer. m_nSize is not only the
// pointer path's size: the non-pointer path switches on it (1..4 selects glVertexAttrib[1-4]f).
struct AttributeData {
    int           m_nSize;
    int           m_nType;
    int           m_nStride;
    const void*   m_pPointer;
    bool          m_bIsPointer;
};

// The program's identity and flags, produced by an enum switch over an int code. A trivially
// copyable 4-byte class: the constructor takes it by value and KotorProgram stores it directly.
class ProgramType {
public:
    // m_nFlags: the features the program's shaders are built with, each a USE_* define in the
    // source, and the shader pair it picks.
    enum {
        SHADER_SKIN = 0x1,
        SHADER_LIGHTMAP = 0x2,
        SHADER_BUMPMAP = 0x4,
        SHADER_CUBEMAP = 0x8,
        SHADER_POST1 = 0x10,              // kotorpost1.vert and .frag
        SHADER_ALPHATEST = 0x20,
        SHADER_FLAT_CUBEMAP = 0x40,
        SHADER_BLOOM = 0x80,              // kotorbloom.vert and .frag
        SHADER_HOLO = 0x100,
        SHADER_DESAT = 0x200,
        SHADER_FORCEGLOW = 0x400,
        SHADER_ILLUM = 0x800,
        SHADER_DISTORTION_INPUT = 0x1000,
        SHADER_POST2 = 0x2000,
        SHADER_SOFT_SHADOW_COPY = 0x4000, // the port's soft-shadow programs, on kotorpost1.vert
        SHADER_SOFT_SHADOW_BLUR = 0x8000,
#ifdef K2_QOL_BUMPMAPS
        // The port's translations of the PC's per-pixel passes, which the port's shaders lacked: the
        // bump light (kotorbump.vert / .frag), the bump-perturbed reflection (the same, USE_BUMPYSHINY),
        // the fog laid over a pass drawn unfogged (USE_FOGPASS) and the noise ("dirt") modes
        // (kotornoise.vert / .frag), their formula in bits 20..21.
        SHADER_QOL_BUMP = 0x10000,
        SHADER_QOL_BUMPYSHINY = 0x20000,
        SHADER_QOL_FOG = 0x40000,
        SHADER_QOL_NOISE = 0x80000,
        SHADER_QOL_NOISE_FORMULA_SHIFT = 20,
        SHADER_QOL_NOISE_LOCAL = 0x400000,   // the noise in model space, not world space
#endif
    };
    // The program types: the fragment-program ids the engine binds, KOTOR II's GLRender::PixelShader
    // modes of the same number (BASE_LIGHTMAP is the mode between DEFAULT and POST_BLOOM_SOURCE that
    // PixelShader leaves unnamed, KOTOR 1's kPSBaseLightmap).
    enum {
        PROGRAM_NONE                  = 0,
        PROGRAM_FOG                   = 2,
        PROGRAM_STANDARD_FOG          = 3,
        PROGRAM_BUMPMAP               = 6,
        PROGRAM_NORMALMAP             = 7,
        PROGRAM_BUMPMAP_OFF           = 13,
        PROGRAM_ENV_MAP_CUBE          = 19,
        PROGRAM_ENV_MAP_FOG           = 20,
        PROGRAM_ENV_MAP_LIGHTMAP_NV1X = 21,
        PROGRAM_ENV_MAP_LIGHTMAP      = 22,
        PROGRAM_BUMPY_SHINY           = 24,
        PROGRAM_LIGHTMAP              = 26,
        PROGRAM_BUMPY_SHINY_OFF       = 28,
        PROGRAM_BUMPY_SHINY_TEXTURES  = 29,
        PROGRAM_MULTI_TEXTURE_OFF     = 32,
        PROGRAM_ENV_MAP_LIGHTMAP_OFF  = 37,
        PROGRAM_FGLOW                 = 50,
        PROGRAM_FGLOW_LM              = 51,
        PROGRAM_FGLOW_EM              = 52,
        PROGRAM_FGLOW_EM_LM           = 53,
        PROGRAM_FGLOW_BUMPMAP         = 54,
        PROGRAM_FGLOW_BUMPY_SHINY     = 55,
        PROGRAM_DESAT                 = 56,
        PROGRAM_DESAT_LM              = 57,
        PROGRAM_DESAT_EM              = 58,
        PROGRAM_DESAT_EM_LM           = 59,
        PROGRAM_DESAT_BUMPMAP         = 60,
        PROGRAM_DESAT_BUMPY_SHINY     = 61,
        PROGRAM_HOLO                  = 75,
        PROGRAM_HOLO_DESAT            = 76,
        PROGRAM_ILLUM                 = 78,
        PROGRAM_ILLUM_EM              = 79,
        PROGRAM_DEFAULT               = 110,
        PROGRAM_BASE_LIGHTMAP         = 111,
        PROGRAM_POST_BLOOM_SOURCE     = 112,
        PROGRAM_POST_PRESENT          = 113,
        PROGRAM_POST_DISTORT          = 114,
        PROGRAM_POST_BLUR             = 115,
        PROGRAM_ENV_MAP               = 117,
    };
    // The port's soft-shadow program types (aurglcore.cpp's QolSoftShadowsFinish).
    enum {
        PROGRAM_SOFT_SHADOW_COPY = 0x102,
        PROGRAM_SOFT_SHADOW_BLUR = 0x103,
    };
#ifdef K2_QOL_BUMPMAPS
    // A noise mode's id with this bit: its noise in model space (auraticore.cpp EnableNoiseShader).
    enum { PROGRAM_QOL_NOISE_LOCAL = 0x1000 };
#endif

    ProgramType(int nType, bool bA, bool bB, bool bC, bool bD);

    int m_nFlags;
};

// The "has this block of floats changed?" cache.
template <int N>
class KotorProgramCachedValue {
public:
    // -1 never matches a real upload, so the first set always goes through.
    KotorProgramCachedValue()
    {
        for (int i = 0; i < N; i++)
            m_afValues[i] = -1.0f;
    }

    bool IsDifferentAndCache(float* pfValues);

    float m_afValues[N];
};

class KotorProgram {
public:
    // The vec4 counts of the light-data and skin-matrix uniform arrays.
    enum {
        LIGHT_DATA_VEC4S = 15,
        SKIN_VEC4S = 51,
    };

    KotorProgram(ProgramType type);
    void Use();
    void Unuse();

    void SetAttributeShadowOnly(AttributeData& attrA, AttributeData& attrB);
    void SetAttributeToLoc(AttributeData& attr, KotorProgramLocations::Location loc,
                           unsigned char bNormalized);
    void SetAttributes(AttributeData& a, AttributeData& b, AttributeData& c, bool bEnabled,
                       AttributeData& e, AttributeData& f, AttributeData& g,
                       AttributeData& h);
    void SetMatrix(float* pfMatrix);
    void SetCamera(float* pfMatrix);
    void SetSkinMatrices(float* pfMatrices);
    void SetLightData(float* pfData);
    void SetFogData(float* pfData, float* pfColour);
    void SetLightMaterialData(float* pfData);
    void SetFragmentShaderParams(float* pfParams);
    void SetVertexShaderParams(float* pfParams);
#ifdef K2_QOL_BUMPMAPS
    void SetBumpAttributes(AttributeData* pFrame);
    void SetBumpParams(float* pfParams);
#endif

    int         m_nCameraSerial;
    int         m_nFogSerial;
    KotorProgramCachedValue<LIGHT_DATA_VEC4S * 4> m_lightDataCache;
    KotorProgramCachedValue<3> m_fogColourCache;
    KotorProgramCachedValue<4> m_fogDataCache;
    KotorProgramCachedValue<16> m_lightMaterialCache;
    KotorProgramCachedValue<12> m_fragmentCache;
    KotorProgramCachedValue<4> m_vertexShaderCache;
    unsigned int m_uProgram;
    unsigned char m_abAttributeQueried[7];
    unsigned char m_abSamplerQueried[4];
    unsigned char m_abUniformQueried[10];
    int         m_anLocations[7];
    int         m_anSamplerUniforms[4];
    int         m_nMatrixUniform;
    int         m_nCameraUniform;
    int         m_nSkinUniform;
    int         m_nLightDataUniform;
    int         m_nFogColourUniform;
    int         m_nFogDataUniform;
    int         m_nLightMaterialUniform;
    int         m_nFragmentShaderUniform;
    int         m_nVertexShaderUniform;
    int         m_nPreLitColorsUniform;
    int         m_nFlags;
#ifdef K2_QOL_BUMPMAPS
    int         m_anBumpLocations[3];   // a_bumpT / a_bumpB / a_bumpN: the tangent frame's rows
    int         m_nBumpUniform;         // u_bumpParams: the bump light, the camera, the attenuation
    KotorProgramCachedValue<12> m_bumpCache;
#endif
};

template <int N>
bool KotorProgramCachedValue<N>::IsDifferentAndCache(float* pfValues)
{
#ifdef K2_HOST_BUILD
    // With K2_GL_NOCACHE set, every upload goes through, as a test of the cache.
    extern bool k2shim_gl_nocache;
    if (k2shim_gl_nocache) {
        for (int j = 0; j < N; j++)
            m_afValues[j] = pfValues[j];
        return true;
    }
#endif
    for (int i = 0; i < N; i++) {
        if (m_afValues[i] != pfValues[i]) {
            for (int j = 0; j < N; j++)
                m_afValues[j] = pfValues[j];
            return true;
        }
    }
    return false;
}

#endif
