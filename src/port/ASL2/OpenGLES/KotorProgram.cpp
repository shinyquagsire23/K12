#include "k2stdint.h"
#include "port/aslglapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include <stddef.h>

#include "KotorProgram.h"
#ifdef K2_HOST_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

// glCreateShader's shader types (port/glconst.h does not carry them; the values are gl2.h's).
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#endif

// The shader source buffer: the USE_* defines and the shader file.
enum { SHADER_SOURCE_SIZE = 128 * 1024 };

static unsigned int CompileShader(const char* pszPath, const char* pszHeader, unsigned int nType);
#ifdef K2_QOL_SOFT_SHADOWS
static unsigned int CompileShaderText(const char* pszText, const char* pszHeader, unsigned int nType);

// The port's soft-shadow programs (aurglcore.cpp QolSoftShadowsFinish), the ids and fragment shaders
// K1's k1kotorgl.cpp gives them: 0x102 copies unit 0 (flag 0x4000), 0x103 blurs the 512x512 shadow
// mask (flag 0x8000) -- the PC's BloomFilterARB with the soft-shadow program: the 5x5 bloom kernel
// times softshadowdarkeningfactor (0.4), offsets -2..2 texels, walked in the PC's seven 4-tap
// passes, each s = sum(w * tap), added with glBlendFunc(GL_SRC_ALPHA, GL_ONE) -- so sum(s * s.a).
// Both on kotorpost1.vert.
static const char kCopyFragment[] =
    "precision mediump float;\n"
    "uniform sampler2D u_texture0Sampler;\n"
    "varying mediump vec4 v_texCoord0;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(u_texture0Sampler, v_texCoord0.st);\n"
    "}\n";

static const char kSoftShadowFragment[] =
    "precision mediump float;\n"
    "uniform sampler2D u_texture0Sampler;\n"
    "varying mediump vec4 v_texCoord0;\n"
    "vec4 T(float u, float v)\n"
    "{\n"
    "    return texture2D(u_texture0Sampler, v_texCoord0.st + vec2(u, v) * (1.0 / 512.0));\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec4 s;\n"
    "    vec4 r = vec4(0.0);\n"
    "    s = 0.048 * T(-2.0, -2.0) + 0.100 * T(-2.0, -1.0) + 0.116 * T(-2.0, 0.0) + 0.100 * T(-2.0, 1.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.048 * T(2.0, -2.0) + 0.100 * T(-1.0, -2.0) + 0.196 * T(-1.0, -1.0) + 0.232 * T(-1.0, 0.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.196 * T(1.0, -1.0) + 0.100 * T(-1.0, 2.0) + 0.116 * T(0.0, -2.0) + 0.232 * T(0.0, -1.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.268 * T(0.0, 0.0) + 0.232 * T(0.0, 1.0) + 0.116 * T(0.0, 2.0) + 0.100 * T(1.0, -2.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.196 * T(-1.0, 1.0) + 0.232 * T(1.0, 0.0) + 0.196 * T(1.0, 1.0) + 0.100 * T(1.0, 2.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.048 * T(-2.0, 2.0) + 0.100 * T(2.0, -1.0) + 0.116 * T(2.0, 0.0) + 0.100 * T(2.0, 1.0);\n"
    "    r += s * s.a;\n"
    "    s = 0.048 * T(2.0, 2.0);\n"
    "    r += s * s.a;\n"
    "    gl_FragColor = r;\n"
    "}\n";
#endif

