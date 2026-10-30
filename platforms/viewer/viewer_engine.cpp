// viewer_engine.cpp - the model viewer's engine half: the scene the viewer renders, built from the
// engine's own pieces the way the game builds an area (nwcarea.cpp) and the GUI its 3D widgets
// (CSWGuiScene, swguidrawable.cpp). Compiled with the engine's flags; see viewer_engine.h.
#include "k2stdint.h"
#include "Vector.h"
#include "Quaternion.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "aurscene.h"
#include "aurwindmanager.h"
#include "aurconsole.h"
#include "exostring.h"
#include "exoresman.h"
#include "lyt.h"
#include "twodimarray.h"
#include "aurgrass.h"
#include "aurtexman.h"
#include "aurtexcontrol.h"
#include "port/aslglapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"
#include "viewer_engine.h"

struct SDL_RWops;

CAurScene*  NewCAurScene(char* pszName);
CAurCamera* NewCAurCamera(char* pszName);
#if KOTOR1
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif
Quaternion YawPitchRoll(float yaw, float pitch, float roll);
void AurMakeCurrent();   // aurora.cpp: the per-frame texture bookkeeping (texcurrentframe)
extern float DeltaT;      // the last frame's length, which the texture controllers step by
void AurGrassClearInfo();
int  AurControlPart(char* pszArgs);   // aurmodel.cpp: the console's "<object> <part> <property> <values>"
void SetClearColor(char* pszColour);  // main.cpp: the frame's clear colour, "r g b"
extern int shadowalpha;               // aurscene.cpp: a float in an int's clothing
void AurGrassAddInfo(int nMaterial, const CAurGrassInfo& info);
#if !KOTOR1
char* SetResolution(char* pTag);   // main.cpp: the console's SetResolution, re-initialised next frame
int  AurTextureGetTID(CAurTexture* pTexture);
#endif

extern int enableshadows;
extern int enablelightmap;
extern int enablebumpmap;
extern int enablegrass;
extern int enableemitters;
extern int enablebumpyshiny;
#if KOTOR1
extern volatile bool g_done;     // MacMain.cpp: the frame loop's exit
extern int preferPerfToVisual;   // K1's perf mode: no frame-buffer passes, no shadows
#endif

// ---- the text model format ----------------------------------------------------------------------
// The engine still parses BioWare's text .mdl (IODispatcher::ReadSync runs a text file through the
// console interpreter line by line), but the shipped games register none of the commands a model file
// is made of, so the viewer does: its light is a text model, and so can be anything a modder drops in
// override/.
void newmodel(char* pszName);
void beginmodelgeom(char* pszName);
void donemodel(char* pszName);
void classification(char* pszName);
void newanim(char* pszName);
char* setsupermodel(char* pszArgs);
char* setanimationscale(char* pszArgs);

