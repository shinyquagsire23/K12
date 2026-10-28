// k1kotorgl.cpp -- KOTOR 1's GLSL renderer layer, on desktop GL. The same layer KOTOR II builds from
// shim/shim_kotorgl.cpp, at a later revision.
//
// No ARB program is ever run: glGenProgramsARB / glProgramStringARB / glDeleteProgramsARB do nothing,
// and glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, id) turns the engine's fixed fragment-program id into
// a ProgramType flag word and binds a GLSL KotorProgram (the data set's kotor.vert / kotor.frag,
// built with `#define USE_*` lines for those flags). The vertex side the engine's ARB vertex programs
// used to read (bones, camera, lights, material) arrives through glProgramEnvParameter4fARB, on KOTOR
// II's env-param layout, and is uploaded as uniforms before every draw (GlesKotorSetupShader). Fog
// and the alpha reference are shader data too: neither GL_FOG nor GL_ALPHA_TEST reaches GL.
//
// KOTOR 1's revision, against KOTOR II's:
//   * ProgramType takes (id, skinning, alpha-test) and knows 13 ids (6..43); every other id is the
//     plain program, with no "ERROR SHADER" print. The flag bits are KOTOR II's. Missing from the
//     shipped build, and supplied here: ids 0x100 (flag 0x100), the force-distortion pass, 0x101
//     (flag 0x200), the Force Speed blur, 0x102 (flag 0x400), a plain copy, and 0x103 (flag 0x800),
//     the soft-shadow blur, whose fragment shaders are built in.
//   * KotorProgram builds 6 defines (SKIN, ALPHATEST, LIGHTMAP, FLAT_CUBEMAP, CUBEMAP, BUMPMAP),
//     queries 6 attributes (no a_color), 3 samplers and 9 uniforms (no u_preLitColor), uploads 3
//     material rows and 2 fragment-parameter rows, and reads its shaders with SDL_RWFromFile (the
//     data set, else the copies compiled into the binary: a PC data set has none).
//   * glDisable(GL_FRAGMENT_PROGRAM_ARB) does NOT unbind the current program (only a bind of id 0
//     does); glEnable/glDisable(GL_ALPHA_TEST) only move the shader's alpha reference (-1 = off).
//
// The shipped build draws through gles2-bc, which feeds the program's attributes from its own copies
// of the client arrays. Here, as in shim_kotorgl.cpp, the target is a desktop GL 2.1 compatibility
// context: ASLgl::glShaderSource (the KOTOR II shim) rewrites the GLSL ES for GLSL 1.20 with
// a_position & co. mapped to the built-ins, which read the engine's client arrays and immediate mode
// directly; only the skinning streams (generic attributes 1 and 4) still go through the shadows here.
// The matrices come from the native matrix stacks, and glBegin runs the setup a draw does. The
// GL_VERTEX_PROGRAM_ARB / GL_FRAGMENT_PROGRAM_ARB capabilities, which gles2-bc ignored, are dropped:
// enabling either natively with no ARB program loaded would fail every draw.
#include "../shim/aslgl.gen.h"
#include "../shim/k2shim.h"

#include <SDL.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" const unsigned char* k2embedded_find(const char* pszName, size_t* pnSize);   // embedfiles.py

void* k2shim_gl_proc(const char* pszName);
extern bool k1gl_tracing;                        // k1port.cpp: the one-frame call trace
void k1gl_trace(const char* pszFormat, ...);
extern bool s_useAlphaTestShader;                // k1port.cpp; the engine sets it (aurmaterial, aurgrass)
extern float s_vertexShaderParams[4];            // src/port/ASL2/OpenGLES/aslgl.cpp: the bump-shell amplitude

namespace ASLgl {
// Rewrites GLSL ES 1.00 as GLSL 1.20 on the built-ins; the KOTOR II shim's (shim_kotorgl.cpp).
void glShaderSource(unsigned int shader, int count, char const* const* strings, int const* lengths);
}

