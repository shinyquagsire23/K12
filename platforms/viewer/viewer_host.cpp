// viewer_host.cpp - the model viewer (k1view / k2view): the game's executable with its intro, main
// menu and game loop swapped for one model in a room, a coloured light circling it, and a Dear ImGui
// panel for the model, its animation, its reflection map, the light and the renderer's switches.
//
// The executable is the game's own (platforms/macos/main.cpp finds the game and runs its start-up);
// this file installs itself as g_pK2HostViewer (src/port/k2viewerapi.h) before main, so the engine
// calls Start where it would begin the intro and Frame where it would run a frame of the game. What it
// renders is built by viewer_engine.cpp, the half that may include src/'s headers.
//
// Mouse: left drag orbits, right (or Shift + left) drag pans, the wheel zooms. WASD moves the camera
// (Q / E down and up, Shift faster) while no panel field has the keyboard. Tab hides the panel.
//   K2VIEW_MODEL=<resref>    the model to start with ("none" for none)
//   K2VIEW_ROOM=<layout>     the backdrop to start with (an area's .lyt; "none" for none)
//   K2VIEW_ANIM=<name>       the animation to start with
//   K2VIEW_CAMERA=<yaw>,<pitch>,<distance>   the camera about the model (radians, radians, metres)
//   K2VIEW_TARGET=<x>,<y>,<z>                what the camera looks at, instead of the model
//   K2VIEW_PANEL=0           start with the panel hidden (screenshots)
//   K2VIEW_OFF=<name>,...    renderer switches to start off, by the panel's first word (Shadows, Bumpy-shiny)
//   K2VIEW_MODELPOS=<x>,<y>[,<z>]            where the model (and the light's orbit) goes; no z: on the floor
//   K2VIEW_LIGHT=<r>,<g>,<b>,<brightness>,<orbit>,<height>   a fixed-colour light instead of the cycle
//   K2VIEW_STEALTH=1         start with the model in the stealth field
//   K2VIEW_ENVMAP=<cm_*>     the reflection map to start with, instead of the model's own
// The window is 1024x768: KOTOR 1's is K1_WINDOW (windowed unless K1_FULLSCREEN=1), KOTOR II's
// K2_WINDOW, set through the engine's SetResolution (one of its modes, or it picks the nearest).
#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl2.h"

#include "../../src/port/k2viewerapi.h"
#include "viewer_engine.h"

namespace {

#if KOTOR1
const char kGameName[] = "Knights of the Old Republic";
const char kDefaultModel[] = "lqa_dewback";   // BioWare's QA creature: skinned, bump-mapped, animated
const char kDefaultRoom[] = "mqatester";      // BioWare's QA room: lightmapped, bump-mapped water
#else
const char kGameName[] = "Knights of the Old Republic II";
const char kDefaultModel[] = "c_hutt";       // skinned and bump-mapped
const char kDefaultRoom[] = "000test";       // a BioWare test layout: one room, five lights
#endif

// The circling light: a text model with one dynamic light node (viewer_engine.cpp registers the
// commands a text model is made of).
const char kLightModel[] =
    "#MAXMODEL ASCII\n"   // the format test reads the first four bytes and does not give them back
    "newmodel k2vlight\n"
    "setsupermodel k2vlight NULL\n"
    "classification effect\n"
    "beginmodelgeom k2vlight\n"
    "node dummy k2vlight\n"
    "  parent NULL\n"
    "endnode\n"
    "node light k2vlight_l\n"
    "  parent k2vlight\n"
    "  position 0 0 0\n"
    "  orientation 0 0 0 0\n"
    "  color 1 1 1\n"
    "  radius 6\n"
    "  multiplier 1\n"
    "  isdynamic 1\n"
    "  affectdynamic 1\n"
    "  shadow 1\n"
    "  lightpriority 1\n"
    "  ambientonly 0\n"
    "endnode\n"
    // Where the light is: a small octahedron, lit by itself in the light's colour.
    "node trimesh k2vlight_m\n"
    "  parent k2vlight\n"
    "  position 0 0 0\n"
    "  orientation 0 0 0 0\n"
    "  ambient 0 0 0\n"
    "  diffuse 0 0 0\n"
    "  selfillumcolor 1 1 1\n"
    "  bitmap fx_white\n"
    "  render 1\n"
    "  shadow 0\n"
    "  verts 6\n"
    "    0.12 0 0\n"
    "    -0.12 0 0\n"
    "    0 0.12 0\n"
    "    0 -0.12 0\n"
    "    0 0 0.12\n"
    "    0 0 -0.12\n"
    "  tverts 1\n"
    "    0.5 0.5 0\n"
    "  faces 8\n"
    "    0 2 4 1 0 0 0 0\n"
    "    2 1 4 1 0 0 0 0\n"
    "    1 3 4 1 0 0 0 0\n"
    "    3 0 4 1 0 0 0 0\n"
    "    2 0 5 1 0 0 0 0\n"
    "    1 2 5 1 0 0 0 0\n"
    "    3 1 5 1 0 0 0 0\n"
    "    0 3 5 1 0 0 0 0\n"
    "endnode\n"
    "endmodelgeom k2vlight\n"
    "donemodel k2vlight\n";

// ---- what the game's files hold ----------------------------------------------------------------
// The names of one resource type in chitin.key, the texture packs and override/, read straight from
// the files (the working directory is the game's folder).
const uint16_t kResMdl = 2002;
const uint16_t kResLyt = 3000;

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

std::vector<unsigned char> ReadFile(const std::filesystem::path& path)
{
    std::vector<unsigned char> data;
    FILE* f = fopen(path.string().c_str(), "rb");
    if (!f) return data;
    fseek(f, 0, SEEK_END);
    long nSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (nSize > 0) {
        data.resize(static_cast<size_t>(nSize));
        if (fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
    }
    fclose(f);
    return data;
}

uint32_t U32(const std::vector<unsigned char>& d, size_t o)
{
    return o + 4 <= d.size() ? static_cast<uint32_t>(d[o] | d[o + 1] << 8 | d[o + 2] << 16 | static_cast<uint32_t>(d[o + 3]) << 24) : 0;
}

std::string ResName(const std::vector<unsigned char>& d, size_t o)
{
    std::string s;
    for (size_t i = 0; i < 16 && o + i < d.size() && d[o + i] != 0; i++) s += static_cast<char>(d[o + i]);
    return Lower(s);
}

// KEY V1: the key table is 22-byte entries (16-byte name, type, id) at the offset in the header.
void KeyNames(uint16_t nType, std::vector<std::string>& names)
{
    std::vector<unsigned char> d = ReadFile("chitin.key");
    if (d.size() < 24 || memcmp(d.data(), "KEY V1  ", 8) != 0) return;
    uint32_t nKeys = U32(d, 12), nOffset = U32(d, 20);
    for (uint32_t i = 0; i < nKeys; i++) {
        size_t o = nOffset + static_cast<size_t>(i) * 22;
        if (o + 22 > d.size()) break;
        if ((d[o + 16] | d[o + 17] << 8) == nType) names.push_back(ResName(d, o));
    }
}

// ERF V1.0: the key list is 24-byte entries (16-byte name, id, type, unused).
void ErfNames(const std::filesystem::path& path, const char* pszPrefix, std::vector<std::string>& names)
{
    std::vector<unsigned char> d = ReadFile(path);
    if (d.size() < 160 || memcmp(d.data(), "ERF V1.0", 8) != 0) return;
    uint32_t nEntries = U32(d, 16), nOffset = U32(d, 24);
    for (uint32_t i = 0; i < nEntries; i++) {
        std::string s = ResName(d, nOffset + static_cast<size_t>(i) * 24);
        if (s.compare(0, strlen(pszPrefix), pszPrefix) == 0) names.push_back(s);
    }
}

// A folder's files with the extension (".mdl"), case ignored, as resrefs.
void FolderNames(const char* pszFolder, const char* pszExt, const char* pszPrefix, std::vector<std::string>& names)
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(pszFolder, ec)) {
        std::string sName = Lower(entry.path().filename().string());
        std::string sExt = Lower(entry.path().extension().string());
        if (sExt == pszExt && sName.compare(0, strlen(pszPrefix), pszPrefix) == 0)
            names.push_back(sName.substr(0, sName.size() - sExt.size()));
    }
}