namespace {
void IgnoreLine(char*) {}

ConsoleFunc s_conNewModel(const_cast<char*>("newmodel"), newmodel);
ConsoleFunc s_conBeginModelGeom(const_cast<char*>("beginmodelgeom"), beginmodelgeom);
ConsoleFunc s_conDoneModel(const_cast<char*>("donemodel"), donemodel);
ConsoleFunc s_conClassification(const_cast<char*>("classification"), classification);
ConsoleFunc s_conNewAnim(const_cast<char*>("newanim"), newanim);
ConsoleFunc s_conSetSuperModel(const_cast<char*>("setsupermodel"), setsupermodel);
ConsoleFunc s_conSetAnimationScale(const_cast<char*>("setanimationscale"), setanimationscale);
ConsoleFunc s_conFileDependancy(const_cast<char*>("filedependancy"), IgnoreLine);

const char kLightModel[] = "k2vlight";
const char kLightPart[] = "k2vlight_l";
const char kMarkerPart[] = "k2vlight_m";

CAurScene*  s_pScene;
CAurCamera* s_pCamera;
CAurObject* s_pSun;
CAurObject* s_pModel;
CAurObject* s_pLight;
char s_szModel[32];
char s_szEnvMap[32];
float s_afModelPos[3];
float s_fModelFacing;
Vector s_vAmbient(0.35f, 0.35f, 0.35f);
bool s_bModelShadow = true;
bool s_bModelStealth;
bool s_bGrass;
K2ViewGrass s_grass;
bool s_bLighting;
K2ViewLighting s_lighting;
#if !KOTOR1
int s_anDirtTID[3];   // each dirt texture's TID when it was last made from s_lighting
#endif

// The animations a model can play: its own, then each supermodel's, a name once.
List<char*> s_animations;

void DeleteObject(CAurObject*& pObject)
{
    if (pObject != 0) {
        delete pObject;
        pObject = 0;
    }
}

void CollectAnimations()
{
    s_animations.m_count = 0;
    if (s_pModel == 0)
        return;
    for (Model* pModel = static_cast<Gob*>(s_pModel)->m_pModel; pModel != 0; pModel = pModel->m_pSuperModel) {
        for (int i = 0; i < pModel->m_animations.m_count; i++) {
            char* pszName = pModel->m_animations.m_pData[i]->m_szName;
            bool bSeen = false;
            for (int j = 0; j < s_animations.m_count && !bSeen; j++)
                bSeen = _stricmp(s_animations.m_pData[j], pszName) == 0;
            if (!bSeen)
                s_animations.Add(pszName);
        }
    }
}

void PlaceModel()
{
    if (s_pModel == 0)
        return;
    s_pModel->SetPosition(Vector(s_afModelPos[0], s_afModelPos[1], s_afModelPos[2]));
    s_pModel->SetOrientation(Quaternion(Vector(0.0f, 0.0f, 1.0f), s_fModelFacing));
}

// The rooms of an area's layout, spawned as the client's area spawns them (CSWCArea's load: the
// layout through the resource manager -- Scene::SpawnRooms reads it through Aurora's own resource
// hook, which serves no .lyt -- and each room's looping animations started).
void SpawnLayout(const char* pszLayout)
{
    CLYT layout;
    if (layout.LoadLayout(CExoString(pszLayout)) == 0)
        return;
    for (int i = 0; i < layout.GetRoomCount(); i++) {
        CExoString sRoom(layout.GetRoomName(i).GetResRefStr());
        Gob* pRoom = s_pScene->SpawnRoom(static_cast<char*>(sRoom.CStr()), layout.GetRoomPosition(i), Quaternion());
        if (pRoom == 0)
            continue;
        pRoom->PlayAnimation(const_cast<char*>("animloop1"), 1.0f, 8, 0.0f);
        pRoom->PlayAnimation(const_cast<char*>("animloop2"), 1.0f, 8, 0.0f);
        pRoom->PlayAnimation(const_cast<char*>("animloop3"), 1.0f, 8, 0.0f);
    }
    layout.UnloadLayout();
}

void ApplyModelEffects()
{
    if (s_pModel == 0)
        return;
    if (s_bModelShadow)
        s_pModel->TurnOnShadows();
    else
        s_pModel->TurnOffShadows();
    if (s_bModelStealth)
        s_pModel->EnableDistortion();
    else
        s_pModel->DisableDistortion();
}

void AttachModel()
{
    if (s_pModel == 0 || s_pScene == 0)
        return;
    PlaceModel();
    s_pModel->AttachToScene(s_pScene);
    ApplyModelEffects();
    if (s_szEnvMap[0] != 0)
        s_pModel->SetEnvironmentMap(s_szEnvMap);
}

// CSWCArea's light: the sun's parts recoloured through the console, as SetGlobalLightAmbient /
// SetGlobalLightDiffuse do, the fog as SetFog / SetFogColor, the shadows' opacity as SetAsDay.
void ApplyLighting()
{
    if (s_pScene == 0)
        return;
    if (!s_bLighting) {
        s_pScene->SetFog(0);
        return;
    }
    const K2ViewLighting& l = s_lighting;
    char szCommand[128];
    __builtin_snprintf(szCommand, sizeof(szCommand), "sun gidy_sun_amb color %f %f %f", static_cast<double>(l.afSunAmbient[0]),
                       static_cast<double>(l.afSunAmbient[1]), static_cast<double>(l.afSunAmbient[2]));
    AurControlPart(szCommand);
    __builtin_snprintf(szCommand, sizeof(szCommand), "sun gidy_sun_diff color %f %f %f", static_cast<double>(l.afSunDiffuse[0]),
                       static_cast<double>(l.afSunDiffuse[1]), static_cast<double>(l.afSunDiffuse[2]));
    AurControlPart(szCommand);
    s_vAmbient = Vector(l.afAmbient[0], l.afAmbient[1], l.afAmbient[2]);
    s_pScene->SetAmbient(s_vAmbient);
    s_pScene->SetFog(l.bFog);
    s_pScene->SetFogRange(l.fFogNear, l.fFogFar);
    Vector vFog(l.afFogColour[0], l.afFogColour[1], l.afFogColour[2]);
    s_pScene->SetFogColor(vFog);
    __builtin_snprintf(szCommand, sizeof(szCommand), "%f %f %f", static_cast<double>(vFog.x), static_cast<double>(vFog.y),
                       static_cast<double>(vFog.z));
    SetClearColor(szCommand);
    float fShadow = l.fShadowOpacity;
    __builtin_memcpy(&shadowalpha, &fShadow, sizeof(fShadow));
#if !KOTOR1
    for (int i = 0; i < 3; i++)
        s_anDirtTID[i] = -1;
#endif
}

#if !KOTOR1
// The dirt textures are made when they are first built (CAurTextureBasic::BuildAndStoreTexture, the
// frame after a noise mesh first binds one), from the server's area, which the viewer has none of
// (DirtyTextureController::Init: none); each is made again from the .are's once it has been built.
void UpdateDirt()
{
    Scene* pScene = static_cast<Scene*>(s_pScene);
    CAurTexture* apDirt[3] = { pScene->m_pDirtyTexture, pScene->m_pDirtyTexture2, pScene->m_pDirtyTexture3 };
    for (int i = 0; i < 3; i++) {
        int nTID = apDirt[i] != 0 ? AurTextureGetTID(apDirt[i]) : 0;
        TextureController* pController = apDirt[i] != 0 ? apDirt[i]->GetReferencedTexture()->GetController() : 0;
        if (nTID == 0 || pController == 0 || nTID == s_anDirtTID[i])
            continue;
        s_anDirtTID[i] = nTID;
        const K2ViewLighting& l = s_lighting;
        DirtyTextureController* pDirt = static_cast<DirtyTextureController*>(pController);
        delete[] pDirt->m_pImage;
        pDirt->m_pImage = 0;
        ASLgl::glBindTexture(GL_TEXTURE_3D, static_cast<unsigned int>(nTID));
        if (s_bLighting && l.anDirtSize[i] != 0)
            pDirt->Init(static_cast<int>(l.anDirtColour[i]), l.anDirtSize[i], l.anDirtFormula[i], l.anDirtFunction[i]);
        else
            pDirt->Init(0, 16, 0, 0);
    }
}
#endif

// CSWCArea's grass setup (its area message handler): one record, filed under every surface material
// surfacemat.2da's Grass column marks. The rooms' grass bins are made as they spawn, from the
// materials in use then.
enum { kSurfaceMaterialRows = 32 };
void SetUpGrass()
{
    AurGrassClearInfo();
    if (!s_bGrass || s_grass.fDensity <= 0.0f)
        return;
    CAurGrassInfo info;
    __builtin_strncpy(info.m_sName, s_grass.szTexture, sizeof(info.m_sName) - 1);
    info.m_sName[sizeof(info.m_sName) - 1] = 0;
    info.m_fDensity = s_grass.fDensity;
    info.m_fQuadSize = s_grass.fQuadSize;
    info.m_vDiffuse = Vector(s_grass.afDiffuse[0], s_grass.afDiffuse[1], s_grass.afDiffuse[2]);
    info.m_vAmbient = Vector(s_grass.afAmbient[0], s_grass.afAmbient[1], s_grass.afAmbient[2]);
#if !KOTOR1
    info.m_vEmissive = Vector(s_grass.afEmissive[0], s_grass.afEmissive[1], s_grass.afEmissive[2]);
#endif
    info.m_fProbLL = s_grass.afProbability[0];
    info.m_fProbLR = s_grass.afProbability[1];
    info.m_fProbUL = s_grass.afProbability[2];
    info.m_fProbUR = s_grass.afProbability[3];
    C2DA surfaceMaterials(CResRef("surfacemat"), 0);
    if (!surfaceMaterials.Load2DArray())
        return;
    for (int nRow = 0; nRow < kSurfaceMaterialRows; nRow++) {
        int nGrass = 0;
        if (surfaceMaterials.GetINTEntry(nRow, CExoString("Grass"), &nGrass) && nGrass != 0)
            AurGrassAddInfo(nRow, info);
    }
}
} // namespace