#ifdef K2_QOL_BUMPMAPS
// The program types the port's translations of the PC's per-pixel passes take over (0: none): the bump
// light, the bump-perturbed reflection and their force-sight variants, the fog pass, and the noise
// modes -- four families of twelve (plain, desaturated, illuminated, both), each the plain, env-mapped,
// lightmapped and env-mapped lightmapped arms in subtract, add and multiply.
static int QolProgramFlags(int nType)
{
    int nLocal = 0;
    if (nType & ProgramType::PROGRAM_QOL_NOISE_LOCAL) {
        nType &= ~ProgramType::PROGRAM_QOL_NOISE_LOCAL;
        nLocal = ProgramType::SHADER_QOL_NOISE_LOCAL;
    }
    switch (nType) {
    case ProgramType::PROGRAM_FOG:
        return ProgramType::SHADER_QOL_FOG;
    case ProgramType::PROGRAM_BUMPMAP:
        return ProgramType::SHADER_QOL_BUMP;
    case ProgramType::PROGRAM_FGLOW_BUMPMAP:
        return ProgramType::SHADER_QOL_BUMP | ProgramType::SHADER_DESAT | ProgramType::SHADER_FORCEGLOW;
    case ProgramType::PROGRAM_DESAT_BUMPMAP:
        return ProgramType::SHADER_QOL_BUMP | ProgramType::SHADER_DESAT;
    case ProgramType::PROGRAM_BUMPY_SHINY:
        return ProgramType::SHADER_QOL_BUMPYSHINY;
    case ProgramType::PROGRAM_FGLOW_BUMPY_SHINY:
        return ProgramType::SHADER_QOL_BUMPYSHINY | ProgramType::SHADER_DESAT | ProgramType::SHADER_FORCEGLOW;
    case ProgramType::PROGRAM_DESAT_BUMPY_SHINY:
        return ProgramType::SHADER_QOL_BUMPYSHINY | ProgramType::SHADER_DESAT;
    default:
        break;
    }
    static const int kNoiseFamilies[4] = { 38, 62, 80, 98 };   // kPSNoise / DesatNoise / IllumNoise / IllumDesatNoise
    for (int i = 0; i < 4; i++) {
        int n = nType - kNoiseFamilies[i];
        if (n < 0 || n >= 12)
            continue;
        int nFlags = ProgramType::SHADER_QOL_NOISE | nLocal | ((n % 3) << ProgramType::SHADER_QOL_NOISE_FORMULA_SHIFT);
        if ((n / 3) & 1)
            nFlags |= ProgramType::SHADER_CUBEMAP;
        if ((n / 3) & 2)
            nFlags |= ProgramType::SHADER_LIGHTMAP;
        if (i & 1)
            nFlags |= ProgramType::SHADER_DESAT;
        if (i & 2)
            nFlags |= ProgramType::SHADER_ILLUM;
        return nFlags;
    }
    return 0;
}
#endif