void SortUnique(std::vector<std::string>& names)
{
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
}

// The folder a name is in, whatever its case on disk (Linux).
std::string FindFolder(const char* pszName)
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(".", ec))
        if (entry.is_directory(ec) && Lower(entry.path().filename().string()) == pszName)
            return entry.path().string();
    return std::string();
}

// ---- an area's grass ----------------------------------------------------------------------------
// The modules' archives (RIM, ERF / MOD) hold the areas' .are files, named as their layouts are; the
// grass a backdrop gets is its area's (CSWCArea reads the same fields from the server's message).
const uint32_t kResAre = 2012;

std::vector<unsigned char> ReadRange(const std::filesystem::path& path, uint32_t nOffset, uint32_t nSize)
{
    std::vector<unsigned char> data(nSize);
    FILE* f = fopen(path.string().c_str(), "rb");
    if (!f) return std::vector<unsigned char>();
    if (fseek(f, static_cast<long>(nOffset), SEEK_SET) != 0 || fread(data.data(), 1, nSize, f) != nSize) data.clear();
    fclose(f);
    return data;
}

struct AreaFile {
    std::filesystem::path path;
    uint32_t nOffset, nSize;
};

// RIM V1.0: the entries are 32 bytes (16-byte name, type, id, offset, size); ERF / MOD V1.0: 24-byte
// keys (16-byte name, id, 16-bit type) and 8-byte resources (offset, size). The first archive to
// hold a name keeps it.
void IndexAreas(std::map<std::string, AreaFile>& areas)
{
    std::string sModules = FindFolder("modules");
    if (sModules.empty()) return;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(sModules, ec)) {
        std::string sExt = Lower(entry.path().extension().string());
        if (sExt != ".rim" && sExt != ".mod" && sExt != ".erf") continue;
        std::vector<unsigned char> h = ReadRange(entry.path(), 0, 32);
        if (h.size() < 32) continue;
        if (memcmp(h.data(), "RIM V1.0", 8) == 0) {
            uint32_t nEntries = U32(h, 12);
            std::vector<unsigned char> t = ReadRange(entry.path(), U32(h, 16), nEntries * 32);
            for (uint32_t i = 0; i < nEntries && (i + 1) * 32 <= t.size(); i++)
                if (U32(t, i * 32 + 16) == kResAre)
                    areas.emplace(ResName(t, i * 32), AreaFile{entry.path(), U32(t, i * 32 + 24), U32(t, i * 32 + 28)});
        } else if (memcmp(h.data(), "MOD V1.0", 8) == 0 || memcmp(h.data(), "ERF V1.0", 8) == 0) {
            uint32_t nEntries = U32(h, 16);
            std::vector<unsigned char> k = ReadRange(entry.path(), U32(h, 24), nEntries * 24);
            std::vector<unsigned char> r = ReadRange(entry.path(), U32(h, 28), nEntries * 8);
            for (uint32_t i = 0; i < nEntries && (i + 1) * 24 <= k.size() && (i + 1) * 8 <= r.size(); i++)
                if ((k[i * 24 + 20] | k[i * 24 + 21] << 8) == kResAre)
                    areas.emplace(ResName(k, i * 24), AreaFile{entry.path(), U32(r, i * 8), U32(r, i * 8 + 4)});
        }
    }
}

// What an area without a .are (BioWare's test rooms) gets: the usual blade layout, in the game's own
// grass texture.
K2ViewGrass DefaultGrass()
{
    K2ViewGrass g = {};
#if KOTOR1
    snprintf(g.szTexture, sizeof g.szTexture, "lda_grass2");   // Dantooine's; KOTOR 1 has no "grass"
#else
    snprintf(g.szTexture, sizeof g.szTexture, "grass");
    g.afEmissive[0] = 0x51 / 255.0f;
    g.afEmissive[1] = 0x82 / 255.0f;
    g.afEmissive[2] = 0x2d / 255.0f;
#endif
    g.fDensity = 4.0f;
    g.fQuadSize = 1.0f;
    const float afProbability[4] = {0.02f, 0.45f, 0.08f, 0.45f};
    memcpy(g.afProbability, afProbability, sizeof afProbability);
    return g;
}

// The light of an area without a .are (BioWare's test rooms), as nearly every area sets it: no sun (its
// light is in the lightmaps), the ambient for creatures and placeables an interior's (KOTOR 1: the Endar
// Spire's 0x60534a; KOTOR II: the commonest, 0x2d2b22), no fog.
K2ViewLighting DefaultLighting()
{
    K2ViewLighting l = {};
#if KOTOR1
    const float afAmbient[3] = {0x4a / 255.0f, 0x53 / 255.0f, 0x60 / 255.0f};
#else
    const float afAmbient[3] = {0x22 / 255.0f, 0x2b / 255.0f, 0x2d / 255.0f};
#endif
    memcpy(l.afAmbient, afAmbient, sizeof afAmbient);
    l.fFogNear = l.fFogFar = 10000.0f;
    l.fShadowOpacity = 0.5f;
    for (int i = 0; i < 3; i++) l.anDirtSize[i] = 16;
    return l;
}