namespace {

enum {
    GL_FOG = 0x0b60, GL_FOG_START = 0x0b63, GL_FOG_END = 0x0b64, GL_FOG_COLOR = 0x0b66,
    GL_ALPHA_TEST = 0x0bc0, GL_QUADS = 0x0007, GL_VERTEX_PROGRAM_ARB = 0x8620,
    GL_FRAGMENT_PROGRAM_ARB = 0x8804, GL_MODELVIEW_MATRIX = 0x0ba6, GL_PROJECTION_MATRIX = 0x0ba7,
    GL_CURRENT_PROGRAM = 0x8b8d, GL_FRAGMENT_SHADER = 0x8b30, GL_VERTEX_SHADER = 0x8b31,
    GL_COMPILE_STATUS = 0x8b81, GL_LINK_STATUS = 0x8b82, GL_INFO_LOG_LENGTH = 0x8b84,
};

// One vertex stream as glVertexAttribPointer takes it (KOTOR II's AttributeData).
struct AttributeData {
    int m_nSize;
    int m_nType;
    int m_nStride;
    const void* m_pPointer;
    bool m_bIsPointer;
};

// ProgramType::ProgramType(int, bool, bool)
struct ProgramType {
    ProgramType(int nType, bool bSkin, bool bAlphaTest)
    {
        m_nFlags = (bAlphaTest ? 0x20 : 0) | (bSkin ? 0x1 : 0);
        switch (nType) {
        case 6:            m_nFlags |= 0x4; break;    // bump
        case 19: case 20:  m_nFlags |= 0x8; break;    // cube map
        case 22: case 39:  m_nFlags |= 0xa; break;    // lightmap + cube map
        case 24: case 29:  m_nFlags |= 0xc; break;    // bump + cube map
        case 26:           m_nFlags |= 0x2; break;    // lightmap
        case 40:           m_nFlags |= 0x10; break;   // kotorpost1
        case 41:           m_nFlags |= 0x80; break;   // kotorbloom
        case 43:           m_nFlags |= 0x48; break;   // flat cube map
        case 0x100:        m_nFlags |= 0x100; break;  // the port's force distortion (kDistortionFragment)
        case 0x101:        m_nFlags |= 0x200; break;  // the port's Force Speed blur (kSpeedBlurFragment)
        case 0x102:        m_nFlags |= 0x400; break;  // the port's copy (kCopyFragment)
        case 0x103:        m_nFlags |= 0x800; break;  // the port's soft-shadow blur (kSoftShadowFragment)
        default:           break;
        }
#ifdef K2_QOL_BUMPMAPS
        if (nType == 6)
            m_nFlags |= 0x1000;                       // the bump pass: kotorbump.vert / .frag
        else if (nType == 24)
            m_nFlags |= 0x2000;                       // the bumpy-shiny pass: the same, USE_BUMPYSHINY
#endif
    }
    int m_nFlags;
};

// KOTOR 1's force-distortion pass (a heat-distortion vertex program over kotorpost1.vert's
// full-screen quad), the port's own program rather than a data-set asset. Unit 0 is the copy of the
// scene, unit 2 distortiontex (its quad coordinates run top to bottom); the scene is read at
// d.z * screen + d.xy * screen / 10, d = 2 * distortiontex - 1. The engine stencils it to the
// force-push gobs (aurglcore.cpp, K1RenderForceDistortion).
const char kDistortionFragment[] =
    "precision mediump float;\n"
    "uniform sampler2D u_texture0Sampler;\n"
    "uniform sampler2D u_texture2Sampler;\n"
    "varying mediump vec4 v_texCoord0;\n"
    "void main()\n"
    "{\n"
    "    vec3 d = texture2D(u_texture2Sampler, vec2(v_texCoord0.s, 1.0 - v_texCoord0.t)).xyz * 2.0 - 1.0;\n"
    "    gl_FragColor = vec4(texture2D(u_texture0Sampler, d.z * v_texCoord0.st + 0.1 * d.xy).xyz, 1.0);\n"
    "}\n";

// The Force Speed blur, KOTOR 1's accumulation blur, blends the last blurred frame (unit 0) over
// the scene with alpha = the accumulation ratio (fragment parameter 0.x) (aurglcore.cpp,
// K1RenderSpeedBlur).
const char kSpeedBlurFragment[] =
    "precision mediump float;\n"
    "uniform vec4 u_fragmentShaderParams[2];\n"
    "uniform sampler2D u_texture0Sampler;\n"
    "varying mediump vec4 v_texCoord0;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(texture2D(u_texture0Sampler, v_texCoord0.st).xyz, u_fragmentShaderParams[0].x);\n"
    "}\n";

// Unit 0 as it is, for the soft-shadow pass's knock-down, scene restore and darkening quads.
const char kCopyFragment[] =
    "precision mediump float;\n"
    "uniform sampler2D u_texture0Sampler;\n"
    "varying mediump vec4 v_texCoord0;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(u_texture0Sampler, v_texCoord0.st);\n"
    "}\n";

// The soft-shadow blur, KOTOR 1's bloom filter with the soft-shadow program, runs the 512x512
// shadow mask (unit 0) through the 5x5 bloom kernel times softshadowdarkeningfactor (0.4), offsets
// -2..2 texels.
// The shipped build walks the kernel in seven 4-tap passes, each s = sum(w * tap), added with
// glBlendFunc(GL_SRC_ALPHA, GL_ONE) -- so the result is sum(s * s.a) -- and the taps are grouped as
// it groups them. One pass here, written out.
const char kSoftShadowFragment[] =
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

// Compiles a shader from a file, or from pszText when the path is null. A missing asset is reported.
unsigned int CompileShader(const char* pszPath, const char* pszHeader, unsigned int nType,
                           const char* pszText = 0)
{
    static char s_szSource[0x20000];
    if (pszText) {
        snprintf(s_szSource, sizeof(s_szSource), "%s", pszText);
        pszPath = "(built in)";
    } else {
        SDL_RWops* rw = SDL_RWFromFile(pszPath, "rb");
        size_t nEmbedded;
        if (!rw)
            if (const unsigned char* pEmbedded = k2embedded_find(pszPath, &nEmbedded))
                rw = SDL_RWFromConstMem(pEmbedded, static_cast<int>(nEmbedded));
        if (!rw) {
            fprintf(stderr, "k1: shader %s: not found (the working directory / built in)\n", pszPath);
            return 0;
        }
        size_t nRead = SDL_RWread(rw, s_szSource, 1, sizeof(s_szSource) - 1);
        SDL_RWclose(rw);
        s_szSource[nRead] = '\0';
    }
    unsigned int nShader = k2gl::glCreateShader(nType);
    const char* apszSource[2] = { pszHeader, s_szSource };
    ASLgl::glShaderSource(nShader, 2, apszSource, 0);
    k2gl::glCompileShader(nShader);
    int nStatus = 0, nLogLength = 0;
    k2gl::glGetShaderiv(nShader, GL_COMPILE_STATUS, &nStatus);
    k2gl::glGetShaderiv(nShader, GL_INFO_LOG_LENGTH, &nLogLength);
    if (nStatus == 0) {
        // The device fetches the log and drops it; the host prints it.
        char* pszLog = static_cast<char*>(malloc(nLogLength > 1 ? nLogLength : 1));
        pszLog[0] = 0;
        if (nLogLength > 1) k2gl::glGetShaderInfoLog(nShader, nLogLength, 0, pszLog);
        fprintf(stderr, "k1: shader %s does not compile: %s\n", pszPath, pszLog);
        free(pszLog);
        k2gl::glDeleteShader(nShader);
        nShader = 0;
    }
    return nShader;
}

template <int N>
bool IsDifferentAndCache(float* pfCache, const float* pfValues)
{
    for (int i = 0; i < N; i++) {
        if (pfCache[i] != pfValues[i]) {
            memcpy(pfCache, pfValues, N * sizeof(float));
            return true;
        }
    }
    return false;
}

// Attribute locations, in the order Use() queries them.
enum { LOC_INDICES, LOC_WEIGHTS, LOC_POSITION, LOC_NORMAL, LOC_TEXCOORD0, LOC_TEXCOORD1,
#ifdef K2_QOL_BUMPMAPS
       LOC_BUMP_T, LOC_BUMP_B, LOC_BUMP_N,
#endif
       LOC_COUNT };

class KotorProgram {
public:
    explicit KotorProgram(ProgramType type)
    {
        m_nCameraSerial = 0;
        m_nFogSerial = 0;
        for (float& f : m_afLightData) f = -1.0f;
        for (float& f : m_afFogColour) f = -1.0f;
        for (float& f : m_afFogData) f = -1.0f;
        for (float& f : m_afLightMaterialData) f = -1.0f;
        for (float& f : m_afFragmentShaderParams) f = -1.0f;
        for (float& f : m_afVertexShaderParams) f = -1.0f;
#ifdef K2_QOL_BUMPMAPS
        for (float& f : m_afBumpParams) f = -1.0f;
#endif
        m_uProgram = 0;
        m_nFlags = type.m_nFlags;
        Use();
    }