void k2view_build_scene(const char* pszLayout)
{
    // The objects go before the scene they are in.
    char szModel[32];
    __builtin_strncpy(szModel, s_szModel, sizeof(szModel));
    DeleteObject(s_pModel);
    if (s_pLight != 0)
        s_pLight->AttachToScene(0);
    DeleteObject(s_pSun);
    if (s_pCamera != 0) {
        delete s_pCamera;
        s_pCamera = 0;
    }
    if (s_pScene != 0) {
        delete s_pScene;
        s_pScene = 0;
    }

    s_pScene = NewCAurScene(const_cast<char*>("viewerscene"));
    s_pCamera = NewCAurCamera(const_cast<char*>("camera"));
    // The grid an area of up to 64 x 64 ten-unit cells needs; the viewer moves nothing between them.
    s_pScene->InitBSP(64, 64, 5, true, 10.0f);
    SetUpGrass();
    if (pszLayout != 0 && pszLayout[0] != 0)
        SpawnLayout(pszLayout);
    // Every room draws (the camera goes anywhere); the current room, which turns the frame-buffer
    // passes and the shadows on (Scene::Render), follows the camera's target (k2view_set_camera).
    s_pScene->DisableVisibilityGraph();
    s_pScene->SetCurrentRoom(0);
    s_pScene->SetFog(0);

    s_pSun = NewCAurObject(const_cast<char*>("gidy_sun"), const_cast<char*>("sun"), 0, 0);
    if (s_pSun != 0) {
        s_pSun->SetPosition(Vector(4000.0f, 4500.0f, 7000.0f));
        s_pSun->AttachToScene(s_pScene);
    }

    s_pCamera->SetClipDist(0.1f, 10000.0f);

    if (szModel[0] != 0)
        k2view_set_model(szModel);
    if (s_pLight != 0)
        s_pLight->AttachToScene(s_pScene);

    s_pScene->SetAmbient(s_vAmbient);
    ApplyLighting();
    s_pScene->FinishSceneGeometry();
    s_pScene->ComputeStaticLighting();
    s_pCamera->AttachToScene(s_pScene);
}