ProgramType::ProgramType(int nType, bool bA, bool bB, bool bC, bool bD)
{
    m_nFlags = 0;

#ifdef K2_QOL_BUMPMAPS
    int nQolFlags = QolProgramFlags(nType);
    if (nQolFlags != 0)
        m_nFlags = nQolFlags;
    else
#endif
    switch (nType) {
    case PROGRAM_NONE: case PROGRAM_FOG: case PROGRAM_STANDARD_FOG: case PROGRAM_NORMALMAP:
    case PROGRAM_BUMPMAP_OFF: case PROGRAM_ENV_MAP_LIGHTMAP_NV1X: case PROGRAM_BUMPY_SHINY_OFF:
    case PROGRAM_MULTI_TEXTURE_OFF: case PROGRAM_ENV_MAP_LIGHTMAP_OFF: case PROGRAM_DEFAULT:
        break;
    case PROGRAM_BUMPMAP:
        m_nFlags = SHADER_BUMPMAP;
        break;
    case PROGRAM_ENV_MAP_CUBE: case PROGRAM_ENV_MAP_FOG:
        m_nFlags = SHADER_CUBEMAP;
        break;
    case PROGRAM_ENV_MAP_LIGHTMAP: case PROGRAM_BASE_LIGHTMAP:
        m_nFlags = SHADER_LIGHTMAP | SHADER_CUBEMAP;
        break;
    case PROGRAM_BUMPY_SHINY: case PROGRAM_BUMPY_SHINY_TEXTURES:
        m_nFlags = SHADER_BUMPMAP | SHADER_CUBEMAP;
        break;
    case PROGRAM_LIGHTMAP:
        m_nFlags = SHADER_LIGHTMAP;
        break;
    case PROGRAM_FGLOW:
        m_nFlags = SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_FGLOW_LM:
        m_nFlags = SHADER_LIGHTMAP | SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_FGLOW_EM:
        m_nFlags = SHADER_CUBEMAP | SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_FGLOW_EM_LM:
        m_nFlags = SHADER_LIGHTMAP | SHADER_CUBEMAP | SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_FGLOW_BUMPMAP:
        m_nFlags = SHADER_BUMPMAP | SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_FGLOW_BUMPY_SHINY:
        m_nFlags = SHADER_BUMPMAP | SHADER_CUBEMAP | SHADER_DESAT | SHADER_FORCEGLOW;
        break;
    case PROGRAM_DESAT:
        m_nFlags = SHADER_DESAT;
        break;
    case PROGRAM_DESAT_LM:
        m_nFlags = SHADER_LIGHTMAP | SHADER_DESAT;
        break;
    case PROGRAM_DESAT_EM:
        m_nFlags = SHADER_CUBEMAP | SHADER_DESAT;
        break;
    case PROGRAM_DESAT_EM_LM:
        m_nFlags = SHADER_LIGHTMAP | SHADER_CUBEMAP | SHADER_DESAT;
        break;
    case PROGRAM_DESAT_BUMPMAP:
        m_nFlags = SHADER_BUMPMAP | SHADER_DESAT;
        break;
    case PROGRAM_DESAT_BUMPY_SHINY:
        m_nFlags = SHADER_BUMPMAP | SHADER_CUBEMAP | SHADER_DESAT;
        break;
    case PROGRAM_HOLO: case PROGRAM_HOLO_DESAT:
        m_nFlags = SHADER_HOLO;
        break;
    case PROGRAM_ILLUM:
        m_nFlags = SHADER_ILLUM;
        break;
    case PROGRAM_ILLUM_EM:
        m_nFlags = SHADER_CUBEMAP | SHADER_ILLUM;
        break;
    case PROGRAM_POST_PRESENT:
        bD = true;
        // falls through
    case PROGRAM_POST_BLOOM_SOURCE:
        m_nFlags = SHADER_POST1;
        break;
    case PROGRAM_POST_DISTORT:
        m_nFlags = SHADER_POST1 | SHADER_POST2;
        break;
    case PROGRAM_POST_BLUR:
        m_nFlags = SHADER_BLOOM;
        break;
    case PROGRAM_ENV_MAP:
        m_nFlags = SHADER_CUBEMAP | SHADER_FLAT_CUBEMAP;
        break;
#ifdef K2_QOL_SOFT_SHADOWS
    case PROGRAM_SOFT_SHADOW_COPY:
        m_nFlags = SHADER_SOFT_SHADOW_COPY;
        break;
    case PROGRAM_SOFT_SHADOW_BLUR:
        m_nFlags = SHADER_SOFT_SHADOW_BLUR;
        break;
#endif
    default:
        puts("ERROR SHADER");
        break;
    }

    if (bC)
        m_nFlags |= SHADER_ILLUM;
    if (bA)
        m_nFlags |= SHADER_SKIN;
    if (bB)
        m_nFlags |= SHADER_ALPHATEST;
    if (bD)
        m_nFlags |= SHADER_DISTORTION_INPUT;
}

KotorProgram::KotorProgram(ProgramType type) : m_nCameraSerial(0), m_nFogSerial(0), m_uProgram(0)
{
    m_nFlags = type.m_nFlags;
    Use();
}