// GFF V3.2: the top-level struct's fields: the Grass_* ones, and the sun, moon, ambient, fog and shadow
// ones CSWCArea lights the area with (BYTE fields are inline, DWORD colours are 0xBBGGRR).
bool ReadArea(const AreaFile& area, K2ViewGrass& g, K2ViewLighting& light)
{
    std::vector<unsigned char> d = ReadRange(area.path, area.nOffset, area.nSize);
    if (d.size() < 56 || memcmp(d.data(), "ARE V3.2", 8) != 0) return false;
    uint32_t nStructs = U32(d, 8), nFields = U32(d, 16), nLabels = U32(d, 24), nFieldData = U32(d, 32), nFieldIndices = U32(d, 40);
    uint32_t nTopData = U32(d, nStructs + 4), nTopCount = U32(d, nStructs + 8);
    g = K2ViewGrass();
    std::map<std::string, uint32_t> values;   // the lighting fields, raw
    for (uint32_t i = 0; i < nTopCount; i++) {
        uint32_t nField = nTopCount == 1 ? nTopData : U32(d, nFieldIndices + nTopData + i * 4);
        size_t o = nFields + static_cast<size_t>(nField) * 12;
        uint32_t nType = U32(d, o), nValue = U32(d, o + 8);
        std::string sLabel = ResName(d, nLabels + static_cast<size_t>(U32(d, o + 4)) * 16);
        if (nType == 0 || nType == 4 || nType == 5 || nType == 8) values[sLabel] = nValue;
        if (sLabel.compare(0, 6, "grass_") != 0) continue;
        sLabel = sLabel.substr(6);
        float fValue;
        memcpy(&fValue, &nValue, sizeof fValue);
        float* pColour = sLabel == "diffuse" ? g.afDiffuse : sLabel == "ambient" ? g.afAmbient : sLabel == "emissive" ? g.afEmissive : nullptr;
        if (nType == 11 && sLabel == "texname" && nFieldData + nValue < d.size()) {
            size_t nLen = std::min<size_t>(d[nFieldData + nValue], sizeof g.szTexture - 1);
            if (nFieldData + nValue + 1 + nLen <= d.size())
                memcpy(g.szTexture, &d[nFieldData + nValue + 1], nLen);
        } else if (nType == 8 && sLabel == "density") g.fDensity = fValue;
        else if (nType == 8 && sLabel == "quadsize") g.fQuadSize = fValue;
        else if (nType == 8 && sLabel == "prob_ll") g.afProbability[0] = fValue;
        else if (nType == 8 && sLabel == "prob_lr") g.afProbability[1] = fValue;
        else if (nType == 8 && sLabel == "prob_ul") g.afProbability[2] = fValue;
        else if (nType == 8 && sLabel == "prob_ur") g.afProbability[3] = fValue;
        else if (nType == 4 && pColour != nullptr)
            for (int c = 0; c < 3; c++) pColour[c] = static_cast<float>((nValue >> (c * 8)) & 0xff) / 255.0f;
    }
    if (g.szTexture[0] == 0) snprintf(g.szTexture, sizeof g.szTexture, "grass");   // the server's default

    auto Raw = [&](const char* pszName, uint32_t nDefault) {
        auto it = values.find(pszName);
        return it != values.end() ? it->second : nDefault;
    };
    auto Float = [&](const char* pszName, float fDefault) {
        uint32_t n;
        memcpy(&n, &fDefault, sizeof n);
        n = Raw(pszName, n);
        float f;
        memcpy(&f, &n, sizeof f);
        return f;
    };
    auto Colour = [&](const char* pszName, float* pColour) {
        uint32_t n = Raw(pszName, 0);
        for (int c = 0; c < 3; c++) pColour[c] = static_cast<float>((n >> (c * 8)) & 0xff) / 255.0f;
    };
    const bool bNight = (Raw("isnight", 0) & 0xff) != 0;
    const char* p = bNight ? "moon" : "sun";
    Colour((std::string(p) + "ambientcolor").c_str(), light.afSunAmbient);
    Colour((std::string(p) + "diffusecolor").c_str(), light.afSunDiffuse);
    Colour((std::string(p) + "fogcolor").c_str(), light.afFogColour);
    light.bFog = (Raw((std::string(p) + "fogon").c_str(), 0) & 0xff) != 0;
    light.fFogNear = Float((std::string(p) + "fognear").c_str(), 10000.0f);
    light.fFogFar = Float((std::string(p) + "fogfar").c_str(), 10000.0f);
    Colour("dynambientcolor", light.afAmbient);
    light.fShadowOpacity = static_cast<float>(Raw("shadowopacity", 50) & 0xff) / 100.0f;
    static const char* const apszDirt[3] = {"one", "two", "three"};
    for (int i = 0; i < 3; i++) {
        std::string sIndex = apszDirt[i];
        light.anDirtColour[i] = Raw(("dirtyargb" + sIndex).c_str(), 0);
        light.anDirtSize[i] = static_cast<int>(Raw(("dirtysize" + sIndex).c_str(), 16));
        int nFormula = static_cast<int>(Raw(("dirtyformula" + sIndex).c_str(), 1));   // the file's 0 and 1 swap
        light.anDirtFormula[i] = nFormula == 1 ? 0 : nFormula == 0 ? 1 : nFormula;
        light.anDirtFunction[i] = static_cast<int>(Raw(("dirtyfunc" + sIndex).c_str(), 0));
    }
    return true;
}

// ---- GL state around the panel -----------------------------------------------------------------
// imgui_impl_opengl2 draws with the fixed-function pipeline and restores what it changes, but leaves
// the GLSL program, the buffer bindings, the generic attribute arrays and the other texture units to
// its caller: the engine leaves all of them set, and the shims cache them, so they are put back
// exactly as they were.
typedef void (APIENTRY* PfnUseProgram)(GLuint);
typedef void (APIENTRY* PfnBindBuffer)(GLenum, GLuint);
typedef void (APIENTRY* PfnActiveTexture)(GLenum);
typedef void (APIENTRY* PfnClientActiveTexture)(GLenum);
typedef void (APIENTRY* PfnVertexAttribArray)(GLuint);
typedef void (APIENTRY* PfnGetVertexAttribiv)(GLuint, GLenum, GLint*);

struct GlEntry {
    PfnUseProgram useProgram;
    PfnBindBuffer bindBuffer;
    PfnActiveTexture activeTexture;
    PfnClientActiveTexture clientActiveTexture;
    PfnVertexAttribArray enableAttrib;
    PfnVertexAttribArray disableAttrib;
    PfnGetVertexAttribiv getAttribiv;
    bool bLoaded;
} s_gl;

template <typename T> void Load(T& fn, const char* pszName)
{
    fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(pszName));
}

void LoadGl()
{
    if (s_gl.bLoaded) return;
    Load(s_gl.useProgram, "glUseProgram");
    Load(s_gl.bindBuffer, "glBindBuffer");
    Load(s_gl.activeTexture, "glActiveTexture");
    Load(s_gl.clientActiveTexture, "glClientActiveTexture");
    Load(s_gl.enableAttrib, "glEnableVertexAttribArray");
    Load(s_gl.disableAttrib, "glDisableVertexAttribArray");
    Load(s_gl.getAttribiv, "glGetVertexAttribiv");
    s_gl.bLoaded = true;
}

const int kTextureUnits = 4;
const int kAttribs = 16;