void k2view_set_resolution(int nWidth, int nHeight)
{
#if KOTOR1
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);   // K1's window is made at its size (K1_WINDOW)
#else
    char szArgs[32];
    __builtin_snprintf(szArgs, sizeof(szArgs), "%d %d", nWidth, nHeight);
    SetResolution(szArgs);
#endif
}

int k2view_room_count()
{
    return s_pScene != 0 ? s_pScene->GetNumRooms() : 0;
}

int k2view_main_room_bounds(float* pMin, float* pMax)
{
    CAurRoom* pBest = 0;
    float fBestArea = 0.0f;
    for (int i = 0; i < k2view_room_count(); i++) {
        CAurRoom* pRoom = s_pScene->GetRoom(i);
        if (pRoom->m_bboxMax.x < pRoom->m_bboxMin.x || pRoom->m_bboxMax.y < pRoom->m_bboxMin.y)
            continue;   // a room with no geometry keeps the empty box
        float fArea = (pRoom->m_bboxMax.x - pRoom->m_bboxMin.x) * (pRoom->m_bboxMax.y - pRoom->m_bboxMin.y);
        if (pBest == 0 || fArea > fBestArea) {
            pBest = pRoom;
            fBestArea = fArea;
        }
    }
    if (pBest == 0)
        return 0;
    pMin[0] = pBest->m_bboxMin.x; pMin[1] = pBest->m_bboxMin.y; pMin[2] = pBest->m_bboxMin.z;
    pMax[0] = pBest->m_bboxMax.x; pMax[1] = pBest->m_bboxMax.y; pMax[2] = pBest->m_bboxMax.z;
    return 1;
}