void KotorProgram::Use()
{
    if (m_uProgram == 0) {
        int nStatus;
        int nLogLength;
        char szSource[SHADER_SOURCE_SIZE];

        memset(szSource, 0, sizeof(szSource));

        int nFlags = m_nFlags;

        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_SKIN) ? "#define USE_SKIN 1\n" : "#define USE_SKIN 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_ALPHATEST) ? "#define USE_ALPHATEST 1\n" : "#define USE_ALPHATEST 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_LIGHTMAP) ? "#define USE_LIGHTMAP 1\n" : "#define USE_LIGHTMAP 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_FLAT_CUBEMAP) ? "#define USE_FLAT_CUBEMAP 1\n" : "#define USE_FLAT_CUBEMAP 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_CUBEMAP) ? "#define USE_CUBEMAP 1\n" : "#define USE_CUBEMAP 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_BUMPMAP) ? "#define USE_BUMPMAP 1\n" : "#define USE_BUMPMAP 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_HOLO) ? "#define USE_HOLO 1\n" : "#define USE_HOLO 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_DESAT) ? "#define USE_DESAT 1\n" : "#define USE_DESAT 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_FORCEGLOW) ? "#define USE_FORCEGLOW 1\n" : "#define USE_FORCEGLOW 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_ILLUM) ? "#define USE_ILLUM 1\n" : "#define USE_ILLUM 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_DISTORTION_INPUT) ? "#define USE_DISTORTION_INPUT 1\n" : "#define USE_DISTORTION_INPUT 0\n", sizeof(szSource));
        __strcat_chk(szSource, (nFlags & ProgramType::SHADER_POST2) ? "#define USE_POST2 1\n" : "#define USE_POST2 0\n", sizeof(szSource));
        __strcat_chk(szSource, "#define USE_ANDROID 1\n", sizeof(szSource));
#ifdef K2_QOL_BUMPMAPS
        {
            char szQol[160];
            snprintf(szQol, sizeof(szQol),
                     "#define USE_BUMPYSHINY %d\n#define USE_FOGPASS %d\n#define NOISE_FORMULA %d\n#define NOISE_LOCAL %d\n",
                     (nFlags & ProgramType::SHADER_QOL_BUMPYSHINY) != 0, (nFlags & ProgramType::SHADER_QOL_FOG) != 0,
                     (nFlags >> ProgramType::SHADER_QOL_NOISE_FORMULA_SHIFT) & 3,
                     (nFlags & ProgramType::SHADER_QOL_NOISE_LOCAL) != 0);
            __strcat_chk(szSource, szQol, sizeof(szSource));
        }
#endif

#ifdef K2_QOL_SOFT_SHADOWS
        const char* pszVertexPath = (nFlags & ProgramType::SHADER_BLOOM) ? "kotorbloom.vert" : (nFlags & (ProgramType::SHADER_POST1 | ProgramType::SHADER_SOFT_SHADOW_COPY | ProgramType::SHADER_SOFT_SHADOW_BLUR)) ? "kotorpost1.vert" : "kotor.vert";
#else
        const char* pszVertexPath = (nFlags & ProgramType::SHADER_BLOOM) ? "kotorbloom.vert" : (nFlags & ProgramType::SHADER_POST1) ? "kotorpost1.vert" : "kotor.vert";
#endif
        const char* pszFragmentPath = (nFlags & ProgramType::SHADER_BLOOM) ? "kotorbloom.frag" : (nFlags & ProgramType::SHADER_POST1) ? "kotorpost1.frag" : "kotor.frag";
#ifdef K2_QOL_BUMPMAPS
        if (nFlags & (ProgramType::SHADER_QOL_BUMP | ProgramType::SHADER_QOL_BUMPYSHINY | ProgramType::SHADER_QOL_FOG)) {
            pszVertexPath = "kotorbump.vert";
            pszFragmentPath = "kotorbump.frag";
        } else if (nFlags & ProgramType::SHADER_QOL_NOISE) {
            pszVertexPath = "kotornoise.vert";
            pszFragmentPath = "kotornoise.frag";
        }
#endif
        unsigned int nVertexShader = CompileShader(pszVertexPath, szSource, GL_VERTEX_SHADER);
#ifdef K2_QOL_SOFT_SHADOWS
        unsigned int nFragmentShader = (nFlags & (ProgramType::SHADER_SOFT_SHADOW_COPY | ProgramType::SHADER_SOFT_SHADOW_BLUR))
            ? CompileShaderText((nFlags & ProgramType::SHADER_SOFT_SHADOW_COPY) ? kCopyFragment : kSoftShadowFragment, szSource, GL_FRAGMENT_SHADER)
            : CompileShader(pszFragmentPath, szSource, GL_FRAGMENT_SHADER);
#else
        unsigned int nFragmentShader = CompileShader(pszFragmentPath, szSource, GL_FRAGMENT_SHADER);
#endif

        m_uProgram = ASLgl::glCreateProgram();
        ASLgl::glAttachShader(m_uProgram, nVertexShader);
        ASLgl::glAttachShader(m_uProgram, nFragmentShader);
        ASLgl::glLinkProgram(m_uProgram);

        ASLgl::glGetProgramiv(m_uProgram, GL_LINK_STATUS, &nStatus);      // GL_LINK_STATUS
        ASLgl::glGetProgramiv(m_uProgram, GL_INFO_LOG_LENGTH, &nLogLength);   // GL_INFO_LOG_LENGTH
        int nLength = nLogLength;
        if (nLength >= 2) {
            char* pszLog = static_cast<char*>(malloc(nLength));
            ASLgl::glGetProgramInfoLog(m_uProgram, nLength, 0, pszLog);
            printf("Kotor shader link error: %s\n", pszLog);
            free(pszLog);
        }

        if (nStatus != 0)
            puts("Linked program");
        else
            ASLgl::glDeleteProgram(m_uProgram);

        ASLgl::glUseProgram(m_uProgram);

        // Mark each location queried, then record it.
        m_abAttributeQueried[0] = 1;
        m_anLocations[0] = ASLgl::glGetAttribLocation(m_uProgram, "a_matrixIndices");
        m_abAttributeQueried[1] = 1;
        m_anLocations[1] = ASLgl::glGetAttribLocation(m_uProgram, "a_matrixWeights");
        m_abAttributeQueried[2] = 1;
        m_anLocations[2] = ASLgl::glGetAttribLocation(m_uProgram, "a_position");
        m_abAttributeQueried[3] = 1;
        m_anLocations[3] = ASLgl::glGetAttribLocation(m_uProgram, "a_normal");
        m_abAttributeQueried[4] = 1;
        m_anLocations[4] = ASLgl::glGetAttribLocation(m_uProgram, "a_color");
        m_abAttributeQueried[5] = 1;
        m_anLocations[5] = ASLgl::glGetAttribLocation(m_uProgram, "a_texCoord0");
        m_abAttributeQueried[6] = 1;
        m_anLocations[6] = ASLgl::glGetAttribLocation(m_uProgram, "a_texCoord1");

#ifdef K2_QOL_BUMPMAPS
        m_anBumpLocations[0] = ASLgl::glGetAttribLocation(m_uProgram, "a_bumpT");
        m_anBumpLocations[1] = ASLgl::glGetAttribLocation(m_uProgram, "a_bumpB");
        m_anBumpLocations[2] = ASLgl::glGetAttribLocation(m_uProgram, "a_bumpN");
        m_nBumpUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_bumpParams");
#endif

        m_abSamplerQueried[0] = 1;
        m_anSamplerUniforms[0] = ASLgl::glGetUniformLocation(m_uProgram, "u_texture0Sampler");
        m_abSamplerQueried[1] = 1;
        m_anSamplerUniforms[1] = ASLgl::glGetUniformLocation(m_uProgram, "u_texture1Sampler");
        m_abSamplerQueried[2] = 1;
        m_anSamplerUniforms[2] = ASLgl::glGetUniformLocation(m_uProgram, "u_texture2Sampler");
        m_abSamplerQueried[3] = 1;
        m_anSamplerUniforms[3] = ASLgl::glGetUniformLocation(m_uProgram, "u_texture3Sampler");

        m_abUniformQueried[0] = 1;
        m_nMatrixUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_matrices");
        m_abUniformQueried[1] = 1;
        m_nCameraUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_cameraMatrix");
        m_abUniformQueried[2] = 1;
        m_nSkinUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_boneMatrices");
        m_abUniformQueried[3] = 1;
        m_nLightDataUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_lightData");
        m_abUniformQueried[4] = 1;
        m_nFogColourUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_fogColor");
        m_abUniformQueried[5] = 1;
        m_nFogDataUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_fogData");
        m_abUniformQueried[6] = 1;
        m_nLightMaterialUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_lightMaterialData");
        m_abUniformQueried[7] = 1;
        m_nFragmentShaderUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_fragmentShaderParams");
        m_abUniformQueried[8] = 1;
        m_nVertexShaderUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_vertexShaderParams");
        m_abUniformQueried[9] = 1;
        m_nPreLitColorsUniform = ASLgl::glGetUniformLocation(m_uProgram, "u_preLitColors");

        if (m_anSamplerUniforms[0] >= 0)
            ASLgl::glUniform1i(m_anSamplerUniforms[0], 0);
        if (m_anSamplerUniforms[1] >= 0)
            ASLgl::glUniform1i(m_anSamplerUniforms[1], 1);
        if (m_anSamplerUniforms[2] >= 0)
            ASLgl::glUniform1i(m_anSamplerUniforms[2], 2);
        if (m_anSamplerUniforms[3] >= 0)
            ASLgl::glUniform1i(m_anSamplerUniforms[3], 3);
    }

    ASLgl::glUseProgram(m_uProgram);
}

