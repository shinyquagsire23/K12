// shim_kotorgl.cpp -- the engine's ASLgl "Kotor2" state layer, on desktop GL.
//
// The Android engine never runs the PC's ARB fragment/vertex programs. Its ASLgl layer swallows
// glGenProgramsARB/glProgramStringARB (both no-ops), and glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, id)
// turns the engine's fragment-program id into a `ProgramType` flag word and binds a GLSL
// `KotorProgram` (kotor.vert/kotor.frag from the OBB, built with `#define USE_*` lines for those
// flags). The vertex side that the ARB vertex programs used to read (bones, camera, lights,
// material) arrives through glProgramEnvParameter4f[v]ARB. This layer keeps it in shadow arrays and
// uploads it as uniforms in SetupShader() before every draw. Fog and alpha test likewise stop being
// GL state and become uniforms.
//
// KotorProgram/ProgramType are engine code (src/game/KotorProgram). This file holds only the ASLgl
// entry points around them. Three things differ from the Android build, because the target is a
// desktop GL 2.1 compatibility context rather than gles2-bc on GLES 2:
//   * glShaderSource rewrites the GLES 2 GLSL for GLSL 1.20: a #version line, empty precision
//     qualifiers, and the fixed-function attributes (a_position, a_normal, a_color, a_texCoord0/1)
//     mapped to the built-ins (gl_Vertex, ...). Android feeds those attributes from its own
//     shadows of glVertexPointer & co., and emulates glBegin/glEnd with arrays. On desktop GL the
//     built-ins read the engine's native client arrays AND immediate mode directly, so only the
//     skinning weights/indices (generic attributes 1 and 4) still go through the shadows.
//   * The model-view/projection come from the native matrix stack (glGetFloatv), not gles2-bc's.
//   * glBegin runs the same setup as a draw call: immediate-mode draws on Android reach
//     glDrawArrays through its glEnd emulation, which always has the colour array enabled.
// Android also forwards glEnable/glDisable(GL_VERTEX_PROGRAM_ARB / GL_FRAGMENT_PROGRAM_ARB) to
// gles2-bc, which has no such capability. Here they are dropped instead: no ARB program is ever
// loaded, so enabling one natively would make every draw fail.
#include <inttypes.h>
#include "aslgl.gen.h"
#include "k2shim.h"

#include "KotorProgram.h"
#include "aurglcore.h"

#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <dlfcn.h>
#include <map>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

// Engine globals this layer reads (defined by the engine).
extern float s_vertexShaderParams[4];         // aslgl.cpp
#if !KOTOR1
float s_vertexShaderParams[4];
#endif
extern bool g_illumShader;                    // GLRenderFragmentPrograms.cpp
#if !KOTOR1
bool g_illumShader;
#endif
extern GLRender::PerInstanceColor g_illumShaderValues;   // auraticore.cpp
#if !KOTOR1
GLRender::PerInstanceColor g_illumShaderValues;
#endif
extern unsigned char g_useDistortionOutput;   // aurglcore.cpp
#if !KOTOR1
unsigned char g_useDistortionOutput;
#endif