void DrawPanel()
{
    LoadGl();
    GLint nProgram = 0, nArrayBuffer = 0, nElementBuffer = 0, nActive = 0, nClientActive = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &nProgram);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &nArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &nElementBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &nActive);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &nClientActive);
    GLboolean abEnabled[kTextureUnits][3];
    GLint anAttrib[kAttribs];
    for (int i = 0; i < kTextureUnits; i++) {
        s_gl.activeTexture(GL_TEXTURE0 + i);
        abEnabled[i][0] = glIsEnabled(GL_TEXTURE_2D);
        abEnabled[i][1] = glIsEnabled(GL_TEXTURE_CUBE_MAP);
        abEnabled[i][2] = glIsEnabled(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_TEXTURE_1D);
    }
    for (int i = 0; i < kAttribs; i++) {
        anAttrib[i] = 0;
        s_gl.getAttribiv(static_cast<GLuint>(i), GL_VERTEX_ATTRIB_ARRAY_ENABLED, &anAttrib[i]);
        if (anAttrib[i]) s_gl.disableAttrib(static_cast<GLuint>(i));
    }
    s_gl.useProgram(0);
    s_gl.bindBuffer(GL_ARRAY_BUFFER, 0);
    s_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    s_gl.activeTexture(GL_TEXTURE0);
    s_gl.clientActiveTexture(GL_TEXTURE0);

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    for (int i = 0; i < kAttribs; i++)
        if (anAttrib[i]) s_gl.enableAttrib(static_cast<GLuint>(i));
    for (int i = 0; i < kTextureUnits; i++) {
        s_gl.activeTexture(GL_TEXTURE0 + i);
        if (abEnabled[i][0]) glEnable(GL_TEXTURE_2D);
        if (abEnabled[i][1]) glEnable(GL_TEXTURE_CUBE_MAP);
        if (abEnabled[i][2]) glEnable(GL_TEXTURE_1D);
    }
    s_gl.activeTexture(static_cast<GLenum>(nActive));
    s_gl.clientActiveTexture(static_cast<GLenum>(nClientActive));
    s_gl.bindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(nArrayBuffer));
    s_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(nElementBuffer));
    s_gl.useProgram(static_cast<GLuint>(nProgram));
}

// ---- the viewer --------------------------------------------------------------------------------
const float kPi = 3.14159265f;

void HueToRgb(float fHue, float* pRgb)
{
    float h = fHue - std::floor(fHue);
    ImGui::ColorConvertHSVtoRGB(h, 1.0f, 1.0f, pRgb[0], pRgb[1], pRgb[2]);
}

// A searchable list of names; returns the clicked one.
const char* PickList(const char* pszId, ImGuiTextFilter& filter, const std::vector<std::string>& names,
                     const std::string& sCurrent, float fHeight)
{
    const char* pszPicked = nullptr;
    ImGui::PushID(pszId);
    filter.Draw("##filter", -1.0f);
    if (ImGui::BeginListBox("##list", ImVec2(-1.0f, fHeight))) {
        ImGuiListClipper clipper;
        std::vector<const std::string*> shown;
        for (const std::string& s : names)
            if (filter.PassFilter(s.c_str())) shown.push_back(&s);
        clipper.Begin(static_cast<int>(shown.size()));
        while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                if (ImGui::Selectable(shown[static_cast<size_t>(i)]->c_str(), *shown[static_cast<size_t>(i)] == sCurrent))
                    pszPicked = shown[static_cast<size_t>(i)]->c_str();
        ImGui::EndListBox();
    }
    ImGui::PopID();
    return pszPicked;
}

class Viewer : public K2HostViewer {
public:
    void Start() override;
    int  Frame() override;

private:
    static int EventWatch(void* pUser, SDL_Event* pEvent);
    void ReadLists();
    void Build();
    void SetRoom(const std::string& sRoom);
    void LoadArea();
    void Rebuild();
    void SetModel(const std::string& sModel);
    void FrameModel();
    void Panel();
    void MoveKeys(float fDelta);
    void ApplyWind() { k2view_set_wind(m_fWindStrength, m_fWindDirection, m_fWindVariationA, m_fWindVariationB); }

    SDL_Window* m_pWindow = nullptr;
    SDL_GLContext m_pContext = nullptr;
    Uint32 m_nWindowId = 0;
    uint64_t m_nLastCounter = 0;
    bool m_bBuilt = false;
    bool m_bQuit = false;
    bool m_bShowPanel = true;
    bool m_bDragOrbit = false;
    bool m_bDragPan = false;

    std::vector<std::string> m_models, m_rooms, m_cubeMaps;
    std::map<std::string, AreaFile> m_areas;
    ImGuiTextFilter m_modelFilter, m_roomFilter;
    std::string m_sRoom, m_sModel, m_sAnim, m_sEnvMap;
    char m_szModelEntry[32] = "";
    std::string m_sStatus;

    float m_afModelPos[3] = {0.0f, 0.0f, 0.0f};
    float m_fModelFacing = 0.0f;
    float m_afModelMin[3] = {0.0f, 0.0f, 0.0f};
    float m_afModelMax[3] = {0.0f, 0.0f, 0.0f};

    float m_afTarget[3] = {0.0f, 0.0f, 1.0f};
    float m_fYaw = 0.6f, m_fPitch = 1.2f, m_fDist = 4.0f, m_fFov = 55.0f;

    bool m_bLight = true, m_bLightCircle = true, m_bRainbow = true;
    float m_afLightColour[3] = {1.0f, 0.2f, 0.2f};
    float m_fLightMultiplier = 1.0f;
    float m_fLightRadius = 6.0f, m_fCircleRadius = 2.0f, m_fCircleHeight = 1.5f, m_fCircleSpeed = 0.8f;
    float m_fLightAngle = 0.0f, m_fHue = 0.0f, m_fHueSpeed = 0.15f;
    float m_afAmbient[3] = {0.0f, 0.0f, 0.0f};
    bool m_bShadow = true, m_bStealth = false;
    bool m_bGrass = true;
    K2ViewGrass m_grass = {};
    K2ViewLighting m_light = DefaultLighting();
    std::string m_sGrassSource;
    float m_fWindStrength = 1.0f, m_fWindDirection = 0.785398f, m_fWindVariationA = 100.0f, m_fWindVariationB = 3.0f;
};

Viewer s_viewer;

// Installed before main: the engine consults g_pK2HostViewer when it would start the intro.
struct Install {
    Install()
    {
        g_pK2HostViewer = &s_viewer;
#if KOTOR1
        setenv("K1_WINDOW", "1024x768", 0);
        setenv("K1_FULLSCREEN", "0", 0);
#endif
    }
} s_install;