static unsigned int CompileShader(const char* pszPath, const char* pszHeader, unsigned int nType)
{
    int nStatus;
    int nLogLength;
    char szSource[SHADER_SOURCE_SIZE];
    const char* papszSource[2];

    FILE* pFile = fopen(pszPath, "rb");
#ifdef K2_FIX_ENGINE_BUGS
    if (pFile == 0)
        return 0;
    uint64_t nRead = fread(szSource, 1, sizeof(szSource) - 1, pFile);
#else
    uint64_t nRead = fread(szSource, 1, sizeof(szSource), pFile);
#endif
    fclose(pFile);
    szSource[nRead] = '\0';

    unsigned int nShader = ASLgl::glCreateShader(nType);

    papszSource[0] = pszHeader;
    papszSource[1] = szSource;
#ifdef K2_HOST_BUILD
    // K2_GL_FLATFRAG=1 (a test): every kotor.frag program writes solid magenta, so a draw that
    // rasterises shows as a silhouette whatever its texture, lighting or alpha test.
    static const bool s_bFlat = getenv("K2_GL_FLATFRAG") != 0;
    if (s_bFlat && nType == GL_FRAGMENT_SHADER && strcmp(pszPath, "kotor.frag") == 0) {
        static char s_szHeader[4096];
        snprintf(s_szHeader, sizeof s_szHeader, "%s#define main k2_real_main\n", pszHeader);
        const char* papszFlat[3] = {
            s_szHeader, szSource,
            "\n#undef main\nvoid main()\n{\n    gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);\n}\n"};
        ASLgl::glShaderSource(nShader, 3, papszFlat, 0);
    } else
#endif
    ASLgl::glShaderSource(nShader, 2, papszSource, 0);
    ASLgl::glCompileShader(nShader);

    ASLgl::glGetShaderiv(nShader, GL_COMPILE_STATUS, &nStatus);      // GL_COMPILE_STATUS
    ASLgl::glGetShaderiv(nShader, GL_INFO_LOG_LENGTH, &nLogLength);   // GL_INFO_LOG_LENGTH

    // A log length of 0 or 1 means there is nothing to print.
    int nLength = nLogLength;
    if (nLength >= 2) {
        char* pszLog = static_cast<char*>(malloc(nLength));
        ASLgl::glGetShaderInfoLog(nShader, nLength, 0, pszLog);
        printf("Kotor shader error: %s\n", pszLog);
        free(pszLog);
    }

    if (nStatus != 0) {
        puts("Compiled shader");
    } else {
        ASLgl::glDeleteShader(nShader);
        nShader = 0;
    }
    return nShader;
}