int k2view_floor_at(float fX, float fY, float fTop, float fBottom, float* pZ)
{
    if (s_pScene == 0)
        return 0;
    Vector vHit;
    if (s_pScene->HitCheck(Vector(fX, fY, fTop), Vector(fX, fY, fBottom), &vHit, 0) == 0)
        return 0;
    *pZ = vHit.z;
    return 1;
}

void k2view_create_light(void* pLightModel)
{
#if KOTOR1
    s_pLight = NewCAurObject(const_cast<char*>(kLightModel), const_cast<char*>(kLightModel),
                             static_cast<SDL_RWops*>(pLightModel), 0);
#else
    s_pLight = NewCAurObject(const_cast<char*>(kLightModel), const_cast<char*>(kLightModel),
                             static_cast<FILE*>(pLightModel), 0);
#endif
    if (s_pLight != 0 && s_pScene != 0)
        s_pLight->AttachToScene(s_pScene);
}

int k2view_light_state()
{
    if (s_pLight == 0)
        return 0;
    Part* pPart = s_pLight->GetPart(const_cast<char*>(kLightPart));
    return pPart != 0 && pPart->AsPartLight() != 0 ? 2 : 1;
}

int k2view_set_model(const char* pszResRef)
{
    DeleteObject(s_pModel);
    s_animations.m_count = 0;
    __builtin_strncpy(s_szModel, pszResRef, sizeof(s_szModel) - 1);
    s_szModel[sizeof(s_szModel) - 1] = 0;
    if (s_szModel[0] == 0)
        return 0;
    s_pModel = NewCAurObject(s_szModel, const_cast<char*>("viewermodel"), 0, 0);
    if (s_pModel == 0)
        return 0;
    AttachModel();
    CollectAnimations();
    return 1;
}

const char* k2view_model_name()
{
    return s_szModel;
}

void k2view_model_bounds(float* pMin, float* pMax)
{
    Vector vMin, vMax;
    if (s_pModel != 0)
        s_pModel->GetMinimumBoundingBox(vMin, vMax);
    pMin[0] = vMin.x; pMin[1] = vMin.y; pMin[2] = vMin.z;
    pMax[0] = vMax.x; pMax[1] = vMax.y; pMax[2] = vMax.z;
}

void k2view_set_model_position(const float* pPos, float fFacing)
{
    s_afModelPos[0] = pPos[0];
    s_afModelPos[1] = pPos[1];
    s_afModelPos[2] = pPos[2];
    s_fModelFacing = fFacing;
    PlaceModel();
}

int k2view_animation_count()
{
    return s_animations.m_count;
}

const char* k2view_animation_name(int i)
{
    return i >= 0 && i < s_animations.m_count ? s_animations.m_pData[i] : "";
}