    void Use()
    {
        if (m_uProgram == 0) {
            static char s_szDefines[0x400];
            int f = m_nFlags;
            snprintf(s_szDefines, sizeof(s_szDefines),
                     "#define USE_SKIN %d\n#define USE_ALPHATEST %d\n#define USE_LIGHTMAP %d\n"
                     "#define USE_FLAT_CUBEMAP %d\n#define USE_CUBEMAP %d\n#define USE_BUMPMAP %d\n",
                     (f & 1) != 0, (f & 0x20) != 0, (f & 2) != 0, (f & 0x40) != 0, (f & 8) != 0, (f & 4) != 0);
#ifdef K2_QOL_BUMPMAPS
            size_t nDefines = strlen(s_szDefines);
            snprintf(s_szDefines + nDefines, sizeof(s_szDefines) - nDefines, "#define USE_BUMPYSHINY %d\n",
                     (f & 0x2000) != 0);
#endif
            const char* pszVertex = (f & 0x80) ? "kotorbloom.vert" : (f & 0xf10) ? "kotorpost1.vert" : "kotor.vert";
            const char* pszFragment = (f & 0x80) ? "kotorbloom.frag" : (f & 0x10) ? "kotorpost1.frag" : "kotor.frag";
#ifdef K2_QOL_BUMPMAPS
            if (f & 0x3000) {
                pszVertex = "kotorbump.vert";
                pszFragment = "kotorbump.frag";
            }
#endif
            unsigned int nVertexShader = CompileShader(pszVertex, s_szDefines, GL_VERTEX_SHADER);
            unsigned int nFragmentShader = CompileShader(pszFragment, s_szDefines, GL_FRAGMENT_SHADER,
                                                         (f & 0x100) ? kDistortionFragment
                                                         : (f & 0x200) ? kSpeedBlurFragment
                                                         : (f & 0x400) ? kCopyFragment
                                                         : (f & 0x800) ? kSoftShadowFragment : 0);
            m_uProgram = k2gl::glCreateProgram();
            k2gl::glAttachShader(m_uProgram, nVertexShader);
            k2gl::glAttachShader(m_uProgram, nFragmentShader);
            // Host only: gl4es keeps the fixed-function arrays in generic slots (0 vertex, 2 normal,
            // 3 colour, 4 secondary, 5 fog coord, 8+ texcoords), so a skin stream the linker put in
            // one of them would replace that array for every later fixed-function draw. 6 and 7 are
            // the two slots none of them uses.
            typedef void (*BindAttribLocation)(unsigned int, unsigned int, const char*);
            static BindAttribLocation s_pBind = reinterpret_cast<BindAttribLocation>(k2shim_gl_proc("glBindAttribLocation"));
            if (s_pBind) {
                s_pBind(m_uProgram, 6, "a_matrixIndices");
                s_pBind(m_uProgram, 7, "a_matrixWeights");
#ifdef K2_QOL_BUMPMAPS
                // gl4es' texcoord sets 4..6, which nothing else uses.
                s_pBind(m_uProgram, 12, "a_bumpT");
                s_pBind(m_uProgram, 13, "a_bumpB");
                s_pBind(m_uProgram, 14, "a_bumpN");
#endif
            }
            k2gl::glLinkProgram(m_uProgram);
            int nStatus = 0, nLogLength = 0;
            k2gl::glGetProgramiv(m_uProgram, GL_LINK_STATUS, &nStatus);
            k2gl::glGetProgramiv(m_uProgram, GL_INFO_LOG_LENGTH, &nLogLength);
            if (nStatus == 0) {
                char* pszLog = static_cast<char*>(malloc(nLogLength > 1 ? nLogLength : 1));
                pszLog[0] = 0;
                if (nLogLength > 1) k2gl::glGetProgramInfoLog(m_uProgram, nLogLength, 0, pszLog);
                fprintf(stderr, "k1: KotorProgram flags 0x%x does not link: %s\n", m_nFlags, pszLog);
                free(pszLog);
                k2gl::glDeleteProgram(m_uProgram);
            }
            k2gl::glUseProgram(m_uProgram);
            static const char* const s_apszAttributes[LOC_COUNT] = {
                "a_matrixIndices", "a_matrixWeights", "a_position", "a_normal", "a_texCoord0", "a_texCoord1",
#ifdef K2_QOL_BUMPMAPS
                "a_bumpT", "a_bumpB", "a_bumpN",
#endif
            };
            for (int i = 0; i < LOC_COUNT; i++)
                m_anLocations[i] = k2gl::glGetAttribLocation(m_uProgram, s_apszAttributes[i]);
            k1gl_trace("KotorProgram flags %#x: program %u, indices at %d, weights at %d", m_nFlags, m_uProgram,
                       m_anLocations[LOC_INDICES], m_anLocations[LOC_WEIGHTS]);
            m_anSamplerUniforms[0] = k2gl::glGetUniformLocation(m_uProgram, "u_texture0Sampler");
            m_anSamplerUniforms[1] = k2gl::glGetUniformLocation(m_uProgram, "u_texture1Sampler");
            m_anSamplerUniforms[2] = k2gl::glGetUniformLocation(m_uProgram, "u_texture2Sampler");
            m_nMatrixUniform = k2gl::glGetUniformLocation(m_uProgram, "u_matrices");
            m_nCameraUniform = k2gl::glGetUniformLocation(m_uProgram, "u_cameraMatrix");
            m_nSkinUniform = k2gl::glGetUniformLocation(m_uProgram, "u_boneMatrices");
            m_nLightDataUniform = k2gl::glGetUniformLocation(m_uProgram, "u_lightData");
            m_nFogColourUniform = k2gl::glGetUniformLocation(m_uProgram, "u_fogColor");
            m_nFogDataUniform = k2gl::glGetUniformLocation(m_uProgram, "u_fogData");
            m_nLightMaterialUniform = k2gl::glGetUniformLocation(m_uProgram, "u_lightMaterialData");
            m_nFragmentShaderUniform = k2gl::glGetUniformLocation(m_uProgram, "u_fragmentShaderParams");
            m_nVertexShaderUniform = k2gl::glGetUniformLocation(m_uProgram, "u_vertexShaderParams");
#ifdef K2_QOL_BUMPMAPS
            m_nBumpUniform = k2gl::glGetUniformLocation(m_uProgram, "u_bumpParams");
#endif
            for (int i = 0; i < 3; i++)
                if (m_anSamplerUniforms[i] >= 0)
                    k2gl::glUniform1i(m_anSamplerUniforms[i], i);
        }
        k2gl::glUseProgram(m_uProgram);
        // (The device then disables generic attribute arrays 0..9 in gles2-bc's state. Here the
        // built-ins own the client arrays -- gl4es keeps gl_Vertex in attribute 0 -- so that reset
        // would switch the engine's own vertex array off.)
    }