#ifdef K2_QOL_SOFT_SHADOWS
// CompileShader for source text in memory (the port's built-in fragment shaders).
static unsigned int CompileShaderText(const char* pszText, const char* pszHeader, unsigned int nType)
{
    int nStatus = 0;
    int nLogLength = 0;
    const char* papszSource[2] = { pszHeader, pszText };

    unsigned int nShader = ASLgl::glCreateShader(nType);
    ASLgl::glShaderSource(nShader, 2, papszSource, 0);
    ASLgl::glCompileShader(nShader);
    ASLgl::glGetShaderiv(nShader, GL_COMPILE_STATUS, &nStatus);      // GL_COMPILE_STATUS
    ASLgl::glGetShaderiv(nShader, GL_INFO_LOG_LENGTH, &nLogLength);   // GL_INFO_LOG_LENGTH
    if (nLogLength >= 2) {
        char* pszLog = static_cast<char*>(malloc(nLogLength));
        ASLgl::glGetShaderInfoLog(nShader, nLogLength, 0, pszLog);
        printf("Kotor shader error: %s\n", pszLog);
        free(pszLog);
    }
    if (nStatus == 0) {
        ASLgl::glDeleteShader(nShader);
        nShader = 0;
    }
    return nShader;
}
#endif

void KotorProgram::Unuse()
{
    ASLgl::glUseProgram(0);
}