void k2view_play_animation(const char* pszName)
{
    if (s_pModel != 0)
        s_pModel->PlayAnimation(const_cast<char*>(pszName), 1.0f, 0, 0.0f);
}

int k2view_set_envmap(const char* pszTexture)
{
    __builtin_strncpy(s_szEnvMap, pszTexture, sizeof(s_szEnvMap) - 1);
    s_szEnvMap[sizeof(s_szEnvMap) - 1] = 0;
    if (s_pModel == 0)
        return 0;
    if (s_szEnvMap[0] == 0) {
        // The model's own maps come back with a fresh instance.
        char szModel[32];
        __builtin_strncpy(szModel, s_szModel, sizeof(szModel));
        return k2view_set_model(szModel);
    }
    return s_pModel->SetEnvironmentMap(s_szEnvMap);
}

void k2view_set_model_effects(int bShadow, int bStealth)
{
    s_bModelShadow = bShadow != 0;
    s_bModelStealth = bStealth != 0;
    ApplyModelEffects();
}

int k2view_model_can_cast_shadow()
{
    // Scene::DoGobShadowsForThisLight's test.
    Model* pModel = s_pModel != 0 ? static_cast<Gob*>(s_pModel)->m_pModel : 0;
    return pModel != 0 && (pModel->m_nFlags & (Model::kModelCharacter | Model::kModelItem)) != 0;
}

void k2view_set_grass(const K2ViewGrass* pGrass)
{
    s_bGrass = pGrass != 0;
    if (pGrass != 0)
        s_grass = *pGrass;
}

void k2view_set_lighting(const K2ViewLighting* pLighting)
{
    s_bLighting = pLighting != 0;
    if (pLighting != 0)
        s_lighting = *pLighting;
    ApplyLighting();
}

void k2view_set_light(int bOn, const float* pPos, const float* pColour, float fRadius, float fMultiplier)
{
    if (s_pLight == 0)
        return;
    Vector vColour = bOn ? Vector(pColour[0], pColour[1], pColour[2]) : Vector(0.0f, 0.0f, 0.0f);
    s_pLight->SetPosition(Vector(pPos[0], pPos[1], pPos[2]));
    if (Part* pMarker = s_pLight->GetPart(const_cast<char*>(kMarkerPart))) {
        if (PartTriMesh* pMesh = pMarker->AsPartTriMesh())
            pMesh->m_vSelfIllumColor = bOn ? vColour : Vector(0.15f, 0.15f, 0.15f);
    }
    Part* pPart = s_pLight->GetPart(const_cast<char*>(kLightPart));
    PartLight* pLight = pPart != 0 ? pPart->AsPartLight() : 0;
    if (pLight == 0)
        return;
    pLight->m_vLightColour = vColour;
    pLight->m_fLightRadius = fRadius;
    pLight->m_fLightScale = fMultiplier;
}

void k2view_set_wind(float fStrength, float fDirection, float fVariationA, float fVariationB)
{
    if (s_pScene == 0)
        return;
    // CSWCArea::SetWind's form: a horizontal wind of the given strength, its two deviations in degrees.
    WindManager* pWind = static_cast<Scene*>(s_pScene)->m_pWindManager;
    pWind->SetGlobalWind(Vector(__builtin_cosf(fDirection) * fStrength, __builtin_sinf(fDirection) * fStrength, 0.0f));
    pWind->SetMaximumDeviation(fVariationA * (3.14159265f / 180.0f), fVariationB * (3.14159265f / 180.0f));
}

void k2view_set_ambient(const float* pColour)
{
    s_vAmbient = Vector(pColour[0], pColour[1], pColour[2]);
    for (int i = 0; i < 3; i++)
        s_lighting.afAmbient[i] = pColour[i];
    if (s_pScene != 0)
        s_pScene->SetAmbient(s_vAmbient);
}