    void Unuse() { k2gl::glUseProgram(0); }

    void SetAttributeToLoc(AttributeData& attr, int nLoc)
    {
        int nLocation = m_anLocations[nLoc];
        if (nLocation < 0)
            return;
        if (attr.m_bIsPointer) {
            k2gl::glVertexAttribPointer(nLocation, attr.m_nSize, attr.m_nType, 0, attr.m_nStride, attr.m_pPointer);
            k2gl::glEnableVertexAttribArray(nLocation);
            return;
        }
        switch (attr.m_nSize) {
        case 1: k2gl::glVertexAttrib1f(nLocation, 1.0f); break;
        case 2: k2gl::glVertexAttrib2f(nLocation, 1.0f, 1.0f); break;
        case 3: k2gl::glVertexAttrib3f(nLocation, 1.0f, 1.0f, 1.0f); break;
        case 4: k2gl::glVertexAttrib4f(nLocation, 1.0f, 1.0f, 1.0f, 1.0f); break;
        default: break;
        }
        k2gl::glDisableVertexAttribArray(nLocation);
    }

// The colour stream (the third) is passed and never bound: this revision has no a_color.
    void SetAttributes(AttributeData& position, AttributeData& normal, AttributeData&, AttributeData& texCoord0,
                       AttributeData& texCoord1, AttributeData& weights, AttributeData& indices)
    {
        SetAttributeToLoc(position, LOC_POSITION);
        SetAttributeToLoc(normal, LOC_NORMAL);
        SetAttributeToLoc(texCoord0, LOC_TEXCOORD0);
        if (m_nFlags & 2)
            SetAttributeToLoc(texCoord1, LOC_TEXCOORD1);
        if (m_nFlags & 1) {
            SetAttributeToLoc(weights, LOC_WEIGHTS);
            SetAttributeToLoc(indices, LOC_INDICES);
        }
    }

#ifdef K2_QOL_BUMPMAPS
    // The tangent frame's three streams, for the bump program only.
    void SetBumpAttributes(AttributeData* pFrame)
    {
        if (!(m_nFlags & 0x3000))
            return;
        for (int i = 0; i < 3; i++)
            SetAttributeToLoc(pFrame[i], LOC_BUMP_T + i);
    }

    void SetBumpParams(float* pf)
    {
        if (m_nBumpUniform >= 0 && IsDifferentAndCache<12>(m_afBumpParams, pf))
            k2gl::glUniform4fv(m_nBumpUniform, 3, pf);
    }
#endif

    void SetMatrix(float* pf) { k2gl::glUniformMatrix4fv(m_nMatrixUniform, 2, 0, pf); }
    void SetCamera(float* pf) { k2gl::glUniformMatrix4fv(m_nCameraUniform, 1, 0, pf); }
    void SetSkinMatrices(float* pf)
    {
        if ((m_nFlags & 1) && m_nSkinUniform >= 0)
            k2gl::glUniform4fv(m_nSkinUniform, 0x33, pf);
    }