namespace {

enum {
    GL_LIGHTING = 0x0b50, GL_FOG = 0x0b60, GL_FOG_START = 0x0b63, GL_FOG_END = 0x0b64,
    GL_FOG_COLOR = 0x0b66, GL_ALPHA_TEST = 0x0bc0, GL_GREATER = 0x0204, GL_GEQUAL = 0x0206,
    GL_COLOR_ARRAY = 0x8076, GL_VERTEX_PROGRAM_ARB = 0x8620, GL_FRAGMENT_PROGRAM_ARB = 0x8804,
    GL_MODELVIEW_MATRIX = 0x0ba6, GL_PROJECTION_MATRIX = 0x0ba7, GL_CURRENT_PROGRAM = 0x8b8d,
    GL_BLEND = 0x0be2, GL_BLEND_DST_RGB = 0x80c8, GL_ONE = 1,
};

// Client-array shadows. Only the colour array's enable flag and the two skinning attributes are
// still read (see above), so those are all that is kept.
bool s_bColorArray;                   // the engine's colour-array enable flag
AttributeData s_weights;              // generic attribute 1
AttributeData s_indices;              // generic attribute 4
AttributeData s_unused;               // stands in for the shadows the built-ins replaced
bool s_bSkinning;                     // glEnableVertexAttribArrayARB(1)
bool s_bLightingDisabled;
bool s_bFogDisabled;
bool s_useAlphaTestShader;

float s_fogColor[3];
float s_fogData[4];
float s_lastSetFogData[4];
float s_boneMatrix[0x33 * 4];         // vertex env params, three rows per bone
float s_lightData[15 * 4];
float s_lightMaterialData[4 * 4];     // row 3 holds the per-instance illumination
float s_fragmentShaderParams[8 * 4];
float s_cameraMatrix[16];
int s_nFogSerial;
float s_fAlphaRef;
int s_nCameraSerial;

std::map<unsigned int, KotorProgram*> s_Programs;   // keyed by ProgramType::m_nFlags
KotorProgram* s_pCurrent;

// Binds the current program for a draw; bColorArray is the colour-array flag it reads (glBegin
// passes true, see above).
void SetupShader(bool bColorArray)
{
    KotorProgram* p = s_pCurrent;
    if (!p)
        return;

    p->SetAttributes(s_unused, s_unused, s_unused, bColorArray && s_bLightingDisabled, s_unused,
                     s_unused, s_weights, s_indices);

    // u_matrices = { MV, MVP }.
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
    if (k2shim_gl_tracing) {
        // Traces the matrices this draw goes out with (column-major, as GL keeps them).
        auto dump = [](const char* pszName, const float* f) {
            fprintf(stderr, "k2: trace:   %s", pszName);
            for (int i = 0; i < 16; i++) fprintf(stderr, " %g", f[i]);
            fprintf(stderr, "\n");
        };
        dump("mv", m);
        dump("proj", proj);
        dump("camera", s_cameraMatrix);
        dump("bone0", s_boneMatrix);
        {
            // What the program itself holds and binds (gl4es's view of it).
            typedef int (K2_GLAPI* GetLoc)(unsigned int, const char*);
            typedef void (K2_GLAPI* GetUniformfv)(unsigned int, int, float*);
            static GetLoc s_pGetUniformLocation = reinterpret_cast<GetLoc>(k2shim_gl_proc("glGetUniformLocation"));
            static GetLoc s_pGetAttribLocation = reinterpret_cast<GetLoc>(k2shim_gl_proc("glGetAttribLocation"));
            static GetUniformfv s_pGetUniformfv = reinterpret_cast<GetUniformfv>(k2shim_gl_proc("glGetUniformfv"));
            int nProg = 0;
            k2gl::glGetIntegerv(GL_CURRENT_PROGRAM, &nProg);
            if (s_pGetUniformLocation && s_pGetUniformfv && s_pGetAttribLocation && nProg) {
                int nMvp = s_pGetUniformLocation(static_cast<unsigned>(nProg), "u_matrices[1]");
                float afMvp[16] = {0};
                if (nMvp >= 0) s_pGetUniformfv(static_cast<unsigned>(nProg), nMvp, afMvp);
                fprintf(stderr, "k2: trace:   program %d: u_matrices[1] loc %d =", nProg, nMvp);
                for (int i = 0; i < 16; i++) fprintf(stderr, " %g", afMvp[i]);
                fprintf(stderr, " | a_position %d a_normal %d a_texCoord0 %d a_matrixWeights %d a_matrixIndices %d\n",
                        s_pGetAttribLocation(static_cast<unsigned>(nProg), "a_position"),
                        s_pGetAttribLocation(static_cast<unsigned>(nProg), "a_normal"),
                        s_pGetAttribLocation(static_cast<unsigned>(nProg), "a_texCoord0"),
                        s_pGetAttribLocation(static_cast<unsigned>(nProg), "a_matrixWeights"),
                        s_pGetAttribLocation(static_cast<unsigned>(nProg), "a_matrixIndices"));
                fprintf(stderr, "k2: trace:   expected MVP =");
                for (int i = 16; i < 32; i++) fprintf(stderr, " %g", m[i]);
                fprintf(stderr, "\n");
            }
        }
        fprintf(stderr, "k2: trace:   shell %g | lightmat", s_vertexShaderParams[0]);
        for (int i = 0; i < 16; i++) fprintf(stderr, " %g", s_lightMaterialData[i]);
        fprintf(stderr, " | fog %g %g %g %g | light0", s_fogData[0], s_fogData[1], s_fogData[2], s_fogData[3]);
        for (int i = 0; i < 20; i++) fprintf(stderr, " %g", s_lightData[i]);
        fprintf(stderr, "\n");
    }

    if (p->m_nCameraSerial != s_nCameraSerial) {
        p->SetCamera(s_cameraMatrix);
        p->m_nCameraSerial = s_nCameraSerial;
    }
    p->SetSkinMatrices(s_boneMatrix);
    p->SetLightData(s_lightData);
    if (g_illumShader)
        memcpy(&s_lightMaterialData[12], &g_illumShaderValues, sizeof(g_illumShaderValues));
    if (!s_bLightingDisabled)
        p->SetLightMaterialData(s_lightMaterialData);
#ifdef K2_QOL_IMPROVEMENTS
    // An additive draw (destination factor GL_ONE: light shafts, glows, blades) fogs toward black.
    // Fogged toward the fog colour, a distant glow adds a flat fog-coloured sheet over everything
    // behind it instead of fading out. The serial's low bit keys the colour the program holds.
    int nDst = 0;
    bool bAdditive = k2gl::glIsEnabled(GL_BLEND) &&
                     (k2gl::glGetIntegerv(GL_BLEND_DST_RGB, &nDst), nDst == GL_ONE);
    int nFogKey = s_nFogSerial * 2 + (bAdditive ? 1 : 0);
    if (p->m_nFogSerial != nFogKey) {
        static float s_black[3] = { 0.0f, 0.0f, 0.0f };
        p->SetFogData(s_fogData, bAdditive ? s_black : s_fogColor);
        p->m_nFogSerial = nFogKey;
    }
#else
    if (p->m_nFogSerial != s_nFogSerial) {
        p->SetFogData(s_fogData, s_fogColor);
        p->m_nFogSerial = s_nFogSerial;
    }
#endif
    s_fragmentShaderParams[11] = s_fAlphaRef;   // u_fragmentShaderParams[2].w: the alpha-test ref
    s_pCurrent->SetFragmentShaderParams(s_fragmentShaderParams);
    s_pCurrent->SetVertexShaderParams(s_vertexShaderParams);

    // K2_GL_CHECK: name the first program whose unit-0 texture has no level-0 image (GL samples it
    // as black; Apple's driver only says "unit 0 ... is unloadable").
    static bool s_bReportedEmpty;
    if (k2shim_gl_checking && !s_bReportedEmpty) {
        int nTexture = 0, nWidth = 0;
        k2gl::glActiveTexture(0x84c0);   // GL_TEXTURE0
        k2gl::glGetIntegerv(0x8069, &nTexture);   // GL_TEXTURE_BINDING_2D
        k2gl::glGetTexLevelParameteriv(0x0de1, 0, 0x1000, &nWidth);   // GL_TEXTURE_WIDTH
        if (nWidth == 0) {
            fprintf(stderr, "k2: KotorProgram flags 0x%x draws with texture %d on unit 0, which has no image\n",
                    p->m_nFlags, nTexture);
            s_bReportedEmpty = true;
        }
    }
}

void UnbindCurrent()
{
    if (s_pCurrent) {
        s_pCurrent->Unuse();
        s_pCurrent = 0;
        k2shim_glarr_attribs_off();
    }
}

// GLES 2 GLSL -> GLSL 1.20 (see above). Line-based; the engine's shaders put every precision
// statement and attribute declaration on its own line.
std::string DesktopGLSL(const std::string& src)
{
    static const char* const kBuiltins[][2] = {
        {"a_position", "gl_Vertex"}, {"a_normal", "gl_Normal"}, {"a_color", "gl_Color"},
        {"a_texCoord0", "gl_MultiTexCoord0"}, {"a_texCoord1", "gl_MultiTexCoord1"},
    };
    std::string out = "#version 120\n#define lowp\n#define mediump\n#define highp\n";
    for (auto& b : kBuiltins)
        out += std::string("#define ") + b[0] + " " + b[1] + "\n";
    size_t pos = 0;
    while (pos < src.size()) {
        size_t end = src.find('\n', pos);
        if (end == std::string::npos)
            end = src.size();
        std::string line = src.substr(pos, end - pos);
        pos = end + 1;
        size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos) {
            const char* l = line.c_str() + first;
            if (!strncmp(l, "precision ", 10))
                continue;
            bool builtin = false;
            if (!strncmp(l, "attribute ", 10))
                for (auto& b : kBuiltins) {
                    const char* n = strstr(l, b[0]);
                    size_t len = strlen(b[0]);
                    if (n && (n[len] == ';' || n[len] == ' ' || n[len] == '\t'))
                        builtin = true;
                }
            if (builtin)
                continue;
        }
        out += line;
        out += '\n';
    }
    return out;
}

} // namespace