int Viewer::EventWatch(void*, SDL_Event* pEvent)
{
    if (ImGui::GetCurrentContext() == nullptr) return 1;
    ImGui_ImplSDL2_ProcessEvent(pEvent);
    Viewer* self = &s_viewer;
    ImGuiIO& io = ImGui::GetIO();
    switch (pEvent->type) {
    case SDL_QUIT:
        self->m_bQuit = true;
        break;
    case SDL_WINDOWEVENT:
        if (pEvent->window.event == SDL_WINDOWEVENT_CLOSE) self->m_bQuit = true;
        break;
    case SDL_KEYDOWN:
        if (pEvent->key.keysym.sym == SDLK_TAB && !io.WantTextInput) self->m_bShowPanel = !self->m_bShowPanel;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (io.WantCaptureMouse) break;
        if (pEvent->button.button == SDL_BUTTON_LEFT && !(SDL_GetModState() & KMOD_SHIFT)) self->m_bDragOrbit = true;
        else self->m_bDragPan = true;
        break;
    case SDL_MOUSEBUTTONUP:
        self->m_bDragOrbit = self->m_bDragPan = false;
        break;
    case SDL_MOUSEMOTION:
        if (self->m_bDragOrbit) {
            self->m_fYaw -= pEvent->motion.xrel * 0.01f;
            self->m_fPitch = std::min(std::max(self->m_fPitch - pEvent->motion.yrel * 0.01f, 0.05f), kPi - 0.05f);
        } else if (self->m_bDragPan) {
            // Along the view's right and up, scaled to the distance: the scene follows the mouse.
            float afRight[3], afUp[3], afBack[3];
            k2view_camera_axes(afRight, afUp, afBack);
            float fScale = self->m_fDist * 0.0015f;
            for (int i = 0; i < 3; i++)
                self->m_afTarget[i] += (-afRight[i] * pEvent->motion.xrel + afUp[i] * pEvent->motion.yrel) * fScale;
        }
        break;
    case SDL_MOUSEWHEEL:
        if (!io.WantCaptureMouse)
            self->m_fDist = std::min(std::max(self->m_fDist * std::pow(0.9f, static_cast<float>(pEvent->wheel.y)), 0.3f), 200.0f);
        break;
    default:
        break;
    }
    return 1;
}

void Viewer::ReadLists()
{
    KeyNames(kResMdl, m_models);
    FolderNames(FindFolder("override").c_str(), ".mdl", "", m_models);
    SortUnique(m_models);
    KeyNames(kResLyt, m_rooms);
    FolderNames(FindFolder("override").c_str(), ".lyt", "", m_rooms);
    SortUnique(m_rooms);
    // The high-quality pack; the other two hold the same names at lower sizes.
    std::string sPacks = FindFolder("texturepacks");
    if (!sPacks.empty()) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(sPacks, ec))
            if (Lower(entry.path().filename().string()) == "swpc_tex_tpa.erf") ErfNames(entry.path(), "cm_", m_cubeMaps);
    }
    FolderNames(FindFolder("override").c_str(), ".tpc", "cm_", m_cubeMaps);
    FolderNames(FindFolder("override").c_str(), ".tga", "cm_", m_cubeMaps);
    SortUnique(m_cubeMaps);
    IndexAreas(m_areas);
}