    void SetLightData(float* pf)
    {
        if (m_nLightDataUniform >= 0 && IsDifferentAndCache<0x3c>(m_afLightData, pf))
            k2gl::glUniform4fv(m_nLightDataUniform, 0xf, pf);
    }

    void SetFogData(float* pfData, float* pfColour)
    {
        if (m_nFogColourUniform >= 0 && IsDifferentAndCache<3>(m_afFogColour, pfColour))
            k2gl::glUniform3fv(m_nFogColourUniform, 1, pfColour);
        if (m_nFogDataUniform >= 0 && IsDifferentAndCache<4>(m_afFogData, pfData))
            k2gl::glUniform4fv(m_nFogDataUniform, 1, pfData);
    }

    void SetLightMaterialData(float* pf)
    {
        if (m_nLightMaterialUniform >= 0 && IsDifferentAndCache<12>(m_afLightMaterialData, pf))
            k2gl::glUniform4fv(m_nLightMaterialUniform, 3, pf);
    }

    void SetFragmentShaderParams(float* pf)
    {
        if (m_nFragmentShaderUniform >= 0 && IsDifferentAndCache<8>(m_afFragmentShaderParams, pf))
            k2gl::glUniform4fv(m_nFragmentShaderUniform, 2, pf);
    }

    void SetVertexShaderParams(float* pf)
    {
        if (m_nVertexShaderUniform >= 0 && IsDifferentAndCache<4>(m_afVertexShaderParams, pf))
            k2gl::glUniform4fv(m_nVertexShaderUniform, 1, pf);
    }