void KotorProgram::SetAttributeShadowOnly(AttributeData& attrA, AttributeData& attrB)
{
    SetAttributeToLoc(attrA, KotorProgramLocations::LOC_2, 0);
    SetAttributeToLoc(attrB, KotorProgramLocations::LOC_4, 1);

    if (m_nPreLitColorsUniform >= 0)
        ASLgl::glUniform1f(m_nPreLitColorsUniform, 1.0f);
}

inline void KotorProgram::SetAttributeToLoc(AttributeData& attr, KotorProgramLocations::Location loc,
                                            unsigned char bNormalized)
{
    int nLocation = m_anLocations[loc];
    if (nLocation < 0)
        return;

    if (attr.m_bIsPointer) {
        ASLgl::glVertexAttribPointer(nLocation, attr.m_nSize, attr.m_nType, bNormalized,
                                     attr.m_nStride, attr.m_pPointer);
        ASLgl::glEnableVertexAttribArray(nLocation);
        return;
    }

    switch (attr.m_nSize) {
    case 1:
        ASLgl::glVertexAttrib1f(nLocation, 1.0f);
        break;
    case 2:
        ASLgl::glVertexAttrib2f(nLocation, 1.0f, 1.0f);
        break;
    case 3:
        ASLgl::glVertexAttrib3f(nLocation, 1.0f, 1.0f, 1.0f);
        break;
    case 4:
        ASLgl::glVertexAttrib4f(nLocation, 1.0f, 1.0f, 1.0f, 1.0f);
        break;
    default:
        break;
    }
    ASLgl::glDisableVertexAttribArray(nLocation);
}

void KotorProgram::SetAttributes(AttributeData& a, AttributeData& b, AttributeData& c, bool bEnabled,
                                 AttributeData& e, AttributeData& f, AttributeData& g, AttributeData& h)
{
    SetAttributeToLoc(a, KotorProgramLocations::LOC_2, 0);
    SetAttributeToLoc(b, KotorProgramLocations::LOC_3, 0);
    SetAttributeToLoc(c, KotorProgramLocations::LOC_4, 1);

    if (m_nPreLitColorsUniform >= 0)
        ASLgl::glUniform1f(m_nPreLitColorsUniform, bEnabled ? 1.0f : 0.0f);

    SetAttributeToLoc(e, KotorProgramLocations::LOC_5, 0);

    if (m_nFlags & (ProgramType::SHADER_LIGHTMAP | ProgramType::SHADER_HOLO))
        SetAttributeToLoc(f, KotorProgramLocations::LOC_6, 0);

    if (m_nFlags & ProgramType::SHADER_SKIN) {
        SetAttributeToLoc(g, KotorProgramLocations::LOC_1, 0);
        SetAttributeToLoc(h, KotorProgramLocations::LOC_0, 0);
    }
}