void Viewer::Start()
{
    m_pWindow = SDL_GL_GetCurrentWindow();
    SDL_SetWindowTitle(m_pWindow, (std::string(kGameName) + " - model viewer").c_str());
    SDL_ShowCursor(SDL_ENABLE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    m_pContext = SDL_GL_GetCurrentContext();
    m_nWindowId = SDL_GetWindowID(m_pWindow);
    ImGui_ImplSDL2_InitForOpenGL(m_pWindow, m_pContext);
    ImGui_ImplOpenGL2_Init();
    SDL_AddEventWatch(EventWatch, nullptr);

#if !KOTOR1
    int nWidth = 1024, nHeight = 768;
    if (const char* pszWindow = getenv("K2_WINDOW")) sscanf(pszWindow, "%dx%d", &nWidth, &nHeight);
    k2view_set_resolution(nWidth, nHeight);
#endif
    ReadLists();
    m_nLastCounter = SDL_GetPerformanceCounter();
}

// The scene is built on the first frame: KOTOR II's resolution change re-initialises Aurora at the
// top of it, which drops the textures of a model loaded before.
void Viewer::Build()
{
    const char* pszRoom = getenv("K2VIEW_ROOM");
    const char* pszModel = getenv("K2VIEW_MODEL");
    const char* pszAnim = getenv("K2VIEW_ANIM");
    std::string sRoom = pszRoom ? (strcmp(pszRoom, "none") == 0 ? "" : pszRoom) : kDefaultRoom;
    m_sModel = pszModel ? (strcmp(pszModel, "none") == 0 ? "" : pszModel) : kDefaultModel;

    // K2VIEW_OFF=<name>,...: the renderer switches to start off (the panel's names, case and
    // anything after the first word ignored: "Shadows,Bump").
    if (const char* pszOff = getenv("K2VIEW_OFF")) {
        const char* pszName;
        for (int i = 0; int* pnValue = k2view_switch(i, &pszName); i++) {
            std::string sWord = Lower(std::string(pszName).substr(0, std::string(pszName).find(' ')));
            for (const char* p = pszOff; *p;) {
                const char* pEnd = strchr(p, ',');
                std::string sItem = Lower(std::string(p, pEnd ? pEnd - p : strlen(p)));
                if (!sItem.empty() && sWord.compare(0, sItem.size(), sItem) == 0) *pnValue = 0;
                p = pEnd ? pEnd + 1 : p + strlen(p);
            }
        }
    }
    if (const char* pszStealth = getenv("K2VIEW_STEALTH")) m_bStealth = atoi(pszStealth) != 0;
    if (const char* pszEnvMap = getenv("K2VIEW_ENVMAP")) m_sEnvMap = pszEnvMap;
    k2view_set_model_effects(m_bShadow, m_bStealth);
    SetRoom(sRoom);
#if KOTOR1
    SDL_RWops* pLight = SDL_RWFromConstMem(kLightModel, static_cast<int>(sizeof(kLightModel) - 1));
    k2view_create_light(pLight);
#else
    std::filesystem::path lightPath = std::filesystem::temp_directory_path() / "k2vlight.mdl";
    if (FILE* f = fopen(lightPath.string().c_str(), "w+b")) {
        fwrite(kLightModel, 1, sizeof(kLightModel) - 1, f);
        fseek(f, 0, SEEK_SET);
        k2view_create_light(f);
        fclose(f);
    }
#endif
    static const char* const s_apszLight[] = {"did not load", "has no light node", "loaded"};
    fprintf(stderr, "k2view: the light model %s\n", s_apszLight[k2view_light_state()]);
    if (const char* pszPos = getenv("K2VIEW_MODELPOS")) {
        // x,y alone stands the model on the floor there.
        if (sscanf(pszPos, "%f,%f,%f", &m_afModelPos[0], &m_afModelPos[1], &m_afModelPos[2]) == 2)
            k2view_floor_at(m_afModelPos[0], m_afModelPos[1], 100.0f, -100.0f, &m_afModelPos[2]);
        k2view_set_model_position(m_afModelPos, m_fModelFacing);
    }
    SetModel(m_sModel);
    if (pszAnim) {
        m_sAnim = pszAnim;
        k2view_play_animation(pszAnim);
    }
    m_bBuilt = true;
}

// Rebuilds the scene with the backdrop; the model goes in the middle of its largest room, on the floor.
void Viewer::SetRoom(const std::string& sRoom)
{
    m_sRoom = sRoom;
    LoadArea();
    k2view_set_grass(m_bGrass ? &m_grass : nullptr);
    k2view_set_lighting(&m_light);
    k2view_build_scene(sRoom.c_str());
    ApplyWind();
    float afMin[3], afMax[3];
    if (k2view_main_room_bounds(afMin, afMax) != 0) {
        float x = (afMin[0] + afMax[0]) * 0.5f, y = (afMin[1] + afMax[1]) * 0.5f, z = afMin[2];
        k2view_floor_at(x, y, afMax[2], afMin[2] - 1.0f, &z);
        m_afModelPos[0] = x;
        m_afModelPos[1] = y;
        m_afModelPos[2] = z;
    } else {
        m_afModelPos[0] = m_afModelPos[1] = m_afModelPos[2] = 0.0f;
    }
    k2view_set_model_position(m_afModelPos, m_fModelFacing);
    char szStatus[96];
    snprintf(szStatus, sizeof szStatus, "%s: %d rooms", sRoom.empty() ? "(none)" : sRoom.c_str(), k2view_room_count());
    m_sStatus = szStatus;
    fprintf(stderr, "k2view: backdrop %s, the model at %.2f %.2f %.2f\n", szStatus, m_afModelPos[0],
            m_afModelPos[1], m_afModelPos[2]);
    FrameModel();
}

// The backdrop's grass and light: its area's, else the defaults.
void Viewer::LoadArea()
{
    auto it = m_areas.find(Lower(m_sRoom));
    if (it != m_areas.end() && ReadArea(it->second, m_grass, m_light)) {
        m_sGrassSource = m_sRoom + ".are in " + it->second.path.filename().string();
        if (m_grass.fDensity <= 0.0f) m_sGrassSource += " (none)";
    } else {
        m_grass = DefaultGrass();
        m_light = DefaultLighting();
        m_sGrassSource = "no .are: the defaults";
    }
    memcpy(m_afAmbient, m_light.afAmbient, sizeof m_afAmbient);
}

// The same backdrop again, built with the panel's grass; the model, its animation and the camera stay.
void Viewer::Rebuild()
{
    k2view_set_grass(m_bGrass ? &m_grass : nullptr);
    k2view_build_scene(m_sRoom.c_str());
    ApplyWind();
    k2view_set_model_position(m_afModelPos, m_fModelFacing);
    if (!m_sAnim.empty()) k2view_play_animation(m_sAnim.c_str());
}

void Viewer::SetModel(const std::string& sModel)
{
    m_sModel = sModel;
    snprintf(m_szModelEntry, sizeof m_szModelEntry, "%s", sModel.c_str());
    m_sAnim.clear();
    if (sModel.empty()) {
        k2view_set_model("");
        m_sStatus = "(no model)";
        FrameModel();
        return;
    }
    if (!k2view_set_model(sModel.c_str())) {
        m_sStatus = "no model " + sModel;
        fprintf(stderr, "k2view: %s\n", m_sStatus.c_str());
        return;
    }
    fprintf(stderr, "k2view: model %s, %d animations\n", sModel.c_str(), k2view_animation_count());
    k2view_set_model_position(m_afModelPos, m_fModelFacing);
    m_sStatus = sModel;
    // An idle, else the first animation it has.
    for (const char* pszIdle : {"pause1", "cpause1", "animloop1"}) {
        for (int i = 0; i < k2view_animation_count(); i++) {
            if (strcmp(k2view_animation_name(i), pszIdle) == 0) {
                m_sAnim = pszIdle;
                break;
            }
        }
        if (!m_sAnim.empty()) break;
    }
    if (m_sAnim.empty() && k2view_animation_count() > 0) m_sAnim = k2view_animation_name(0);
    if (!m_sAnim.empty()) k2view_play_animation(m_sAnim.c_str());
    if (!m_sEnvMap.empty()) k2view_set_envmap(m_sEnvMap.c_str());
    FrameModel();
}

// Points the camera at the model's middle, from far enough to see it whole. The bounds are the
// model's own, about its origin.
void Viewer::FrameModel()
{
    k2view_model_bounds(m_afModelMin, m_afModelMax);
    float fSize = 0.0f;
    for (int i = 0; i < 3; i++) {
        m_afTarget[i] = m_afModelPos[i] + (m_afModelMin[i] + m_afModelMax[i]) * 0.5f;
        fSize = std::max(fSize, m_afModelMax[i] - m_afModelMin[i]);
    }
    if (fSize <= 0.0f) {
        for (int i = 0; i < 3; i++) m_afTarget[i] = m_afModelPos[i];
        m_afTarget[2] += 1.0f;
        fSize = 2.0f;
    }
    m_fDist = std::max(fSize * 1.6f, 1.0f);
    m_fCircleRadius = std::max(fSize * 0.6f, 1.0f);
    m_fCircleHeight = m_afModelMax[2] * 0.6f + 0.5f;
    if (const char* pszLight = getenv("K2VIEW_LIGHT")) {
        m_bRainbow = false;
        sscanf(pszLight, "%f,%f,%f,%f,%f,%f", &m_afLightColour[0], &m_afLightColour[1], &m_afLightColour[2],
               &m_fLightMultiplier, &m_fCircleRadius, &m_fCircleHeight);
    }
    if (const char* pszCamera = getenv("K2VIEW_CAMERA"))
        sscanf(pszCamera, "%f,%f,%f", &m_fYaw, &m_fPitch, &m_fDist);
    if (const char* pszPanel = getenv("K2VIEW_PANEL"))
        m_bShowPanel = atoi(pszPanel) != 0;
    if (const char* pszTarget = getenv("K2VIEW_TARGET"))
        sscanf(pszTarget, "%f,%f,%f", &m_afTarget[0], &m_afTarget[1], &m_afTarget[2]);
}

void Viewer::Panel()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Viewer", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("%s  %.0f fps", m_sStatus.c_str(), static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::TextDisabled("drag: orbit  right-drag: pan  wheel: zoom");
    ImGui::TextDisabled("WASD: move  Q/E: down/up  Shift: faster  Tab: panel");

    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (const char* pszPicked = PickList("models", m_modelFilter, m_models, m_sModel, 140.0f))
            SetModel(pszPicked);
        if (ImGui::InputText("ResRef", m_szModelEntry, sizeof m_szModelEntry, ImGuiInputTextFlags_EnterReturnsTrue))
            SetModel(m_szModelEntry);
        bool bMoved = ImGui::DragFloat3("Position", m_afModelPos, 0.05f);
        bMoved |= ImGui::SliderAngle("Facing", &m_fModelFacing, -180.0f, 180.0f);
        if (bMoved) k2view_set_model_position(m_afModelPos, m_fModelFacing);
        if (ImGui::Button("Frame model")) FrameModel();
        bool bEffects = ImGui::Checkbox("Cast shadow", &m_bShadow);
        ImGui::SameLine();
        bEffects |= ImGui::Checkbox("Stealth field", &m_bStealth);
        if (bEffects) k2view_set_model_effects(m_bShadow, m_bStealth);
        if (m_bShadow && !k2view_model_can_cast_shadow())
            ImGui::TextDisabled("only character and item models cast shadows");
    }

    if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginListBox("##anims", ImVec2(-1.0f, 110.0f))) {
            for (int i = 0; i < k2view_animation_count(); i++) {
                const char* pszName = k2view_animation_name(i);
                if (ImGui::Selectable(pszName, m_sAnim == pszName)) {
                    m_sAnim = pszName;
                    k2view_play_animation(pszName);
                }
            }
            ImGui::EndListBox();
        }
    }

    if (ImGui::CollapsingHeader("Reflection map")) {
        if (ImGui::BeginListBox("##cubes", ImVec2(-1.0f, 110.0f))) {
            if (ImGui::Selectable("(the model's own)", m_sEnvMap.empty())) {
                m_sEnvMap.clear();
                k2view_set_envmap("");
                k2view_set_model_position(m_afModelPos, m_fModelFacing);
                if (!m_sAnim.empty()) k2view_play_animation(m_sAnim.c_str());
            }
            for (const std::string& s : m_cubeMaps) {
                if (ImGui::Selectable(s.c_str(), m_sEnvMap == s)) {
                    m_sEnvMap = s;
                    k2view_set_envmap(s.c_str());
                }
            }
            ImGui::EndListBox();
        }
    }

    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("On", &m_bLight);
        ImGui::SameLine();
        ImGui::Checkbox("Circle", &m_bLightCircle);
        ImGui::SameLine();
        ImGui::Checkbox("Cycle colour", &m_bRainbow);
        if (!m_bRainbow) ImGui::ColorEdit3("Colour", m_afLightColour);
        else ImGui::SliderFloat("Cycle speed", &m_fHueSpeed, 0.0f, 1.0f);
        ImGui::SliderFloat("Radius", &m_fLightRadius, 0.5f, 30.0f);
        ImGui::SliderFloat("Brightness", &m_fLightMultiplier, 0.0f, 4.0f);
        ImGui::SliderFloat("Orbit", &m_fCircleRadius, 0.2f, 15.0f);
        ImGui::SliderFloat("Height", &m_fCircleHeight, -2.0f, 10.0f);
        ImGui::SliderFloat("Speed", &m_fCircleSpeed, -3.0f, 3.0f);
        if (ImGui::ColorEdit3("Ambient", m_afAmbient)) k2view_set_ambient(m_afAmbient);
        // The area's sun (moon at night) and fog, as its .are sets them (Grass shows where from).
        {
            bool bChanged = ImGui::ColorEdit3("Sun ambient", m_light.afSunAmbient);
            bChanged |= ImGui::ColorEdit3("Sun diffuse", m_light.afSunDiffuse);
            bool bFog = m_light.bFog != 0;
            if (ImGui::Checkbox("Fog", &bFog)) {
                m_light.bFog = bFog ? 1 : 0;
                bChanged = true;
            }
            ImGui::SameLine();
            bChanged |= ImGui::DragFloatRange2("##fog", &m_light.fFogNear, &m_light.fFogFar, 1.0f, 0.0f, 10000.0f, "%.0f", "%.0f");
            bChanged |= ImGui::ColorEdit3("Fog colour", m_light.afFogColour);
            bChanged |= ImGui::SliderFloat("Shadow opacity", &m_light.fShadowOpacity, 0.0f, 1.0f);
#if !KOTOR1
            // The rooms' three noise ("dirt") textures: each channel's strength (the noise's modulus),
            // the cube's side, how it roughens the texture, random or Perlin.
            static const char* const s_apszFormula[] = {"Subtract", "Add", "Multiply"};
            static const char* const s_apszFunction[] = {"Random", "Perlin"};
            for (int i = 0; i < 3; i++) {
                char szLabel[16];
                snprintf(szLabel, sizeof szLabel, "Dirt %d", i + 1);
                ImGui::PushID(i);
                if (ImGui::TreeNode(szLabel)) {
                    unsigned int& nColour = m_light.anDirtColour[i];
                    int anChannel[4] = {static_cast<int>((nColour >> 16) & 0xff), static_cast<int>((nColour >> 8) & 0xff),
                                        static_cast<int>(nColour & 0xff), static_cast<int>(nColour >> 24)};
                    if (ImGui::SliderInt4("Strength RGBA", anChannel, 0, 255)) {
                        nColour = static_cast<unsigned int>(anChannel[3]) << 24 | static_cast<unsigned int>(anChannel[0]) << 16 |
                                  static_cast<unsigned int>(anChannel[1]) << 8 | static_cast<unsigned int>(anChannel[2]);
                        bChanged = true;
                    }
                    bChanged |= ImGui::SliderInt("Size", &m_light.anDirtSize[i], 0, 64);
                    bChanged |= ImGui::Combo("Formula", &m_light.anDirtFormula[i], s_apszFormula, 3);
                    bChanged |= ImGui::Combo("Function", &m_light.anDirtFunction[i], s_apszFunction, 2);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
#endif
            if (bChanged) {
                memcpy(m_light.afAmbient, m_afAmbient, sizeof m_afAmbient);
                k2view_set_lighting(&m_light);
            }
        }
    }

    if (ImGui::CollapsingHeader("Wind")) {
        // An area's wind power, as the game sets it (CSWCArea::SetWind), or any wind.
        static const char* const s_apszPowers[] = {"None", "Light", "Heavy"};
        static const float s_afPower[3][3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 100.0f, 3.0f}, {2.0f, 150.0f, 5.0f}};
        bool bChanged = false;
        for (int i = 0; i < 3; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(s_apszPowers[i])) {
                m_fWindStrength = s_afPower[i][0];
                m_fWindVariationA = s_afPower[i][1];
                m_fWindVariationB = s_afPower[i][2];
                bChanged = true;
            }
        }
        bChanged |= ImGui::SliderFloat("Strength", &m_fWindStrength, 0.0f, 6.0f);
        bChanged |= ImGui::SliderAngle("Direction", &m_fWindDirection, -180.0f, 180.0f);
        bChanged |= ImGui::SliderFloat("Gusts", &m_fWindVariationA, 0.0f, 180.0f, "%.0f deg");
        bChanged |= ImGui::SliderFloat("Flutter", &m_fWindVariationB, 0.0f, 20.0f, "%.1f deg");
        if (bChanged) ApplyWind();
    }

    if (ImGui::CollapsingHeader("Grass")) {
        // The grass is planted as the rooms spawn: a change rebuilds the backdrop.
        ImGui::TextDisabled("%s", m_sGrassSource.c_str());
        bool bChanged = ImGui::Checkbox("On##grass", &m_bGrass);
        ImGui::InputText("Texture", m_grass.szTexture, sizeof m_grass.szTexture);
        ImGui::SliderFloat("Density", &m_grass.fDensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Blade size", &m_grass.fQuadSize, 0.1f, 3.0f);
        ImGui::ColorEdit3("Diffuse##grass", m_grass.afDiffuse);
        ImGui::ColorEdit3("Ambient##grass", m_grass.afAmbient);
#if !KOTOR1
        ImGui::ColorEdit3("Emissive##grass", m_grass.afEmissive);
#endif
        ImGui::SliderFloat4("Corners", m_grass.afProbability, 0.0f, 1.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("each blade's chance of each texture quarter: LL, LR, UL, UR");
        bChanged |= ImGui::Button("Apply");
        ImGui::SameLine();
        if (ImGui::Button("The area's own")) {
            LoadArea();
            bChanged = true;
        }
        if (bChanged) Rebuild();
    }

    if (ImGui::CollapsingHeader("Backdrop")) {
        if (ImGui::Selectable("(none)", m_sRoom.empty())) SetRoom("");
        if (const char* pszPicked = PickList("rooms", m_roomFilter, m_rooms, m_sRoom, 120.0f)) SetRoom(pszPicked);
    }

    if (ImGui::CollapsingHeader("Renderer")) {
        const char* pszName;
        for (int i = 0; int* pnValue = k2view_switch(i, &pszName); i++) {
            bool bOn = *pnValue != 0;
            if (ImGui::Checkbox(pszName, &bOn)) *pnValue = bOn ? 1 : 0;
        }
        ImGui::SliderFloat("Field of view", &m_fFov, 20.0f, 100.0f);
    }
    ImGui::End();
}

// WASD moves the camera's target along the view and across it, level with the ground; Q / E down
// and up; Shift is faster. Only while the panel does not want the keyboard.
void Viewer::MoveKeys(float fDelta)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard) return;
    const Uint8* pKeys = SDL_GetKeyboardState(nullptr);
    float fForward = static_cast<float>(pKeys[SDL_SCANCODE_W] - pKeys[SDL_SCANCODE_S]);
    float fSide = static_cast<float>(pKeys[SDL_SCANCODE_D] - pKeys[SDL_SCANCODE_A]);
    float fRise = static_cast<float>(pKeys[SDL_SCANCODE_E] - pKeys[SDL_SCANCODE_Q]);
    if (fForward == 0.0f && fSide == 0.0f && fRise == 0.0f) return;
    float afRight[3], afUp[3], afBack[3];
    k2view_camera_axes(afRight, afUp, afBack);
    float afForward[2] = {-afBack[0], -afBack[1]};
    float fLen = std::sqrt(afForward[0] * afForward[0] + afForward[1] * afForward[1]);
    if (fLen < 1e-4f) {   // looking straight down: the view's up is forward
        afForward[0] = afUp[0];
        afForward[1] = afUp[1];
        fLen = std::sqrt(afForward[0] * afForward[0] + afForward[1] * afForward[1]);
    }
    float fRightLen = std::sqrt(afRight[0] * afRight[0] + afRight[1] * afRight[1]);
    float fSpeed = std::max(m_fDist, 2.0f) * 0.8f * fDelta * ((SDL_GetModState() & KMOD_SHIFT) ? 3.0f : 1.0f);
    for (int i = 0; i < 2; i++) {
        m_afTarget[i] += afForward[i] / std::max(fLen, 1e-4f) * fForward * fSpeed;
        m_afTarget[i] += afRight[i] / std::max(fRightLen, 1e-4f) * fSide * fSpeed;
    }
    m_afTarget[2] += fRise * fSpeed;
}