    int m_nCameraSerial;                 // the layer's camera serial at the last upload
    int m_nFogSerial;                    // ... fog serial
    float m_afLightData[0x3c];
    float m_afFogColour[3];
    float m_afFogData[4];
    float m_afLightMaterialData[12];
    float m_afFragmentShaderParams[8];
    float m_afVertexShaderParams[4];
#ifdef K2_QOL_BUMPMAPS
    float m_afBumpParams[12];
    int m_nBumpUniform;
#endif
    unsigned int m_uProgram;
    int m_anLocations[LOC_COUNT];
    int m_anSamplerUniforms[3];
    int m_nMatrixUniform;
    int m_nCameraUniform;
    int m_nSkinUniform;
    int m_nLightDataUniform;
    int m_nFogColourUniform;
    int m_nFogDataUniform;
    int m_nLightMaterialUniform;
    int m_nFragmentShaderUniform;
    int m_nVertexShaderUniform;
    int m_nFlags;
};

// Traces the enabled vertex attribute arrays as the GL sees them (gl4es aliases the fixed-function
// arrays onto generic slots: 0 vertex, 2 normal, 3 colour, 4 secondary, 5 fog, 8+ texcoords).
void TraceArrays(bool bKotor)
{
    typedef void (*GetIv)(unsigned int, unsigned int, int*);
    typedef void (*GetPv)(unsigned int, unsigned int, void**);
    static GetIv s_pGetIv = reinterpret_cast<GetIv>(k2shim_gl_proc("glGetVertexAttribiv"));
    static GetPv s_pGetPv = reinterpret_cast<GetPv>(k2shim_gl_proc("glGetVertexAttribPointerv"));
    if (!s_pGetIv || !s_pGetPv)
        return;
    for (unsigned int i = 0; i < 16; i++) {
        int nEnabled = 0, nBuffer = 0, nStride = 0;
        void* pPointer = 0;
        s_pGetIv(i, 0x8622 /* GL_VERTEX_ATTRIB_ARRAY_ENABLED */, &nEnabled);
        if (!nEnabled)
            continue;
        s_pGetIv(i, 0x889f /* GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING */, &nBuffer);
        s_pGetIv(i, 0x8624 /* GL_VERTEX_ATTRIB_ARRAY_STRIDE */, &nStride);
        s_pGetPv(i, 0x8645 /* GL_VERTEX_ATTRIB_ARRAY_POINTER */, &pPointer);
        k1gl_trace("  attrib %u: buffer %d stride %d pointer %p", i, nBuffer, nStride, pPointer);
        if (i == 3 && nBuffer == 0 && pPointer && !bKotor) {   // a client colour array: its first texels
            const unsigned char* c = static_cast<const unsigned char*>(pPointer);
            int n = nStride ? nStride : 4;
            k1gl_trace("    colours %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x", c[0], c[1],
                       c[2], c[3], c[n], c[n + 1], c[n + 2], c[n + 3], c[2 * n], c[2 * n + 1], c[2 * n + 2],
                       c[2 * n + 3], c[3 * n], c[3 * n + 1], c[3 * n + 2], c[3 * n + 3]);
        }
    }
    // the textures a KotorProgram samples: units 0..2, 2D and cube, with the cube's level chain
    typedef void (*GetTexLevelIv)(unsigned int, int, unsigned int, int*);
    typedef void (*GetTexIv)(unsigned int, unsigned int, int*);
    static GetTexLevelIv s_pLevelIv = reinterpret_cast<GetTexLevelIv>(k2shim_gl_proc("glGetTexLevelParameteriv"));
    static GetTexIv s_pTexIv = reinterpret_cast<GetTexIv>(k2shim_gl_proc("glGetTexParameteriv"));
    if (!s_pLevelIv || !s_pTexIv)
        return;
    int nActive = 0;
    k2gl::glGetIntegerv(0x84e0 /* GL_ACTIVE_TEXTURE */, &nActive);
    for (int u = 0; u < 3; u++) {
        k2gl::glActiveTexture(0x84c0 + u);
        int n2D = 0, nCube = 0;
        k2gl::glGetIntegerv(0x8069 /* GL_TEXTURE_BINDING_2D */, &n2D);
        k2gl::glGetIntegerv(0x8514 /* GL_TEXTURE_BINDING_CUBE_MAP */, &nCube);
        if (!bKotor) {
            int nEnv = 0;
            typedef void (*GetTexEnviv)(unsigned int, unsigned int, int*);
            static GetTexEnviv s_pEnv = reinterpret_cast<GetTexEnviv>(k2shim_gl_proc("glGetTexEnviv"));
            if (s_pEnv) s_pEnv(0x2300 /* GL_TEXTURE_ENV */, 0x2200 /* MODE */, &nEnv);
            int nFormat = 0;
            if (n2D) s_pLevelIv(0x0de1, 0, 0x1003 /* GL_TEXTURE_INTERNAL_FORMAT */, &nFormat);
            k1gl_trace("  ff unit %d: 2D %d%s (format %#x) cube %d%s env %#x", u, n2D,
                       k2gl::glIsEnabled(0xde1) ? " (on)" : "", nFormat, nCube, k2gl::glIsEnabled(0x8513) ? " (on)" : "",
                       nEnv);
            continue;
        }
        if (nCube) {
            int nMin = 0, nMax = 0, w0 = 0, w1 = 0, wLast = 0, nFace5 = 0;
            s_pTexIv(0x8513, 0x2801 /* MIN_FILTER */, &nMin);
            s_pTexIv(0x8513, 0x813d /* MAX_LEVEL */, &nMax);
            s_pLevelIv(0x8515, 0, 0x1000 /* WIDTH */, &w0);
            s_pLevelIv(0x8515, 1, 0x1000, &w1);
            s_pLevelIv(0x851a, 0, 0x1000, &nFace5);
            for (int l = 0; l < 16; l++) {
                int w = 0;
                s_pLevelIv(0x8515, l, 0x1000, &w);
                if (!w) break;
                wLast = l;
            }
            k1gl_trace("  unit %d: 2D %d cube %d (min %#x maxlevel %d, +X w0 %d w1 %d last level %d, -Z w0 %d)", u,
                       n2D, nCube, nMin, nMax, w0, w1, wLast, nFace5);
        } else {
            k1gl_trace("  unit %d: 2D %d", u, n2D);
        }
    }
    k2gl::glActiveTexture(nActive);
}

// ---- the layer's state -----------------------------------------------------------------------------
AttributeData s_unused;               // stands in for the client-array shadows the built-ins replaced
AttributeData s_weights;              // generic attribute 1
AttributeData s_indices;              // generic attribute 4
bool s_bSkinning;                     // glEnableVertexAttribArrayARB(1)
#ifdef K2_QOL_BUMPMAPS
AttributeData s_bumpFrame[3];         // generic attributes 12..14, the tangent frame's rows
float s_bumpParams[3 * 4];            // env params 12..14: bump light, camera, attenuation
#endif
float s_fogColor[3];
float s_fogData[4];                   // u_fogData: start, end
float s_lastSetFogData[2];            // what glFogf set, for the next glEnable(GL_FOG)
bool s_bFogDisabled;
float s_boneMatrix[0x33 * 4];         // env params 0x12..0x44, three rows per bone
float s_lightData[15 * 4];            // env params 0x49..0x57
float s_lightMaterialData[3 * 4];     // env params 0x5d..0x5f
float s_fragmentShaderParams[6 * 4];  // fragment env params 0..5
int s_nFogSerial = 1;
float s_fAlphaRef = -1.0f;            // the last glAlphaFunc reference
float s_fAlphaTestRef = -1.0f;        // the shader's reference: -1 = alpha test off
float s_cameraMatrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
int s_nCameraSerial = 1;

std::map<unsigned int, KotorProgram*> s_Programs;   // keyed by ProgramType::m_nFlags
KotorProgram* s_pCurrent;

// Texturing has to be reset before a draw with no KotorProgram: the engine leaves units 1..3 enabled
// after its multi-texture meshes (and a cube map enabled on unit 0), and desktop GL / gl4es would
// multiply them in -- an empty unit samples black: the dark particles, decals and blaster bolts. Such
// a draw is the fixed-function path, whose shader is the vertex colour times unit 0's 2D texture and
// nothing else. So texturing goes off on units 1..3 and the cube target on unit 0; the programs read
// their samplers whatever is enabled. Re-done only after the engine enables a texture target.
bool s_bFixedFunctionUnitsDirty = true;
void SetupFixedFunctionUnits()
{
    if (!s_bFixedFunctionUnitsDirty)
        return;
    s_bFixedFunctionUnitsDirty = false;
    int nActive = 0x84c0;
    k2gl::glGetIntegerv(0x84e0 /* GL_ACTIVE_TEXTURE */, &nActive);
    for (unsigned int u = 0; u < 4; u++) {
        k2gl::glActiveTexture(0x84c0 + u);
        if (u != 0)
            k2gl::glDisable(0x0de1 /* GL_TEXTURE_2D */);
        k2gl::glDisable(0x8513 /* GL_TEXTURE_CUBE_MAP */);
    }
    k2gl::glActiveTexture(nActive);
}

// GlesKotorSetupShader.
void SetupShader()
{
    KotorProgram* p = s_pCurrent;
    if (!p) {
        SetupFixedFunctionUnits();
        return;
    }
    p->SetAttributes(s_unused, s_unused, s_unused, s_unused, s_unused, s_weights, s_indices);

    // u_matrices = { MV, P * MV }.
    float m[32], proj[16];
    k2gl::glGetFloatv(GL_MODELVIEW_MATRIX, m);
    k2gl::glGetFloatv(GL_PROJECTION_MATRIX, proj);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float f = 0;
            for (int k = 0; k < 4; k++)
                f += proj[k * 4 + r] * m[c * 4 + k];
            m[16 + c * 4 + r] = f;
        }
    p->SetMatrix(m);