void KotorProgram::SetMatrix(float* pfMatrix)
{
    // Two matrices per call.
    ASLgl::glUniformMatrix4fv(m_nMatrixUniform, 2, 0, pfMatrix);
}

void KotorProgram::SetCamera(float* pfMatrix)
{
    ASLgl::glUniformMatrix4fv(m_nCameraUniform, 1, 0, pfMatrix);
}

void KotorProgram::SetSkinMatrices(float* pfMatrices)
{
    if ((m_nFlags & ProgramType::SHADER_SKIN) == 0)
        return;
    if (m_nSkinUniform < 0)
        return;
    ASLgl::glUniform4fv(m_nSkinUniform, SKIN_VEC4S, pfMatrices);
}

void KotorProgram::SetLightData(float* pfData)
{
    if (m_nLightDataUniform < 0)
        return;

    // The upload sends the incoming array, not the cached copy.
    if (m_lightDataCache.IsDifferentAndCache(pfData))
        ASLgl::glUniform4fv(m_nLightDataUniform, LIGHT_DATA_VEC4S, pfData);
}

void KotorProgram::SetFogData(float* pfData, float* pfColour)
{
    if (m_nFogColourUniform >= 0) {
        if (m_fogColourCache.IsDifferentAndCache(pfColour))
            ASLgl::glUniform3fv(m_nFogColourUniform, 1, pfColour);
    }

    if (m_nFogDataUniform >= 0) {
        if (m_fogDataCache.IsDifferentAndCache(pfData))
            ASLgl::glUniform4fv(m_nFogDataUniform, 1, pfData);
    }
}

void KotorProgram::SetLightMaterialData(float* pfData)
{
    int nUniform = m_nLightMaterialUniform;
    if (nUniform < 0)
        return;
    if (!m_lightMaterialCache.IsDifferentAndCache(pfData))
        return;
    ASLgl::glUniform4fv(nUniform, 4, pfData);
}

template bool KotorProgramCachedValue<16>::IsDifferentAndCache(float* pfValues);

void KotorProgram::SetFragmentShaderParams(float* pfParams)
{
    int nUniform = m_nFragmentShaderUniform;
    if (nUniform < 0)
        return;
    if (!m_fragmentCache.IsDifferentAndCache(pfParams))
        return;
    ASLgl::glUniform4fv(nUniform, 3, pfParams);
}

template bool KotorProgramCachedValue<12>::IsDifferentAndCache(float* pfValues);

void KotorProgram::SetVertexShaderParams(float* pfParams)
{
    if (m_nVertexShaderUniform < 0)
        return;

    if (m_vertexShaderCache.IsDifferentAndCache(pfParams))
        ASLgl::glUniform4fv(m_nVertexShaderUniform, 1, pfParams);
}

#ifdef K2_QOL_BUMPMAPS
// The tangent frame's three streams (the ARB programs' attributes 12..14), for the bump programs only.
void KotorProgram::SetBumpAttributes(AttributeData* pFrame)
{
    for (int i = 0; i < 3; i++) {
        int nLocation = m_anBumpLocations[i];
        if (nLocation < 0)
            continue;
        if (pFrame[i].m_bIsPointer) {
            ASLgl::glVertexAttribPointer(nLocation, pFrame[i].m_nSize, pFrame[i].m_nType, 0, pFrame[i].m_nStride,
                                         pFrame[i].m_pPointer);
            ASLgl::glEnableVertexAttribArray(nLocation);
        } else {
            ASLgl::glDisableVertexAttribArray(nLocation);
            ASLgl::glVertexAttrib3f(nLocation, 0.0f, 0.0f, 0.0f);
        }
    }
}

// The vertex env params 12..14: the bump light's position and the camera's in the part's frame, and
// the light's attenuation polynomial.
void KotorProgram::SetBumpParams(float* pfParams)
{
    if (m_nBumpUniform >= 0 && m_bumpCache.IsDifferentAndCache(pfParams))
        ASLgl::glUniform4fv(m_nBumpUniform, 3, pfParams);
}
#endif