int Viewer::Frame()
{
    // KOTOR II's display-mode change remakes the SDL window (and can remake the GL context) after
    // Start: the backends follow it, or Dear ImGui drops every event as another window's. The GL
    // context can stay current on the old window's handle, so the window is the one with the focus.
    SDL_Window* pWindow = SDL_GetMouseFocus();
    if (pWindow == nullptr) pWindow = SDL_GetKeyboardFocus();
    SDL_GLContext pContext = SDL_GL_GetCurrentContext();
    if (pWindow == nullptr && pContext != m_pContext) pWindow = m_pWindow;
    // By id: the new window can take the old one's address.
    if (pWindow != nullptr && (SDL_GetWindowID(pWindow) != m_nWindowId || pContext != m_pContext)) {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        m_pWindow = pWindow;
        m_nWindowId = SDL_GetWindowID(pWindow);
        m_pContext = pContext;
        SDL_SetWindowTitle(m_pWindow, (std::string(kGameName) + " - model viewer").c_str());
        ImGui_ImplSDL2_InitForOpenGL(m_pWindow, m_pContext);
        ImGui_ImplOpenGL2_Init();
    }
    if (!m_bBuilt) Build();
    uint64_t nNow = SDL_GetPerformanceCounter();
    float fDelta = static_cast<float>(nNow - m_nLastCounter) / static_cast<float>(SDL_GetPerformanceFrequency());
    m_nLastCounter = nNow;
    fDelta = std::min(fDelta, 0.1f);

    // No panel while the window has no drawable (minimised, or KOTOR II's display-mode change in
    // flight): Dear ImGui needs a size to lay out in.
    int nW, nH, nWinW, nWinH;
    SDL_GL_GetDrawableSize(m_pWindow, &nW, &nH);
    SDL_GetWindowSize(m_pWindow, &nWinW, &nWinH);
    bool bPanel = m_bShowPanel && nW > 0 && nH > 0 && nWinW > 0 && nWinH > 0 &&
                  (SDL_GetWindowFlags(m_pWindow) & SDL_WINDOW_MINIMIZED) == 0;
    if (bPanel) {
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        Panel();
        ImGui::Render();
    }
    // SDL sends typed characters only while text input is on. The games turn it off (it raises the
    // IME bubble on macOS) and the backend turns it on only on Android, so it follows the panel:
    // on while a text field has the focus.
    bool bWantText = bPanel && ImGui::GetIO().WantTextInput;
    if (bWantText != (SDL_IsTextInputActive() == SDL_TRUE)) {
        if (bWantText) SDL_StartTextInput();
        else SDL_StopTextInput();
    }

    MoveKeys(fDelta);
    if (m_bLightCircle) m_fLightAngle += m_fCircleSpeed * fDelta;
    if (m_bRainbow) {
        m_fHue += m_fHueSpeed * fDelta;
        HueToRgb(m_fHue, m_afLightColour);
    }
    float afLight[3] = {
        m_afModelPos[0] + std::cos(m_fLightAngle) * m_fCircleRadius,
        m_afModelPos[1] + std::sin(m_fLightAngle) * m_fCircleRadius,
        m_afModelPos[2] + m_fCircleHeight,
    };
    k2view_set_light(m_bLight, afLight, m_afLightColour, m_fLightRadius, m_fLightMultiplier);
    k2view_set_camera(m_afTarget, m_fYaw, m_fPitch, m_fDist, m_fFov);

    if (nW > 0 && nH > 0) k2view_render(fDelta, nW, nH);
    if (bPanel) DrawPanel();
    if (m_bQuit) k2view_request_exit();
    return m_bQuit ? 1 : 0;
}

} // namespace