namespace ASLgl {

void glBindProgramARB(unsigned int target, unsigned int id)
{
    if (target != GL_FRAGMENT_PROGRAM_ARB)
        return;
    if (id == 0) {
        UnbindCurrent();
        return;
    }
    static const bool s_bNoAlphaTest = getenv("K2_GL_NOALPHATEST") != 0;   // test: the alpha-test variants off
    ProgramType type(id, s_bSkinning, s_useAlphaTestShader && !s_bNoAlphaTest, g_illumShader,
                     g_useDistortionOutput);
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

// No-op: nothing ever loads an ARB program.
void glGenProgramsARB(int, unsigned int*) {}
void glProgramStringARB(unsigned int, unsigned int, int, void const*) {}
void glDeleteProgramsARB(int, unsigned int const*) {}

// Vertex-program env params only; the ranges are the ARB vertex programs' own layout.
void glProgramEnvParameter4fARB(unsigned int target, unsigned int i, float x, float y, float z, float w)
{
    if (target != GL_VERTEX_PROGRAM_ARB)
        return;
    float* dst;
    if (i - 0x12 < 0x33) {
        dst = &s_boneMatrix[(i - 0x12) * 4];
    } else if (i - 0x59 < 4) {
        // The camera matrix's rows arrive in reverse, so the first param is row 3.
        dst = &s_cameraMatrix[(0x5c - i) * 4];
        ++s_nCameraSerial;
    } else if (i - 0x49 < 0xf) {
        dst = &s_lightData[(i - 0x49) * 4];
    } else if (i - 0x5d < 3) {
        dst = &s_lightMaterialData[(i - 0x5d) * 4];
    } else {
        return;
    }
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
}

// Fragment-program env params only.
void glProgramEnvParameter4fvARB(unsigned int target, unsigned int i, float const* v)
{
    if (target != GL_FRAGMENT_PROGRAM_ARB || i >= 8)
        return;
    memcpy(&s_fragmentShaderParams[i * 4], v, 4 * sizeof(float));
    if (s_pCurrent)
        s_pCurrent->SetFragmentShaderParams(s_fragmentShaderParams);
}

void glEnable(unsigned int cap)
{
    switch (cap) {
    case GL_ALPHA_TEST:
        s_useAlphaTestShader = true;
        return;
    case GL_FOG:
        if (s_bFogDisabled) {
            s_bFogDisabled = false;
            s_fogData[0] = s_lastSetFogData[0];
            s_fogData[1] = s_lastSetFogData[1];
            ++s_nFogSerial;
        }
        return;
    case GL_LIGHTING:
        s_bLightingDisabled = false;
        break;
    case GL_VERTEX_PROGRAM_ARB:
    case GL_FRAGMENT_PROGRAM_ARB:
        return;   // host: see above
    }
    k2gl::glEnable(cap);
}

void glDisable(unsigned int cap)
{
    switch (cap) {
    case GL_ALPHA_TEST:
        s_useAlphaTestShader = false;
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
    case GL_FRAGMENT_PROGRAM_ARB:
        UnbindCurrent();
        return;
    case GL_VERTEX_PROGRAM_ARB:
        return;   // host: see above
    case GL_LIGHTING:
        s_bLightingDisabled = true;
        break;
    }
    k2gl::glDisable(cap);
}

// Stored for the next glEnable(GL_FOG); 0.0001 keeps end != start (the shader divides).
void glFogf(unsigned int pname, float value)
{
    if (pname == GL_FOG_END)
        s_lastSetFogData[1] = value + 0.0001f;
    else if (pname == GL_FOG_START)
        s_lastSetFogData[0] = value;
    else
        return;
    ++s_nFogSerial;
}

void glFogfv(unsigned int pname, float const* v)
{
    if (pname != GL_FOG_COLOR)
        return;
    memcpy(s_fogColor, v, sizeof(s_fogColor));
    ++s_nFogSerial;
}

// Only the GREATER/GEQUAL reference reaches the shader's discard test.
void glAlphaFunc(unsigned int func, float ref)
{
    s_fAlphaRef = (func == GL_GREATER || func == GL_GEQUAL) ? ref : 0.0f;
    k2gl::glAlphaFunc(func, ref);
}

// NV_vertex_program's per-attribute arrays are dropped, as on Android.
void glEnableClientState(unsigned int array)
{
    if (array >> 4 == 0x865)
        return;
    if (array == GL_COLOR_ARRAY)
        s_bColorArray = true;
    k2gl::glEnableClientState(array);
}

void glDisableClientState(unsigned int array)
{
    if (array >> 4 == 0x865)
        return;
    if (array == GL_COLOR_ARRAY)
        s_bColorArray = false;
    k2gl::glDisableClientState(array);
}

// The shipped layer's is empty. The bump passes (aurmodelrender.cpp) bracket their arrays with these
// and switch the normal and colour arrays back on just before the pop, so without the stack the colour
// array stayed enabled on whatever pointer was set last (and the colour flag stayed set).
static std::vector<bool> s_colorArrayStack;

void glPushClientAttrib(unsigned int mask)
{
    s_colorArrayStack.push_back(s_bColorArray);
    k2gl::glPushClientAttrib(mask);
}

void glPopClientAttrib()
{
    k2gl::glPopClientAttrib();
    if (!s_colorArrayStack.empty()) {
        s_bColorArray = s_colorArrayStack.back();
        s_colorArrayStack.pop_back();
    }
}

// The ARB vertex programs' weight (1) and index (4) streams; KotorProgram binds them.
void glVertexAttribPointerARB(unsigned int index, int size, unsigned int type, unsigned char,
                              int stride, void const* pointer)
{
    AttributeData* a = index == 1 ? &s_weights : index == 4 ? &s_indices : 0;
    if (!a)
        return;
    a->m_nSize = size;
    a->m_nType = type;
    a->m_nStride = stride;
    a->m_pPointer = pointer;
}

// Attribute 1 carries the whole skinning pair.
void glEnableVertexAttribArrayARB(unsigned int index)
{
    if (index == 1)
        s_bSkinning = s_weights.m_bIsPointer = s_indices.m_bIsPointer = true;
}

void glDisableVertexAttribArrayARB(unsigned int index)
{
    if (index == 1)
        s_bSkinning = s_weights.m_bIsPointer = s_indices.m_bIsPointer = false;
}

// K2_GL_TRACE: one line per draw (the engine caller, the texture, colour and blend state) plus the
// colour at the viewport's centre before the draw, so the first line showing a new value names the draw
// just before it.
static void TraceDraw(const char* pszWhat, unsigned int mode, int count, void* pCaller)
{
    Dl_info info;
    const char* pszCaller = "?";
    std::string demangled;
    if (dladdr(pCaller, &info) && info.dli_sname) {
#ifdef _MSC_VER   // no Itanium demangler; the MSVC names stay as they are
        demangled = info.dli_sname;
#else
        int status = 0;
        char* d = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
        demangled = d && status == 0 ? d : info.dli_sname;
        free(d);
#endif
        pszCaller = demangled.c_str();
    }
    int tex = 0, w = 0, h = 0, prog = 0, src = 0, dst = 0, vp[4] = {0};
    float c[4];
    unsigned char px[4] = {0};
    k2gl::glGetIntegerv(0x8069, &tex);   // GL_TEXTURE_BINDING_2D
    k2gl::glGetTexLevelParameteriv(0x0de1, 0, 0x1000, &w);
    k2gl::glGetTexLevelParameteriv(0x0de1, 0, 0x1001, &h);
    k2gl::glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    k2gl::glGetIntegerv(0x0be1, &src);
    k2gl::glGetIntegerv(0x0be0, &dst);
    k2gl::glGetIntegerv(0x0ba2, vp);     // GL_VIEWPORT
    k2gl::glGetFloatv(0x0b00, c);        // GL_CURRENT_COLOR
    k2gl::glReadPixels(vp[0] + vp[2] / 2, vp[1] + vp[3] / 2, 1, 1, 0x1908, 0x1401, px);
    // How many pixels of the viewport the previous draw changed (same viewport only; -1 otherwise),
    // and the bounding box of the change: whether a draw reached the screen at all, and where.
    static std::vector<unsigned char> s_prev;
    static int s_prevVp[4];
    std::vector<unsigned char> cur(static_cast<size_t>(vp[2] > 0 ? vp[2] : 0) * (vp[3] > 0 ? vp[3] : 0) * 4);
    if (!cur.empty()) k2gl::glReadPixels(vp[0], vp[1], vp[2], vp[3], 0x1908, 0x1401, cur.data());
    int64_t nChanged = -1;
    int x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1;
    if (memcmp(s_prevVp, vp, sizeof vp) == 0 && s_prev.size() == cur.size()) {
        nChanged = 0;
        for (size_t i = 0; i < cur.size(); i += 4)
            if (memcmp(&cur[i], &s_prev[i], 3) != 0) {
                ++nChanged;
                int x = static_cast<int>((i / 4) % vp[2]), y = static_cast<int>((i / 4) / vp[2]);
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    }
    s_prev.swap(cur);
    memcpy(s_prevVp, vp, sizeof vp);
    {
        int nDepthFunc = 0, nStencilFunc = 0;
        unsigned char bDepthMask = 0, abColorMask[4] = {0};
        float fClearDepth = 0, afDepthRange[2] = {0};
        typedef void (K2_GLAPI* GetBooleanv)(unsigned int, unsigned char*);
        static GetBooleanv s_pGetBooleanv = reinterpret_cast<GetBooleanv>(k2shim_gl_proc("glGetBooleanv"));
        k2gl::glGetIntegerv(0x0b74, &nDepthFunc);              // GL_DEPTH_FUNC
        if (s_pGetBooleanv) s_pGetBooleanv(0x0b72, &bDepthMask);              // GL_DEPTH_WRITEMASK
        if (s_pGetBooleanv) s_pGetBooleanv(0x0c23, abColorMask);              // GL_COLOR_WRITEMASK
        k2gl::glGetFloatv(0x0b73, &fClearDepth);               // GL_DEPTH_CLEAR_VALUE
        k2gl::glGetFloatv(0x0b70, afDepthRange);               // GL_DEPTH_RANGE
        k2gl::glGetIntegerv(0x0b92, &nStencilFunc);            // GL_STENCIL_FUNC
        fprintf(stderr, "k2: trace:   depth func %x mask %d clear %g range %g..%g | colour mask %d%d%d%d | "
                        "stencil %s func %x | cull %s\n",
                nDepthFunc, bDepthMask, fClearDepth, afDepthRange[0], afDepthRange[1], abColorMask[0],
                abColorMask[1], abColorMask[2], abColorMask[3], k2gl::glIsEnabled(0x0b90) ? "on" : "off",
                nStencilFunc, k2gl::glIsEnabled(0x0b44) ? "on" : "off");
        {
            static const unsigned int kCaps[] = {
                0x0b10, 0x0b20, 0x0b42, 0x0b44, 0x0b50, 0x0b57, 0x0b60, 0x0b71, 0x0b90, 0x0bc0, 0x0be2,
                0x0bf2, 0x0c11, 0x0c60, 0x0c61, 0x0c62, 0x0c63, 0x0d80, 0x0de1, 0x3000, 0x3001, 0x3002,
                0x3003, 0x3004, 0x3005, 0x4000, 0x4001, 0x4002, 0x4003, 0x8037, 0x803a, 0x8061, 0x809d,
                0x809e, 0x809f, 0x80a0, 0x8513, 0x8861, 0x0bd0, 0x8642, 0x806f,
            };
            fprintf(stderr, "k2: trace:   enabled");
            for (unsigned int c : kCaps)
                if (k2gl::glIsEnabled(c)) fprintf(stderr, " %x", c);
            float fCoverage = -1;
            int bInvert = 0;
            k2gl::glGetFloatv(0x80aa, &fCoverage);    // GL_SAMPLE_COVERAGE_VALUE
            k2gl::glGetIntegerv(0x80ab, &bInvert);    // GL_SAMPLE_COVERAGE_INVERT
            float afPolyOffset[2] = {0, 0};
            k2gl::glGetFloatv(0x8038, &afPolyOffset[0]);   // GL_POLYGON_OFFSET_FACTOR
            k2gl::glGetFloatv(0x2a00, &afPolyOffset[1]);   // GL_POLYGON_OFFSET_UNITS
            fprintf(stderr, " | coverage %g invert %d | polygon offset %g %g\n", fCoverage, bInvert,
                    afPolyOffset[0], afPolyOffset[1]);
        }
        int nFrontFace = 0, nCullMode = 0;
        k2gl::glGetIntegerv(0x0b46, &nFrontFace);   // GL_FRONT_FACE
        k2gl::glGetIntegerv(0x0b45, &nCullMode);    // GL_CULL_FACE_MODE
        fprintf(stderr, "k2: trace:   front face %x cull mode %x\n", nFrontFace, nCullMode);
    }
    if (s_pCurrent) {
        // A shader draw: its vertex attribute arrays (enabled, size, stride, buffer, pointer) and the
        // client position array gl4es aliases onto attribute 0.
        typedef void (K2_GLAPI* GetAttribiv)(unsigned int, unsigned int, int*);
        typedef void (K2_GLAPI* GetAttribPointerv)(unsigned int, unsigned int, void**);
        static GetAttribiv s_pGetAttribiv = reinterpret_cast<GetAttribiv>(k2shim_gl_proc("glGetVertexAttribiv"));
        static GetAttribPointerv s_pGetAttribPointerv =
            reinterpret_cast<GetAttribPointerv>(k2shim_gl_proc("glGetVertexAttribPointerv"));
        fprintf(stderr, "k2: trace:   alpha test %s ref %g (shader %d)\n", k2gl::glIsEnabled(0x0bc0) ? "on" : "off",
                static_cast<double>(s_fAlphaRef), static_cast<int>(s_useAlphaTestShader));
        {
            // Unit 0's texture: filters, wrap and the mip chain (an incomplete one samples as 0).
            typedef void (K2_GLAPI* GetTexParameteriv)(unsigned int, unsigned int, int*);
            static GetTexParameteriv s_pGetTexParameteriv =
                reinterpret_cast<GetTexParameteriv>(k2shim_gl_proc("glGetTexParameteriv"));
            int nMin = 0, nMag = 0, nWrapS = 0, nWrapT = 0, nMaxLevel = 0, nFormat = 0;
            if (s_pGetTexParameteriv) {
                s_pGetTexParameteriv(0x0de1, 0x2801, &nMin);
                s_pGetTexParameteriv(0x0de1, 0x2800, &nMag);
                s_pGetTexParameteriv(0x0de1, 0x2802, &nWrapS);
                s_pGetTexParameteriv(0x0de1, 0x2803, &nWrapT);
                s_pGetTexParameteriv(0x0de1, 0x813d, &nMaxLevel);   // GL_TEXTURE_MAX_LEVEL
            }
            k2gl::glGetTexLevelParameteriv(0x0de1, 0, 0x1003, &nFormat);   // GL_TEXTURE_INTERNAL_FORMAT
            fprintf(stderr, "k2: trace:   tex0 min %x mag %x wrap %x/%x maxlevel %d format %x levels", nMin, nMag,
                    nWrapS, nWrapT, nMaxLevel, nFormat);
            for (int l = 0; l < 12; l++) {
                int lw = 0, lh = 0;
                k2gl::glGetTexLevelParameteriv(0x0de1, l, 0x1000, &lw);
                k2gl::glGetTexLevelParameteriv(0x0de1, l, 0x1001, &lh);
                if (!lw) break;
                fprintf(stderr, " %dx%d", lw, lh);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "k2: trace:   vertex array %s; attribs", k2gl::glIsEnabled(0x8074) ? "on" : "off");
        for (unsigned int a = 0; a < 9 && s_pGetAttribiv && s_pGetAttribPointerv; a++) {
            int bOn = 0, nSize = 0, nStride = 0, nBuffer = 0;
            void* ptr = 0;
            s_pGetAttribiv(a, 0x8622, &bOn);
            if (!bOn) continue;
            s_pGetAttribiv(a, 0x8623, &nSize);
            s_pGetAttribiv(a, 0x8624, &nStride);
            s_pGetAttribiv(a, 0x889f, &nBuffer);
            s_pGetAttribPointerv(a, 0x8645, &ptr);
            fprintf(stderr, " [%u: size %d stride %d buf %d ptr %p", a, nSize, nStride, nBuffer, ptr);
            if (a == 0 && nBuffer == 0 && ptr)
                fprintf(stderr, " v0 %g,%g,%g", static_cast<float*>(ptr)[0], static_cast<float*>(ptr)[1], static_cast<float*>(ptr)[2]);
            fprintf(stderr, "]");
        }
        fprintf(stderr, "\n");
    }
    if (nChanged > 0)
        fprintf(stderr, "k2: trace:   (previous draw changed %" PRId64 " px, x %d..%d y %d..%d of the viewport)\n",
                nChanged, x0, x1, y0, y1);
    else if (nChanged == 0)
        fprintf(stderr, "k2: trace:   (previous draw changed nothing)\n");
    fprintf(stderr, "k2: trace: %-10s mode %u n %d tex %d (%dx%d%s) prog %d flags 0x%x colour %.2f,%.2f,%.2f,%.2f "
                    "blend %s %x/%x carr %d | centre %d,%d,%d | %s\n",
            pszWhat, mode, count, tex, w, h, k2gl::glIsEnabled(0x0de1) ? "" : " off", prog,
            s_pCurrent ? static_cast<unsigned int>(s_pCurrent->m_nFlags) : 0u,
            c[0], c[1], c[2], c[3], k2gl::glIsEnabled(0x0be2) ? "on" : "off", src, dst,
            static_cast<int>(s_bColorArray), px[0], px[1], px[2], pszCaller);
}

// Android also splits GL_QUADS into triangles here; desktop GL draws quads.
void glDrawArrays(unsigned int mode, int first, int count)
{
    if (k2shim_gl_tracing) TraceDraw("DrawArrays", mode, count, __builtin_return_address(0));
    SetupShader(s_bColorArray);
    k2shim_glarr_caller = __builtin_return_address(0);
    k2gl::glDrawArrays(mode, first, count);
}

void glDrawElements(unsigned int mode, int count, unsigned int type, void const* indices)
{
    if (k2shim_gl_tracing) {
        TraceDraw("DrawElems", mode, count, __builtin_return_address(0));
        int nElementBuffer = 0, nArrayBuffer = 0;
        k2gl::glGetIntegerv(0x8895, &nElementBuffer);   // GL_ELEMENT_ARRAY_BUFFER_BINDING
        k2gl::glGetIntegerv(0x8894, &nArrayBuffer);     // GL_ARRAY_BUFFER_BINDING
        fprintf(stderr, "k2: trace:   indices type %x ptr %p element buffer %d array buffer %d", type, indices,
                nElementBuffer, nArrayBuffer);
        if (nElementBuffer && type == 0x1403 && count > 0) {
            // The index buffer as gl4es holds it (its CPU copy decides which client vertices it sends).
            typedef void (K2_GLAPI* GetBufferParameteriv)(unsigned int, unsigned int, int*);
            typedef void (K2_GLAPI* GetBufferSubData)(unsigned int, int64_t, int64_t, void*);
            static GetBufferParameteriv s_pGetBufferParameteriv =
                reinterpret_cast<GetBufferParameteriv>(k2shim_gl_proc("glGetBufferParameteriv"));
            static GetBufferSubData s_pGetBufferSubData = reinterpret_cast<GetBufferSubData>(k2shim_gl_proc("glGetBufferSubData"));
            int nSize = 0;
            if (s_pGetBufferParameteriv) s_pGetBufferParameteriv(0x8893, 0x8764, &nSize);   // GL_BUFFER_SIZE
            int bMapped = 0, bArrayMapped = 0;
            if (s_pGetBufferParameteriv) {
                s_pGetBufferParameteriv(0x8893, 0x88bc, &bMapped);       // GL_BUFFER_MAPPED (element)
                s_pGetBufferParameteriv(0x8892, 0x88bc, &bArrayMapped);  // GL_BUFFER_MAPPED (array)
            }
            fprintf(stderr, " size %d mapped %d array mapped %d", nSize, bMapped, bArrayMapped);
            int64_t nOffset = static_cast<int64_t>(reinterpret_cast<uintptr_t>(indices));
            if (s_pGetBufferSubData && nSize >= nOffset + count * 2) {
                std::vector<unsigned short> v(static_cast<size_t>(count));
                s_pGetBufferSubData(0x8893, nOffset, count * 2, v.data());
                unsigned int nMax = 0;
                for (int i = 0; i < count; i++) if (v[i] > nMax) nMax = v[i];
                fprintf(stderr, " first %u %u %u max %u", v[0], v[1], v[2], nMax);
            }
        }
        if (!nElementBuffer && indices && type == 0x1403 && count > 0) {   // GL_UNSIGNED_SHORT, client
            const unsigned short* p = static_cast<const unsigned short*>(indices);
            unsigned int nMax = 0;
            for (int i = 0; i < count; i++) if (p[i] > nMax) nMax = p[i];
            fprintf(stderr, " first %u %u %u max %u", p[0], p[1], p[2], nMax);
        }
        fprintf(stderr, "\n");
    }
    SetupShader(s_bColorArray);
    static const bool s_bNoDepth = getenv("K2_GL_NODEPTH") != 0;   // a test: shader draws without depth test
    bool bDepth = s_bNoDepth && k2gl::glIsEnabled(0x0b71);
    if (bDepth) k2gl::glDisable(0x0b71);
    k2shim_glarr_caller = __builtin_return_address(0);
#ifdef K2P_IOS
    // Traces the driver's own error for the draw (gl4es keeps an error state of its own).
    typedef unsigned int (*DriverGetError)(void);
    static DriverGetError s_pDriverGetError =
        reinterpret_cast<DriverGetError>(dlsym(dlopen("/System/Library/Frameworks/OpenGLES.framework/OpenGLES", RTLD_NOW),
                              "glGetError"));
    if (k2shim_gl_tracing && s_pDriverGetError) {
        unsigned int nBefore = s_pDriverGetError();
        k2gl::glDrawElements(mode, count, type, indices);
        unsigned int nAfter = s_pDriverGetError();
        fprintf(stderr, "k2: trace:   driver error before 0x%x after draw 0x%x\n", nBefore, nAfter);
    } else
#endif
    k2gl::glDrawElements(mode, count, type, indices);
    if (bDepth) k2gl::glEnable(0x0b71);
}

// Host only (see above): uniforms cannot change inside glBegin/glEnd.
void glBegin(unsigned int mode)
{
    if (k2shim_gl_tracing) TraceDraw("Begin", mode, 0, __builtin_return_address(0));
    SetupShader(true);
    k2gl::glBegin(mode);
}

} // namespace ASLgl

// Overrides the driver's own glGetString, which is what the engine's direct calls read
// (glExtensionSupported in aurglcore_ios.cpp calls the plain C import, not ASLgl::glGetString). On
// Android that is a GLES 2 driver, so GL_EXTENSIONS here is what such a driver reports, not the
// desktop (or gl4es) list, which would steer the engine onto NV/ATI/APPLE paths the Android build
// never runs: the two OES names the engine probes (the packed depth/stencil FBO in aurglcore.cpp --
// separate depth and stencil renderbuffers abort Apple's GL-on-Metal) and the texture extensions
// the real driver has. This definition takes the symbol from OpenGL.framework (an object file beats
// a dylib), so gl4es must not export the name (android/gl4es_patch.cmake); k2gl::glGetString still
// reaches the real driver through k2shim_gl_proc.
extern "C" unsigned char const* glGetString(unsigned int name)
{
    const unsigned char* native = k2gl::glGetString(name);
    if (name != 0x1f03)   // GL_EXTENSIONS
        return native;
    static std::string s_extensions;
    if (s_extensions.empty() && native) {
        std::string list = " " + std::string(reinterpret_cast<const char*>(native)) + " ";
        s_extensions = "GL_OES_packed_depth_stencil GL_OES_depth24";
        for (const char* ext : {"GL_EXT_texture_compression_s3tc", "GL_EXT_texture_filter_anisotropic"})
            if (list.find(std::string(" ") + ext + " ") != std::string::npos)
                s_extensions += std::string(" ") + ext;
    }
    return reinterpret_cast<unsigned char const*>(s_extensions.c_str());
}

namespace ASLgl {

// GL_EXTENSIONS is the driver's list (::glGetString above) plus a fixed suffix of the desktop
// extensions the engine's PC paths probe for; its ARB program entries are fake and handled above.
unsigned char const* glGetString(unsigned int name)
{
    const unsigned char* p = ::glGetString(name);
    if (name != 0x1f03) {   // GL_EXTENSIONS
        // A NULL from the driver comes back as "": AurGetVertexProgramARBError reads
        // GL_PROGRAM_ERROR_STRING_ARB's first byte, which GLES (gl4es) does not know.
        return p ? p : reinterpret_cast<const unsigned char*>("");
    }
    static std::string s_extensions;
    if (s_extensions.empty() && p && *p) {
        char buf[0x4000];
        snprintf(buf, sizeof(buf),
                 "%s GL_EXT_bgra GL_ARB_texture_cube_map GL_ARB_vertex_program GL_ARB_fragment_program "
                 "GL_ARB_multitexture GL_NV_register_combiners GL_NV_texture_shader GL_ARB_depth_texture "
                 "GL_ARB_vertex_buffer_object",
                 reinterpret_cast<const char*>(p));
        s_extensions = buf;
    }
    return reinterpret_cast<unsigned char const*>(s_extensions.c_str());
}

// Host only: the OBB's shaders are GLSL ES 1.00 (see DesktopGLSL).
void glShaderSource(unsigned int shader, int count, char const* const* strings, int const* lengths)
{
    std::string src;
    for (int i = 0; i < count; i++)
        src.append(strings[i], lengths && lengths[i] >= 0 ? static_cast<size_t>(lengths[i]) : strlen(strings[i]));
    std::string desktop = DesktopGLSL(src);
    const char* p = desktop.c_str();
    k2gl::glShaderSource(shader, 1, &p, 0);
}

} // namespace ASLgl