void k2view_set_camera(const float* pTarget, float fYaw, float fPitch, float fDist, float fFov)
{
    if (s_pCamera == 0)
        return;
    // The follow camera's placement (CAurBehaviorCameraFollow): it looks down its -Z, so it sits
    // along +Z of its orientation from the target.
    const float kDegrees = 57.29578f;   // YawPitchRoll takes degrees
    Quaternion q = YawPitchRoll(fYaw * kDegrees, fPitch * kDegrees, 0.0f);
    Vector vDir = q.zdir();
    s_pCamera->SetOrientation(q);
    s_pCamera->SetPosition(Vector(pTarget[0] + vDir.x * fDist, pTarget[1] + vDir.y * fDist,
                                  pTarget[2] + vDir.z * fDist));
    s_pCamera->SetViewAngle(fFov);

    // The smallest room whose box holds the target, else the one already current.
    CAurRoom* pBest = 0;
    float fBestArea = 0.0f;
    for (int i = 0; i < s_pScene->GetNumRooms(); i++) {
        CAurRoom* pRoom = s_pScene->GetRoom(i);
        const Vector& vMin = pRoom->m_bboxMin;
        const Vector& vMax = pRoom->m_bboxMax;
        if (pTarget[0] < vMin.x || pTarget[0] > vMax.x || pTarget[1] < vMin.y || pTarget[1] > vMax.y)
            continue;
        float fArea = (vMax.x - vMin.x) * (vMax.y - vMin.y);
        if (pBest == 0 || fArea < fBestArea) {
            pBest = pRoom;
            fBestArea = fArea;
        }
    }
    if (pBest != 0)
        s_pScene->SetCurrentRoom(pBest);
}

void k2view_render(float fDelta, int nWidth, int nHeight)
{
    if (s_pScene == 0 || s_pCamera == 0)
        return;
    // What the client's MainLoop does before it draws: the frame time, the resource manager's
    // pending loads, Aurora's per-frame texture pass (the controllers -- cycles, water -- run off
    // the frame counter it advances).
    DeltaT = fDelta;
    g_pExoResMan->Update(0);
    AurMakeCurrent();
    // The scene's focus, the player in the game: the shadows are cast from the light it picks.
    s_pScene->SetSceneFocus(Vector(s_afModelPos[0], s_afModelPos[1], s_afModelPos[2]));
    s_pScene->Animate(fDelta);
#if !KOTOR1
    UpdateDirt();
#endif
    ASLgl::glViewport(0, 0, nWidth, nHeight);
    s_pCamera->RenderScene();
}

void k2view_camera_axes(float* pRight, float* pUp, float* pBack)
{
    Quaternion q = s_pCamera != 0 ? s_pCamera->GetOrientation() : Quaternion();
    Vector vRight = q.xdir(), vUp = q.ydir(), vBack = q.zdir();
    pRight[0] = vRight.x; pRight[1] = vRight.y; pRight[2] = vRight.z;
    pUp[0] = vUp.x; pUp[1] = vUp.y; pUp[2] = vUp.z;
    pBack[0] = vBack.x; pBack[1] = vBack.y; pBack[2] = vBack.z;
}

void k2view_request_exit()
{
#if KOTOR1
    g_done = true;
#endif
}

int* k2view_switch(int i, const char** ppszName)
{
    static const struct {
        const char* pszName;
        int* pnValue;
    } s_switches[] = {
        {"Shadows", &enableshadows},
        {"Lightmaps", &enablelightmap},
        {"Bump maps", &enablebumpmap},
        {"Bumpy-shiny (EMBM; takes a backdrop reload)", &enablebumpyshiny},
        {"Grass", &enablegrass},
        {"Emitters", &enableemitters},
#if KOTOR1
        {"Performance mode (no frame-buffer passes)", &preferPerfToVisual},
#endif
    };
    if (i < 0 || i >= static_cast<int>(sizeof(s_switches) / sizeof(s_switches[0])))
        return 0;
    *ppszName = s_switches[i].pszName;
    return s_switches[i].pnValue;
}
