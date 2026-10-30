#ifndef K2VIEWER_ENGINE_H
#define K2VIEWER_ENGINE_H

// The model viewer's engine half (viewer_engine.cpp). It is compiled with the engine's flags so it can
// include src/'s headers, and so may include no system header; viewer_host.cpp (SDL, Dear ImGui, GL)
// drives it through these calls, all on the game thread.

// KOTOR II: the engine's window and render size, as the console's SetResolution sets them (the frame
// loop re-initialises Aurora at the start of the next frame). KOTOR 1 makes its window at K1_WINDOW.
void k2view_set_resolution(int nWidth, int nHeight);

// The scene: the backdrop's rooms (an area layout, "" for none), the sun, the model, the light and the
// camera. Changing the backdrop rebuilds all of it, the model and the light included.
void k2view_build_scene(const char* pszLayout);
int  k2view_room_count();
int  k2view_main_room_bounds(float* pMin, float* pMax);   // the largest room's box; 0: no rooms
// The first surface a ray straight down from fTop to fBottom meets at (fX, fY): 0 for none.
int  k2view_floor_at(float fX, float fY, float fTop, float fBottom, float* pZ);

// An area's grass, as its .are gives it (CSWCArea's load: every surface material surfacemat.2da marks
// as grass gets it). Read when the backdrop is built: set it before k2view_build_scene. A null or a
// zero density: no grass.
struct K2ViewGrass {
    char  szTexture[32];
    float fDensity, fQuadSize;
    float afDiffuse[3], afAmbient[3], afEmissive[3];   // emissive: KOTOR II only
    float afProbability[4];   // the four blade corners' chances: lower left, lower right, upper left, upper right
};
void k2view_set_grass(const K2ViewGrass* pGrass);

// pLightModel: the light's model as text, an SDL_RWops* (KOTOR 1) or a FILE* (KOTOR II), consumed.
void k2view_create_light(void* pLightModel);
int  k2view_light_state();   // 0 no light model, 1 no light node in it, 2 a light

int  k2view_set_model(const char* pszResRef);   // 0: no such model (the old one is gone either way)
const char* k2view_model_name();
void k2view_model_bounds(float* pMin, float* pMax);
void k2view_set_model_position(const float* pPos, float fFacing);
int  k2view_animation_count();
const char* k2view_animation_name(int i);
void k2view_play_animation(const char* pszName);
int  k2view_set_envmap(const char* pszTexture);   // "" restores the model's own
// The model's shadow (the engine casts one for character and item models only, from the light the
// scene's focus -- the model -- picks) and the stealth field's distortion (the frame-buffer pass).
void k2view_set_model_effects(int bShadow, int bStealth);
int  k2view_model_can_cast_shadow();

// An area's light, as its .are gives it and CSWCArea sets it (SetAsDay / SetAsNight): the sun's two
// lights recoloured (the moon's colours at night), the scene's ambient for dynamic objects, the fog
// and the shadows' opacity. Applied at once and on every rebuild; a null keeps the sun model's own
// colours, no fog. Colours 0..1.
struct K2ViewLighting {
    float afSunAmbient[3], afSunDiffuse[3], afAmbient[3];
    int   bFog;
    float fFogNear, fFogFar;
    float afFogColour[3];
    float fShadowOpacity;   // 0..1
    // KOTOR II: the three noise ("dirt") textures some rooms' meshes are roughened with, as the .are's
    // Dirty*One / Two / Three set them (CSWSArea): the colour 0xAARRGGBB (each channel the noise's
    // modulus, 0 for none), the cube's side, the formula (0 subtract, 1 add, 2 multiply) and the
    // function (0 random, 1 Perlin).
    unsigned int anDirtColour[3];
    int anDirtSize[3], anDirtFormula[3], anDirtFunction[3];
};
void k2view_set_lighting(const K2ViewLighting* pLighting);

void k2view_set_light(int bOn, const float* pPos, const float* pColour, float fRadius, float fMultiplier);
void k2view_set_ambient(const float* pColour);
// The scene's wind (dangly meshes, emitters, grass): strength, direction in radians, and the two
// deviations in degrees, as an area's wind power sets them (none 0/0/0, light 1/100/3, heavy 2/150/5).
void k2view_set_wind(float fStrength, float fDirection, float fVariationA, float fVariationB);
void k2view_set_camera(const float* pTarget, float fYaw, float fPitch, float fDist, float fFov);
void k2view_render(float fDelta, int nWidth, int nHeight);
// The camera's axes in the world: its right, its up, and back (it looks down its -Z).
void k2view_camera_axes(float* pRight, float* pUp, float* pBack);

// Ends the game's frame loop after this frame: KOTOR II's ends when Frame returns non-zero, KOTOR 1's
// on g_done (MacMain.cpp). Both games answer the window's close with their quit dialog, which the
// viewer never shows.
void k2view_request_exit();

// The renderer's switches: engine globals, by index until null.
int* k2view_switch(int i, const char** ppszName);

#endif // K2VIEWER_ENGINE_H