    if (p->m_nCameraSerial != s_nCameraSerial) {
        p->SetCamera(s_cameraMatrix);
        p->m_nCameraSerial = s_nCameraSerial;
    }
    p->SetSkinMatrices(s_boneMatrix);
#ifdef K2_QOL_BUMPMAPS
    p->SetBumpAttributes(s_bumpFrame);
    p->SetBumpParams(s_bumpParams);
#endif
    p->SetLightData(s_lightData);
    p->SetLightMaterialData(s_lightMaterialData);
    if (p->m_nFogSerial != s_nFogSerial) {
        p->SetFogData(s_fogData, s_fogColor);
        p->m_nFogSerial = s_nFogSerial;
    }
    s_fragmentShaderParams[7] = s_fAlphaTestRef;
    s_pCurrent->SetFragmentShaderParams(s_fragmentShaderParams);
    s_pCurrent->SetVertexShaderParams(s_vertexShaderParams);
}

} // namespace

extern "C" {

void android_port_glBindProgramARB(unsigned int target, unsigned int id)
{
    if (k1gl_tracing) k1gl_trace("glBindProgramARB(%#x, %u)", target, id);
    if (target != GL_FRAGMENT_PROGRAM_ARB)
        return;
    if (id == 0) {
        if (s_pCurrent) {
            s_pCurrent->Unuse();
            s_pCurrent = 0;
            k2shim_glarr_attribs_off();
        }
        return;
    }
    ProgramType type(id, s_bSkinning, s_useAlphaTestShader);
    unsigned int key = type.m_nFlags;
    if (s_pCurrent && static_cast<unsigned int>(s_pCurrent->m_nFlags) == key)
        return;
    if (s_pCurrent)
        k2shim_glarr_attribs_off();
    KotorProgram*& slot = s_Programs[key];
    if (!slot)
        slot = new KotorProgram(type);   // the ctor builds and binds the program
    s_pCurrent = slot;
    int nBound = 0;
    k2gl::glGetIntegerv(GL_CURRENT_PROGRAM, &nBound);
    if (slot->m_uProgram == 0 || slot->m_uProgram != static_cast<unsigned int>(nBound))
        slot->Use();
}

// Vertex-program env params only; the ranges are the engine's ARB vertex programs' layout.
void android_port_glProgramEnvParameter4fARB(unsigned int target, unsigned int i, float x, float y, float z,
                                             float w)
{
    if (k1gl_tracing) k1gl_trace("glProgramEnvParameter4fARB(%#x, %u, %g, %g, %g, %g)", target, i, x, y, z, w);
    if (target != GL_VERTEX_PROGRAM_ARB)
        return;
    float* dst;
    if (i - 0x12 < 0x33) {
        dst = &s_boneMatrix[(i - 0x12) * 4];
    } else if (i - 0x59 < 4) {
        // The camera matrix's rows arrive in reverse: the first param is row 3.
        dst = &s_cameraMatrix[(0x5c - i) * 4];
        ++s_nCameraSerial;
    } else if (i - 0x49 < 0xf) {
        dst = &s_lightData[(i - 0x49) * 4];
    } else if (i - 0x5d < 3) {
        dst = &s_lightMaterialData[(i - 0x5d) * 4];
#ifdef K2_QOL_BUMPMAPS
    } else if (i - 12 < 3) {
        dst = &s_bumpParams[(i - 12) * 4];
#endif
    } else {
        return;
    }
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
}

// Fragment-program env params 0..5, although only rows 0..1 reach the shader.
void android_port_glProgramEnvParameter4fvARB(unsigned int target, unsigned int i, float const* v)
{
    if (k1gl_tracing) k1gl_trace("glProgramEnvParameter4fvARB(%#x, %u, %g, %g, %g, %g)", target, i, v[0], v[1], v[2], v[3]);
    if (target != GL_FRAGMENT_PROGRAM_ARB || i > 5)
        return;
    memcpy(&s_fragmentShaderParams[i * 4], v, 4 * sizeof(float));
    if (s_pCurrent)
        s_pCurrent->SetFragmentShaderParams(s_fragmentShaderParams);
}

void android_port_glEnable(unsigned int cap)
{
    if (k1gl_tracing) k1gl_trace("glEnable(%#x)", cap);
    switch (cap) {
    case GL_ALPHA_TEST:
        s_fAlphaTestRef = s_fAlphaRef;
        return;
    case GL_FOG:
        if (s_bFogDisabled) {
            s_bFogDisabled = false;
            s_fogData[0] = s_lastSetFogData[0];
            s_fogData[1] = s_lastSetFogData[1];
            ++s_nFogSerial;
        }
        return;
    case GL_VERTEX_PROGRAM_ARB:
    case GL_FRAGMENT_PROGRAM_ARB:
        return;   // host: see above
    case 0x0de1:  // GL_TEXTURE_2D
    case 0x8513:  // GL_TEXTURE_CUBE_MAP
        s_bFixedFunctionUnitsDirty = true;
        break;
    }
    k2gl::glEnable(cap);
}

void android_port_glDisable(unsigned int cap)
{
    if (k1gl_tracing) k1gl_trace("glDisable(%#x)", cap);
    switch (cap) {
    case GL_ALPHA_TEST:
        s_fAlphaTestRef = -1.0f;
        return;
    case GL_FOG:
        // Fog "off" is a start/end pair no fragment reaches, not a state.
        if (!s_bFogDisabled) {
            s_bFogDisabled = true;
            s_fogData[0] = 10000.0f;
            s_fogData[1] = 10001.0f;
            ++s_nFogSerial;
        }
        return;
    case GL_VERTEX_PROGRAM_ARB:
    case GL_FRAGMENT_PROGRAM_ARB:
        return;   // host: see above
    }
    k2gl::glDisable(cap);
}

// gles2-bc's glIsEnabled knows neither program capability.
unsigned char android_port_glIsEnabled(unsigned int cap)
{
    if (cap == GL_VERTEX_PROGRAM_ARB || cap == GL_FRAGMENT_PROGRAM_ARB || cap == GL_FOG || cap == GL_ALPHA_TEST)
        return 0;
    return k2gl::glIsEnabled(cap);
}

// Stored for the next glEnable(GL_FOG); 0.0001 keeps end != start (the shader divides).
void android_port_glFogf(unsigned int pname, float value)
{
    if (k1gl_tracing) k1gl_trace("glFogf(%#x, %g)", pname, value);
    if (pname == GL_FOG_END)
        s_lastSetFogData[1] = value + 0.0001f;
    else if (pname == GL_FOG_START)
        s_lastSetFogData[0] = value;
    else
        return;
    ++s_nFogSerial;
}

void android_port_glFogfv(unsigned int pname, float const* v)
{
    if (pname != GL_FOG_COLOR)
        return;
    memcpy(s_fogColor, v, sizeof(s_fogColor));
    ++s_nFogSerial;
}

// The device's own test is inverted: a reference set while alpha testing is off switches the
// shader's reference on (until the next glDisable(GL_ALPHA_TEST)). Kept.
void android_port_glAlphaFunc(unsigned int func, float ref)
{
    if (k1gl_tracing) k1gl_trace("glAlphaFunc(%u, %g)", func, static_cast<double>(ref));
    s_fAlphaRef = ref;
    if (s_fAlphaTestRef < 0.0f)
        s_fAlphaTestRef = ref;
    k2gl::glAlphaFunc(func, ref);
}

// NV_vertex_program's per-attribute arrays are dropped, like the device.
void android_port_glEnableClientState(unsigned int array)
{
    if (k1gl_tracing) k1gl_trace("glEnableClientState(%u)", array);
    if (array >> 4 == 0x865)
        return;
    k2gl::glEnableClientState(array);
}

void android_port_glDisableClientState(unsigned int array)
{
    if (k1gl_tracing) k1gl_trace("glDisableClientState(%u)", array);
    if (array >> 4 == 0x865)
        return;
    k2gl::glDisableClientState(array);
}

// The ARB vertex programs' weight (1) and index (4) streams; KotorProgram binds them.
void android_port_glVertexAttribPointerARB(unsigned int index, int size, unsigned int type, unsigned char,
                                           int stride, void const* pointer)
{
    if (k1gl_tracing) k1gl_trace("glVertexAttribPointerARB(%u, %d, %#x, %d, %p)", index, size, type, stride, pointer);
    AttributeData* a = index == 1 ? &s_weights : index == 4 ? &s_indices : 0;
#ifdef K2_QOL_BUMPMAPS
    if (index - 12 < 3)
        a = &s_bumpFrame[index - 12];
#endif
    if (!a)
        return;
    a->m_nSize = size;
    a->m_nType = type;
    a->m_nStride = stride;
    a->m_pPointer = pointer;
}

// Attribute 1 carries the whole skinning pair.
void android_port_glEnableVertexAttribArrayARB(unsigned int index)
{
    if (k1gl_tracing) k1gl_trace("glEnableVertexAttribArrayARB(%u)", index);
    if (index == 1)
        s_bSkinning = s_weights.m_bIsPointer = s_indices.m_bIsPointer = true;
#ifdef K2_QOL_BUMPMAPS
    if (index - 12 < 3)
        s_bumpFrame[index - 12].m_bIsPointer = true;
#endif
}

void android_port_glDisableVertexAttribArrayARB(unsigned int index)
{
    if (k1gl_tracing) k1gl_trace("glDisableVertexAttribArrayARB(%u)", index);
    if (index == 1)
        s_bSkinning = s_weights.m_bIsPointer = s_indices.m_bIsPointer = false;
#ifdef K2_QOL_BUMPMAPS
    if (index - 12 < 3)
        s_bumpFrame[index - 12].m_bIsPointer = false;
#endif
}

// The device also splits GL_QUADS into triangles; desktop GL and gl4es draw quads.
void android_port_glDrawArrays(unsigned int mode, int first, int count)
{
    if (k1gl_tracing) k1gl_trace("glDrawArrays(%u, %d, %d)%s", mode, first, count, s_pCurrent ? " [kotor]" : "");
    SetupShader();
    if (k1gl_tracing) TraceArrays(s_pCurrent != 0);
    k2shim_glarr_caller = __builtin_return_address(0);
    k2gl::glDrawArrays(mode, first, count);
}

void android_port_glDrawElements(unsigned int mode, int count, unsigned int type, void const* indices)
{
    if (k1gl_tracing) k1gl_trace("glDrawElements(%u, %d, %#x, %p)%s", mode, count, type, indices, s_pCurrent ? " [kotor]" : "");
    SetupShader();
    if (k1gl_tracing) TraceArrays(s_pCurrent != 0);
    k2shim_glarr_caller = __builtin_return_address(0);
    k2gl::glDrawElements(mode, count, type, indices);
}

// Host only: uniforms cannot change inside glBegin/glEnd, and the device's glEnd draws through
// android_port_glDrawArrays, so the setup runs here.
void android_port_glBegin(unsigned int mode)
{
    if (k1gl_tracing) k1gl_trace("glBegin(%u)%s", mode, s_pCurrent ? " [kotor]" : "");
    SetupShader();
    if (k1gl_tracing) TraceArrays(s_pCurrent != 0);
    k2gl::glBegin(mode);
}

} // extern "C"
