#include "k2stdint.h"
#include "nwcarea.h"
#include "AURORA/aurmodel.h"
#include "CExoArrayListImpl.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "nwccreature.h"
#include "aurscene.h"
#include "exosound.h"
#include "nwcmappin.h"
#include "nwcplaceable.h"
#include "nwsplaceable.h"
#include "nwcobject.h"
#include "nwctrigger.h"
#include "nwssoundobject.h"
#include "nwcambientsound.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "exostring.h"
#include "swminigame.h"
#include "swmgtrackfollower.h"
#include "nwcvisualeffect.h"
#include "gameobjectarray.h"
#include "nwcdoor.h"
#include "nwcanimbase.h"
#include "nwcmodule.h"
#include "nwsrules.h"
#include "clientoptions.h"
#include "Quaternion.h"
#include "guiingame.h"
#include "clientaimaster.h"
#include "worldtimer.h"
#include "serverexoappinternal.h"
#include "auremitter.h"
#include "lyt.h"
#include "aurgrass.h"
#include "swbehaviorattachsound.h"
#include "nwchandlemessage.h"
#include "serveranimation.h"

extern CExoSound* g_pExoSound;
extern int shadowalpha;
extern CAurCamera* g_pWeatherCamera;
extern float projectionalpha;
#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstdlib.h"

// The rows of surfacemat.2da the grass setup walks.
enum {
    SURFACE_MATERIAL_ROWS = 32,
};
extern bool s_oondarDone;
void AurGrassClearInfo();
int AurControlPart(char* cmd);
void SetClearColor(char* s);
int AurSetGlobalWind(Vector v);
int AurSetGlobalWindVariation(float a, float b);

CAurScene* NewCAurScene(char* pszName);
void AurGrassAddInfo(int nRow, const CAurGrassInfo& info);
void AurEnableInterleavedAnimations();
void AurDisableInterleavedAnimations();
void AurTextureMakeCurrent();
void AurTextureSuspendUpdates();
void AurTextureResumeUpdates();
void PlayRoomSoundEvent(CAurObject* pObject, char* pszEvent, void* pUserData);

int RyanTotalLevelVertices;
int RyanTotalLevelVertSize;
int RyanStartTextureSize;
int RyanStartParts;
int RyanEndParts;
int RyanEndTextureSize;
int RyanLevelTextureSize;
int RyanLevelTotalParts;
int RyanLevelVertSize;
int RyanLevelVertices;
extern int usedtexturememory;
extern int countpart;

static volatile int s_shadowAlphaSaved;

static const float s_fWindScale[3]      = {0.0f, 1.0f, 2.0f};
static const float s_fWindVariationA[3] = {0.0f, 100.0f, 150.0f};
static const float s_fWindVariationB[3] = {0.0f, 3.0f, 5.0f};

// A two-lane int vector, used where a dawn/dusk pair is written in a single store.
#ifdef _MSC_VER
struct CSWCAreaV2si { int x, y; };
#else
typedef int CSWCAreaV2si __attribute__((ext_vector_type(2)));
#endif

CSWCArea* CSWCArea::AsSWCArea()
{
    return this;
}

// A client-side stub; line of sight always reports clear.
int CSWCArea::ClearLineOfSightOneWay(Vector from, Vector to, Vector* pOut)
{
    K2_UNUSED(from); K2_UNUSED(to); K2_UNUSED(pOut);
    return 1;
}

// A client-side stub that always reports walkable.
int CSWCArea::TestLineWalkable(CSWCCreature* pCreature, Vector from, Vector to, float f)
{
    K2_UNUSED(pCreature); K2_UNUSED(from); K2_UNUSED(to); K2_UNUSED(f);
    return 1;
}

int CSWCArea::GetObjectsAtPositionVisible(const Vector& pos)
{
    K2_UNUSED(pos);
    return 1;
}

int CSWCArea::LoadBorder()
{
    return 1;
}

// Splits a bit-compressed tile address into remainder and quotient against m_nWidth.
// A null this returns 0 without touching the out-params.
int CSWCArea::TranslateBitCompressedAddress(int a, int b, int* outRem, int* outQuo)
{
    if (reinterpret_cast<uintptr_t>(this) == 0)
        return 0;
    int n = m_nWidth;
    int t = b + (a << 3);
    *outQuo = t / n;
    *outRem = t % n;
    return 1;
}

int CSWCArea::GetRoomIndex(const Vector& pos, int a)
{
    return g_pAppManager->m_pServerExoApp->GetRoomIndex(m_areaResRef, pos, a);
}

void CSWCArea::EmptyCAurObjectToHitCheckIgnoreList()
{
    while (m_hitCheckIgnoreList.m_pInternal->m_pHead != 0)
        m_hitCheckIgnoreList.m_pInternal->RemoveHead();
}

void CSWCArea::AddCAurObjectToHitCheckIgnoreList(CAurObject* pObject)
{
    CExoLinkedListNode* pPos = m_hitCheckIgnoreList.GetHeadPos();
    CAurObject* pData = m_hitCheckIgnoreList.GetHead();
    while (pData != pObject && pPos != 0)
        pData = m_hitCheckIgnoreList.GetNext(pPos);
    if (pData == 0)
        m_hitCheckIgnoreList.AddTail(pObject);
}

int CSWCArea::TestDirectLine(unsigned long long objectId, const Vector& from, const Vector& to,
                             const float& hitFraction, float f, int g, CWalkHitInfo* pHit)
{
    if (g_pAppManager->m_pServerExoApp != 0)
        return g_pAppManager->m_pServerExoApp->TestDirectLine(objectId, m_areaResRef, from, to,
                                                              hitFraction, f, g, pHit);
    return 0;
}

void CSWCArea::UpdateShadowingLights()
{
    if (m_pSunGob != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(1);
    }
}

int CSWCArea::GetIsObjectCorpse(unsigned long long objectId)
{
    for (int i = 0; i < 3; i++)
        if (m_aCorpseIds[i] == objectId)
            return 1;
    return m_aCorpseIds[3] == objectId;
}

void CSWCArea::ClearTilePathNodes()
{
    if (m_tilePathNodesB.m_pData != 0) {
        delete[] m_tilePathNodesB.m_pData;
        m_tilePathNodesB.m_pData = 0;
    }
    m_tilePathNodesB.m_count = 0;
    m_tilePathNodesB.m_capacity = 0;
    if (m_tilePathNodesA.m_pData != 0) {
        delete[] m_tilePathNodesA.m_pData;
        m_tilePathNodesA.m_pData = 0;
    }
    m_tilePathNodesA.m_count = 0;
    m_tilePathNodesA.m_capacity = 0;
}

void* CSWCArea::GetPlaceableObjectLight(unsigned long long objectId)
{
    for (int i = 0; i < m_placeableObjectLights.m_count; i++) {
        CSWCPlaceableObjectLight* pEntry = m_placeableObjectLights.m_pArray[i];
        if (pEntry != 0 && pEntry->m_objectId == objectId)
            return pEntry->m_pLight;
    }
    return 0;
}

#if KOTOR1
int CSWCArea::AddPlaceableObjectLight(unsigned long long objectId, unsigned char nPlaceableId, Vector pos)
#else
int CSWCArea::AddPlaceableObjectLight(unsigned long long objectId, int nPlaceableId, Vector pos)
#endif
{
    Quaternion qRoot = {1.0f, 0.0f, 0.0f, 0.0f};
    Vector vColor = {0.0f, 0.0f, 0.0f};
    Vector vOffset = {0.0f, 0.0f, 0.0f};
    Vector vRootPos = {0.0f, 0.0f, 0.0f};
    int nColorRow;
    int bGot;
    {
        C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
        CExoString sColumn("LightColor");
        bGot = p2DA->GetINTEntry(nPlaceableId, sColumn, &nColorRow);
    }
    if (bGot != 0) {
        CSWCPlaceableObjectLight* pEntry = nullptr;
        bool bFound = false;
        for (int i = 0; i < m_placeableObjectLights.m_count; i++) {
            pEntry = m_placeableObjectLights.m_pArray[i];
            if (pEntry != 0 && pEntry->m_objectId == objectId) {
                bFound = true;
                break;
            }
        }
        if (!bFound)
            pEntry = new CSWCPlaceableObjectLight;

        pEntry->m_pLight = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_placeable01"), const_cast<char*>("placeablelight"),
                                               0, 0));
        if (pEntry->m_pLight == 0) {
            delete pEntry;
            return 0;
        }
        pEntry->m_objectId = objectId;

        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pLightColor;
            int nRow = nColorRow;
            CExoString sCol("RED");
            p2DA->GetFLOATEntry(nRow, sCol, &vColor.x);
        }
        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pLightColor;
            int nRow = nColorRow;
            CExoString sCol("GREEN");
            p2DA->GetFLOATEntry(nRow, sCol, &vColor.y);
        }
        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pLightColor;
            int nRow = nColorRow;
            CExoString sCol("BLUE");
            p2DA->GetFLOATEntry(nRow, sCol, &vColor.z);
        }
        int bGotX;
        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
            CExoString sCol("LightOffsetX");
            bGotX = p2DA->GetFLOATEntry(nPlaceableId, sCol, &vOffset.x);
        }
        if (bGotX == 0)
            vOffset.x = 0.0f;
        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
            CExoString sCol("LightOffsetY");
            bGotX = p2DA->GetFLOATEntry(nPlaceableId, sCol, &vOffset.y);
        }
        if (bGotX == 0)
            vOffset.y = 0.0f;
        {
            C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
            CExoString sCol("LightOffsetZ");
            bGotX = p2DA->GetFLOATEntry(nPlaceableId, sCol, &vOffset.z);
        }
        if (bGotX == 0)
            vOffset.z = 3.0f;

        pEntry->m_pLight->GetPartLocalPosition(const_cast<char*>("root"), &vRootPos, &qRoot);
        Vector vNewPos;
        vNewPos.x = vRootPos.x + vOffset.x;
        vNewPos.y = vRootPos.y + vOffset.y;
        vNewPos.z = vRootPos.z + vOffset.z;
        pEntry->m_pLight->SetPartLocalPosition(const_cast<char*>("root"), vNewPos, qRoot);
        pEntry->m_pLight->SetPosition(pos);
        pEntry->m_pLight->AttachToScene(m_pScene);

            if (bFound == false)
                m_placeableObjectLights.Add(pEntry);
    }
    return 1;
}

#if KOTOR1
int CSWCArea::AddPlaceableObjectStatic(unsigned long long objectId, unsigned char nPlaceableId,
#else
int CSWCArea::AddPlaceableObjectStatic(unsigned long long objectId, int nPlaceableId,
#endif
                                       Vector vPos, Vector vFacing)
{
    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    CSWCPlaceable* pPlaceable =
        g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(objectId);
    if (pPlaceable == 0) {
        CExoString sModelName;
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
        if (pOptions->m_nViolenceLevel != 0) {
            CExoString sColumn("ModelName");
            p2DA->GetCExoStringEntry(nPlaceableId, sColumn, &sModelName);
        } else {
            int bGot;
            {
                CExoString sColumn("LowGore");
                bGot = p2DA->GetCExoStringEntry(nPlaceableId, sColumn, &sModelName);
            }
            if (bGot == 0) {
                p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
                CExoString sColumn("ModelName");
                p2DA->GetCExoStringEntry(nPlaceableId, sColumn, &sModelName);
            }
        }
        pPlaceable = new CSWCPlaceable;
        pPlaceable->SetAppearance(nPlaceableId);
        pPlaceable->SetId(objectId);
        pPlaceable->SetArea(this);
        if (pArray->AddExternalObject(objectId, static_cast<CGameObject*>(pPlaceable), 1) !=
            CGameObjectArray::SUCCESS)
            return 0;
        pPlaceable->SetPosition(vPos);
        pPlaceable->SetOrientation(vFacing);
        // The yaw was already applied from the raw Vector above, so this call's result is unused.
        YawPitchRoll(Yaw(vFacing), 0.0f, 0.0f);
    }
    pPlaceable->m_bPlot = 1;
    pPlaceable->m_bLockable = 0;
    pPlaceable->m_bLocked = 0;
    pPlaceable->m_bHasDescription = 0;
    pPlaceable->m_bHostileFaction = 0;
    pPlaceable->m_bIgnoreHitcheck = 0;
    pPlaceable->m_bReserved1 = 0;
    pPlaceable->m_bTrapDisarmable = 0;
    pPlaceable->SetAnimation(0, 0, 0);
    g_pAppManager->m_pClientExoApp->RemoveObjectFromHitcheckIgnoreList(objectId);
    m_staticPlaceableIdList.Add(objectId);
    return 1;
}

void CSWCArea::RemovePlaceableObjectLight(unsigned long long objectId, int bDeleteLight)
{
    for (int i = 0; i < m_placeableObjectLights.m_count; i++) {
        CSWCPlaceableObjectLight* pEntry = m_placeableObjectLights.m_pArray[i];
        if (pEntry != 0 && pEntry->m_objectId == objectId) {
            if (bDeleteLight != 0)
                delete pEntry->m_pLight;
            delete m_placeableObjectLights.m_pArray[i];
            m_placeableObjectLights.DelIndex(i);
            return;
        }
    }
}

int CSWCArea::GetIsDark()
{
    if (m_pScene != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(0);
        m_bIsDark = 0;
    }
    return 0;
}

void CSWCArea::ForceBrightSunAndMoon()
{
    m_sunFogOn = 0;
    m_vMoonAmbientRG = CSWCAreaVector2(0.0f, 0.0f);
    m_fMoonAmbientBlue = 0.0f;
    m_fSunAmbientBlue = 0.0f;
    m_vSunAmbientRG = m_vMoonAmbientRG;
    m_vMoonDiffuseRG = CSWCAreaVector2(1.0f, 1.0f);
    m_fMoonDiffuseBlue = 1.0f;
    m_fSunDiffuseBlue = 1.0f;
    m_vSunDiffuseRG = m_vMoonDiffuseRG;
    if (m_isNight != 0)
        SetAsNight();
    else
        SetAsDay();
}

void CSWCArea::SetAsDawn()
{
    if (m_dayNightCycle == 0) {
        if (m_isNight != 0) {
            SetAsNight();
            return;
        }
        SetAsDay();
        return;
    }
    projectionalpha = 0.0f;
    *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
    m_bDayNightSwitchPending = 1;
    *reinterpret_cast<CSWCAreaV2si*>(&m_v2iDawnDusk) = CSWCAreaV2si{1, 0};
    m_isNight = 1;
    if (m_pSunGob != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(1);
    }
}

void CSWCArea::SetAsDusk()
{
    if (m_dayNightCycle == 0) {
        if (m_isNight != 0) {
            SetAsNight();
            return;
        }
        SetAsDay();
        return;
    }
    projectionalpha = 0.05f;
    *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
    m_bDayNightSwitchPending = 1;
    *reinterpret_cast<CSWCAreaV2si*>(&m_v2iDawnDusk) = CSWCAreaV2si{0, 1};
    m_isNight = 0;
    if (m_pSunGob != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(1);
    }
}

void CSWCArea::SetAsDay()
{
    if (m_dayNightCycle == 0 && m_isNight != 0) {
        SetAsNight();
        return;
    }
    projectionalpha = 0.05f;
    *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
    m_bDayNightSwitchPending = 1;
    *reinterpret_cast<CSWCAreaV2si*>(&m_v2iDawnDusk) = CSWCAreaV2si{0, 0};
    m_isNight = 0;
    SetFog(m_sunFogNear, m_sunFogFar, m_vDayFogColor);
    SetGlobalLightAmbient(1, Vector{m_vSunAmbientRG.x, m_vSunAmbientRG.y, m_fSunAmbientBlue});
    SetGlobalLightDiffuse(1, Vector{m_vSunDiffuseRG.x, m_vSunDiffuseRG.y, m_fSunDiffuseBlue});
    if (m_pSunGob != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(1);
    }
}

void CSWCArea::SetAsNight()
{
    if (m_dayNightCycle == 0 && m_isNight == 0) {
        SetAsDay();
        return;
    }
    projectionalpha = 0.0f;
    *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
    m_bDayNightSwitchPending = 1;
    *reinterpret_cast<CSWCAreaV2si*>(&m_v2iDawnDusk) = CSWCAreaV2si{0, 0};
    m_isNight = 1;
    SetFog(m_moonFogNear, m_moonFogFar, m_vNightFogColor);
    SetGlobalLightAmbient(1, Vector{m_vMoonAmbientRG.x, m_vMoonAmbientRG.y, m_fMoonAmbientBlue});
    SetGlobalLightDiffuse(1, Vector{m_vMoonDiffuseRG.x, m_vMoonDiffuseRG.y, m_fMoonDiffuseBlue});
    if (m_pSunGob != 0) {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer != 0)
            pPlayer->GetCreatureLight(1);
    }
}

void CSWCArea::AttachmentFromServerArea(CSWSArea* pServerArea)
{
    m_pServerArea = pServerArea;
}

void CSWCArea::DetachFromServerArea()
{
    m_pServerArea = 0;
}

void CSWCArea::StartWeather(unsigned char nWeatherType)
{
    m_weatherType = nWeatherType;
    m_weatherTransitionTime = 6000;
}

// The weather-type argument is ignored; the area always clears to calm.
void CSWCArea::StopWeather(unsigned char nWeatherType)
{
    K2_UNUSED(nWeatherType);
    m_weatherType = WEATHER_CLEAR;
    m_weatherTransitionTime = 6000;
}

int CSWCArea::SetWeather(unsigned char nWeather, float f)
{
    CExoString sAnim;
    if (f != 0.0f) {
        if (m_nCurrentWeather != nWeather) {
            if (m_apWeatherGobs[0] != 0) { delete m_apWeatherGobs[0]; m_apWeatherGobs[0] = 0; }
            if (m_apWeatherGobs[1] != 0) { delete m_apWeatherGobs[1]; m_apWeatherGobs[1] = 0; }
            if (m_apWeatherGobs[2] != 0) { delete m_apWeatherGobs[2]; m_apWeatherGobs[2] = 0; }
            if (m_apWeatherGobs[3] != 0) { delete m_apWeatherGobs[3]; m_apWeatherGobs[3] = 0; }
            if (m_apWeatherGobs[4] != 0) { delete m_apWeatherGobs[4]; m_apWeatherGobs[4] = 0; }
            if (m_apWeatherGobs[5] != 0) { delete m_apWeatherGobs[5]; m_apWeatherGobs[5] = 0; }
            switch (nWeather) {
            case WEATHER_CLEAR:
                break;
            case WEATHER_SNOW:
                m_apWeatherGobs[0] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[0] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[0]->SetEmitterAsWeather(1);
#endif
                m_apWeatherGobs[1] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[1] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[1]->SetEmitterAsWeather(1);
#endif
                m_apWeatherGobs[2] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[2] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[2]->SetEmitterAsWeather(1);
#endif
                m_apWeatherGobs[3] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[3] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[3]->SetEmitterAsWeather(1);
#endif
                m_apWeatherGobs[4] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[4] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[4]->SetEmitterAsWeather(1);
#endif
                m_apWeatherGobs[5] = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_snow"), const_cast<char*>("weather"), 0, 0));
                if (m_apWeatherGobs[5] == 0) return 0;
#if !KOTOR1
                m_apWeatherGobs[5]->SetEmitterAsWeather(1);
#endif
                break;
            case WEATHER_RAIN:
#if !KOTOR1
                if (m_chanceRain != 0) {
#else
                {
#endif
                    Gob* pWeatherGob;
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[0] = pWeatherGob;
                    if (m_apWeatherGobs[0] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[1] = pWeatherGob;
                    if (m_apWeatherGobs[1] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[2] = pWeatherGob;
                    if (m_apWeatherGobs[2] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[3] = pWeatherGob;
                    if (m_apWeatherGobs[3] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[4] = pWeatherGob;
                    if (m_apWeatherGobs[4] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                    pWeatherGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_rain"), const_cast<char*>("weather"), 0, 0));
                    m_apWeatherGobs[5] = pWeatherGob;
                    if (m_apWeatherGobs[5] == 0) return 0;
#if !KOTOR1
                    pWeatherGob->SetEmitterAsWeather(1);
#endif
                }
                break;
            default:
                return 0;
            }
            if (m_apWeatherGobs[0] != 0) m_apWeatherGobs[0]->AttachToScene(m_pScene);
            if (m_apWeatherGobs[1] != 0) m_apWeatherGobs[1]->AttachToScene(m_pScene);
            if (m_apWeatherGobs[2] != 0) m_apWeatherGobs[2]->AttachToScene(m_pScene);
            if (m_apWeatherGobs[3] != 0) m_apWeatherGobs[3]->AttachToScene(m_pScene);
            if (m_apWeatherGobs[4] != 0) m_apWeatherGobs[4]->AttachToScene(m_pScene);
            if (m_apWeatherGobs[5] != 0) m_apWeatherGobs[5]->AttachToScene(m_pScene);
            m_nCurrentWeather = nWeather;
        }
        if (m_chanceLightning != 0) {
            unsigned char nLightningWeather = nWeather;
            if (m_pLightningGob != 0) {
                delete m_pLightningGob;
                nLightningWeather = m_nCurrentWeather;
                m_pLightningGob = 0;
            }
            if (nLightningWeather == WEATHER_RAIN) {
                m_pLightningGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_lightning01"), const_cast<char*>("weatherlight"), 0, 0));
            } else if (nLightningWeather == WEATHER_SNOW) {
                m_pLightningGob = static_cast<Gob*>(NewCAurObject(const_cast<char*>("fx_lightning01"), const_cast<char*>("weatherlight"), 0, 0));
            } else {
                return 0;
            }
            if (m_pLightningGob != 0)
                m_pLightningGob->AttachToScene(m_pScene);
        } else {
            if (m_pLightningGob != 0) {
                delete m_pLightningGob;
                m_pLightningGob = 0;
            }
        }
    } else {
        if (m_apWeatherGobs[0] != 0) { delete m_apWeatherGobs[0]; m_apWeatherGobs[0] = 0; }
        if (m_apWeatherGobs[1] != 0) { delete m_apWeatherGobs[1]; m_apWeatherGobs[1] = 0; }
        if (m_apWeatherGobs[2] != 0) { delete m_apWeatherGobs[2]; m_apWeatherGobs[2] = 0; }
        if (m_apWeatherGobs[3] != 0) { delete m_apWeatherGobs[3]; m_apWeatherGobs[3] = 0; }
        if (m_apWeatherGobs[4] != 0) { delete m_apWeatherGobs[4]; m_apWeatherGobs[4] = 0; }
        if (m_apWeatherGobs[5] != 0) { delete m_apWeatherGobs[5]; m_apWeatherGobs[5] = 0; }
        if (m_pLightningGob != 0) { delete m_pLightningGob; m_pLightningGob = 0; }
        m_nCurrentWeather = WEATHER_CLEAR;
    }
    if (nWeather == WEATHER_RAIN)
        sAnim = const_cast<char*>("rain");
    else if (nWeather == WEATHER_SNOW)
        sAnim = const_cast<char*>("snow");
    float fSpeed = f / 100.0f;
    for (int i = 0; i < 6; i++) {
        if (m_apWeatherGobs[i] != 0) {
            m_apWeatherGobs[i]->PlayAnimation(static_cast<char*>(sAnim.CStr()), 0.0f, AnimRun::kAnimRemove, 0.0f);
            m_apWeatherGobs[i]->PlayAnimation(static_cast<char*>(sAnim.CStr()), fSpeed, 0, 0.0f);
            float fLen, fCur;
            m_apWeatherGobs[i]->GetAnimation(static_cast<char*>(sAnim.CStr()), &fLen, &fCur);
            static_cast<CAurObject*>(m_apWeatherGobs[i])->AnimateExternal(fCur);
            m_apWeatherGobs[i]->PlayAnimation(static_cast<char*>(sAnim.CStr()), 0.0f, 0, 0.0f);
        }
    }
    return 1;
}

// EFFECTIVE: the sound loop's entry guard is cmp #1/b.lt here, cmp #0/b.le in the orig.
void CSWCArea::UnloadArea()
{
#if !KOTOR1
    s_oondarDone = 0;
#endif
    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->FadeAndStopDialogAmbientTrack();
    AurGrassClearInfo();
    ClearTilePathNodes();
    m_triggerIdList.SetSize(0);
    if (m_apWeatherGobs[0] != 0) { delete m_apWeatherGobs[0]; m_apWeatherGobs[0] = 0; }
    if (m_apWeatherGobs[1] != 0) { delete m_apWeatherGobs[1]; m_apWeatherGobs[1] = 0; }
    if (m_apWeatherGobs[2] != 0) { delete m_apWeatherGobs[2]; m_apWeatherGobs[2] = 0; }
    if (m_apWeatherGobs[3] != 0) { delete m_apWeatherGobs[3]; m_apWeatherGobs[3] = 0; }
    if (m_apWeatherGobs[4] != 0) { delete m_apWeatherGobs[4]; m_apWeatherGobs[4] = 0; }
    if (m_apWeatherGobs[5] != 0) { delete m_apWeatherGobs[5]; m_apWeatherGobs[5] = 0; }
    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    CClientAIMaster* pAIMaster = g_pAppManager->m_pClientExoApp->GetClientAIMaster();
    int nSounds = m_soundObjectIdList.m_count;
    if (nSounds > 0) {
        int i = 0;
        do {
            CSWCSoundObject* pSound = g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(
                m_soundObjectIdList.m_pArray[i]);
            if (pSound != 0) {
                pSound->Stop();
                pAIMaster->RemoveObject(static_cast<CSWCObject*>(pSound));
                pArray->Delete(m_soundObjectIdList.m_pArray[i]);
                delete pSound;
            }
            i++;
        } while (i != nSounds);
    }
    m_soundObjectIdList.SetSize(0);
    if (m_pAmbientSound != 0)
        m_pAmbientSound->StopSounds(0);
    if (m_pLightningGob != 0) {
        delete m_pLightningGob;
        m_pLightningGob = 0;
    }
    m_vWeatherPercentWindTimer.x = 0.0f;
    m_vWeatherPercentWindTimer.y = 0.0f;
    m_fLightningDelay = 0.0f;
    m_fLightningFlashTime = 0.0f;
    m_fReserved1 = 0.0f;
    m_weatherTransitionTime = 0;
    m_nCurrentWeather = WEATHER_CLEAR;
    m_weatherType = WEATHER_CLEAR;
    ClearAllMapPins();
    if (m_pSunGob != 0) {
        delete m_pSunGob;
        m_pSunGob = 0;
    }
    if (m_pScene != 0) {
        delete m_pScene;
        m_pScene = 0;
    }
    while (m_behaviorAttachSounds.m_pInternal->m_pHead != 0) {
        CSWBehaviorAttachSound* pObject = m_behaviorAttachSounds.RemoveHead();
        if (pObject != 0)
            delete pObject;
    }
    while (m_hitCheckIgnoreList.m_pInternal->m_pHead != 0)
        m_hitCheckIgnoreList.RemoveHead();
    for (int i = m_placeableObjectLights.m_count - 1; i >= 0; i--) {
        CSWCPlaceableObjectLight* pEntry = m_placeableObjectLights.m_pArray[i];
        if (pEntry != 0) {
            if (pEntry->m_pLight != 0)
                delete pEntry->m_pLight;
            delete m_placeableObjectLights.m_pArray[i];
            m_placeableObjectLights.DelIndex(i);
        }
    }
    for (int i = m_staticPlaceableIdList.m_count - 1; i >= 0; i--) {
        CGameObject* pObj;
        CGameObjectArray* pArr = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
        if (pArr->Delete(m_staticPlaceableIdList.m_pArray[i], &pObj) == CGameObjectArray::SUCCESS &&
            pObj != 0 && pObj->AsSWCPlaceable() != 0 && pObj != 0)
            delete pObj;
        m_staticPlaceableIdList.DelIndex(i);
    }
    if (m_pReserved1 != 0) {
        operator delete[](m_pReserved1);
        m_pReserved1 = 0;
        m_nReserved2 = 0xffff;
        m_nReserved3 = 0xff;
        m_nReserved4 = 0;
    }
    for (int i = 0; i < 4; i++) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            m_aCorpseIds[i]);
        if (pCreature != 0) {
            CClientAIMaster* pMaster = g_pAppManager->m_pClientExoApp->GetClientAIMaster();
            pMaster->m_list.RemoveObject(static_cast<CSWCObject*>(pCreature));
            CGameObject* pObj;
            CGameObjectArray* pArr = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
            if (pArr->Delete(m_aCorpseIds[i], &pObj) == CGameObjectArray::SUCCESS &&
                pObj != 0 && static_cast<CSWCCreature*>(pObj->AsSWCCreature()) == pCreature)
                delete pCreature;
        }
        m_aCorpseIds[i] = OBJECT_INVALID;
    }
    m_nOldestCorpse = 0;
    m_nNextCorpse = 1;
}

CSWCRoom* CSWCArea::GetRoom(int nRoom)
{
    if (nRoom == -1)
        return 0;
    return &m_paRooms[nRoom];
}

CSWCMapPin* CSWCArea::GetMapPin(unsigned int nId, int nType)
{
    if (m_mapPins.m_count >= 1) {
        uint64_t n = static_cast<unsigned int>(m_mapPins.m_count);
        CSWCMapPin** pp = m_mapPins.m_pArray;
        do {
            CSWCMapPin* pPin = *pp;
            if (pPin != 0 && pPin->m_nPinId == nId && pPin->m_nPinType == nType)
                return pPin;
            pp++;
        } while (--n != 0);
    }
    return 0;
}

void CSWCArea::SetHideFadeGeometryMode(unsigned char mode)
{
    if (m_pScene == 0)
        return;
    int nMode = mode;
    switch (mode) {
    case 0:
        nMode = CAurScene::ENABLE_FADE;
        break;
    case 1:
        nMode = CAurScene::DISABLE_FADE;
        break;
    case 2:
        nMode = CAurScene::ALWAYS_FADE;
        break;
    }
    m_pScene->SetGeomFadeProperties(nMode);
}

int CSWCArea::SetFog(float fNear, float fFar, const Vector& color)
{
    if (m_pScene == 0)
        return 0;
    m_pScene->SetFogRange(fNear, fFar);
    m_fogNear = fNear;
    m_fogFar = fFar;
    SetFogColor(color);
    return 1;
}

// Pushes the fog color to the renderer as a "r g b" console string and caches it in m_fogColor.
int CSWCArea::SetFogColor(const Vector& color)
{
    if (m_pScene == 0)
        return 0;
    m_pScene->SetFogColor(color);
    CExoString s;
    s.Format(const_cast<char*>("%f %f %f"), color.x, color.y, color.z);
    SetClearColor(static_cast<char*>(s.CStr()));
    m_fogColor = color;
    return 1;
}

int CSWCArea::SetWind(unsigned char nWind)
{
    Vector v;
    v.x = 1.0f;
    v.y = 1.0f;
    v.z = 0.0f;
    v = normalize(v);
    if (nWind <= 2) {
        float k = s_fWindScale[static_cast<signed char>(nWind)];
        float b = s_fWindVariationB[static_cast<signed char>(nWind)];
        float a = s_fWindVariationA[static_cast<signed char>(nWind)];
        AurSetGlobalWind(Vector{v.x * k, v.y * k, v.z * k});
        AurSetGlobalWindVariation(a, b);
        m_windPower = nWind;
        return 1;
    }
    return 0;
}

CSWMiniGame* CSWCArea::RestartMiniGame()
{
    if (m_pMiniGame == 0)
        return 0;
    CSWMiniGame* pNew = new CSWMiniGame(this);
    int nTracks = m_pMiniGame->m_protoTracks.m_count;
    for (int i = 0; i < nTracks; i++) {
        SWMGTrackInfo* pTrack = m_pMiniGame->GetProtoTrack(i);
        pNew->AddTrack(pTrack->m_resTrack, pTrack->m_vPosition);
    }
    int nObstacles = m_pMiniGame->m_obstacleIds.m_count;
    for (int i = 0; i < nObstacles; i++) {
        CSWMGObstacle* pObstacle = m_pMiniGame->GetObstacle(i);
        CAurObject* pGob = pObstacle->m_pGob;
        CResRef resName(static_cast<char*>(pGob->GetModelName()));  // CAurObject::GetModelName is void*
        pNew->AddObstacle(resName, pGob->GetPosition());
    }
    if (pNew->Load() == 0)
        return 0;
    pNew->SetCamera(m_pMiniGame->m_pCamera);
    delete m_pMiniGame;
    m_pMiniGame = pNew;
    pNew->Go();
    return m_pMiniGame;
}

CSWCRoom* CSWCArea::GetRoom(const Vector& pos)
{
    int nRoom = g_pAppManager->m_pServerExoApp->GetRoomIndex(m_areaResRef, pos, 0);
    if (nRoom == -1)
        return 0;
    return &m_paRooms[nRoom];
}

CSWCRoom* CSWCArea::GetRoom(const CExoString& sRoom)
{
    CSWCRoom* pFound = 0;
    for (int i = 0; i < m_nRooms; i++) {
        CResRef resRoom = m_paRooms[i].m_resRoom;
        CExoString sName;
        resRoom.CopyToString(sName);
        if (sRoom.CompareNoCase(sName.CStr()) != 0) {
            pFound = &m_paRooms[i];
            break;
        }
    }
    return pFound;
}

// Resolves the server area from this area's object id on first use, then back-attaches.
CSWSArea* CSWCArea::GetServerArea()
{
    if (m_pServerArea == 0) {
        CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
        m_pServerArea = pApp->GetAreaByGameObjectID(pApp->ClientToServerObjectId(m_objectId));
        if (m_pServerArea != 0)
            m_pServerArea->AttachmentFromClientArea(this);
    }
    return m_pServerArea;
}

void CSWCArea::StartSounds()
{
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
    g_pAppManager->m_pClientExoApp->StopMenuMusic();
#if KOTOR1
    g_pAppManager->m_pClientExoApp->StopLoadMusic();
    m_pAmbientSound->StartSounds();
#else
    m_pAmbientSound->StartSounds();
    g_pAppManager->m_pClientExoApp->StopLoadMusic();
#endif
}

void CSWCArea::DisplayAllTriggers(int b)
{
    for (int i = m_triggerIdList.m_count - 1; i >= 0; i--) {
        CSWCTrigger* pTrigger = g_pAppManager->m_pClientExoApp->GetTriggerByGameObjectID(
            m_triggerIdList.m_pArray[i]);
        pTrigger->SpecialDisplayTrigger(b);
    }
}

void CSWCArea::UpdateSoundObjectsTimeOfDay()
{
    int nCount = m_soundObjectIdList.m_count;
    for (int i = 0; i < nCount; i++) {
        CSWCSoundObject* pSound = g_pAppManager->m_pClientExoApp->GetSoundObjectByGameObjectID(
            m_soundObjectIdList.m_pArray[i]);
        if (pSound != 0)
            pSound->UpdateTimeOfDay();
    }
}

// Drives the day/night transition. While one is latched it lerps the sun ambient and diffuse
// colors, the fog color and the fog near/far between the night and day sets against the module's
// time-of-day fraction, settles on the end state (dawn to day, dusk to night) at fraction 1, then
// updates every sound object and the ambient sound. deltaTime is unused: the fraction comes from the
// module clock.
void CSWCArea::UpdateDayNightCycle(float deltaTime)
{
    K2_UNUSED(deltaTime);
    Vector vFogColor = {0.0f, 0.0f, 0.0f};
    Vector vSunAmb;
    Vector vSunDiff;
    Vector vFog;
    if (m_v2iDawnDusk.x == 0 && m_v2iDawnDusk.y == 0)
        return;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0)
        return;
    float fTime = pModule->m_fTimeOfDay;
    float fMax = pModule->m_fHourLengthSec;
    if (m_bDayNightTransitioning == 0)
        m_bDayNightTransitioning = 1;
    if (m_v2iDawnDusk.x != 0) {
        if (fTime >= fMax) {
            // dawn complete: settle on day
            projectionalpha = 0.05f;
            *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
            m_bDayNightTransitioning = 0;
            m_v2iDawnDusk.x = 0;
            vSunAmb.x = m_vSunAmbientRG.x;
            vSunAmb.y = m_vSunAmbientRG.y;
            vSunAmb.z = m_fSunAmbientBlue;
            vSunDiff.x = m_vSunDiffuseRG.x;
            vSunDiff.y = m_vSunDiffuseRG.y;
            vSunDiff.z = m_fSunDiffuseBlue;
            vFogColor = m_vDayFogColor;
            vFog.x = m_sunFogNear;
            vFog.y = m_sunFogFar;
            if (m_pSunGob != 0) {
                CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
                if (pPlayer != 0)
                    pPlayer->GetCreatureLight(1);
            }
            m_isNight = 0;
        } else {
            // dawn in progress: night -> day
            float fRatio = fTime / fMax;
            projectionalpha = fRatio * 0.05f + 0.0f;
            vSunAmb.x = m_vMoonAmbientRG.x + fRatio * (m_vSunAmbientRG.x - m_vMoonAmbientRG.x);
            vSunAmb.y = m_vMoonAmbientRG.y + fRatio * (m_vSunAmbientRG.y - m_vMoonAmbientRG.y);
            vFogColor.x = m_vNightFogColor.x + fRatio * (m_vDayFogColor.x - m_vNightFogColor.x);
            vFogColor.y = m_vNightFogColor.y + fRatio * (m_vDayFogColor.y - m_vNightFogColor.y);
            vFogColor.z = m_vNightFogColor.z + fRatio * (m_vDayFogColor.z - m_vNightFogColor.z);
            vSunAmb.z = m_fMoonAmbientBlue + fRatio * (m_fSunAmbientBlue - m_fMoonAmbientBlue);
            vSunDiff.x = m_vMoonDiffuseRG.x + fRatio * (m_vSunDiffuseRG.x - m_vMoonDiffuseRG.x);
            vSunDiff.y = m_vMoonDiffuseRG.y + fRatio * (m_vSunDiffuseRG.y - m_vMoonDiffuseRG.y);
            vSunDiff.z = m_fMoonDiffuseBlue + fRatio * (m_fSunDiffuseBlue - m_fMoonDiffuseBlue);
            vFog.x = m_moonFogNear + fRatio * (m_sunFogNear - m_moonFogNear);
            vFog.y = m_moonFogFar + fRatio * (m_sunFogFar - m_moonFogFar);
        }
    } else if (m_v2iDawnDusk.y != 0) {
        if (fTime >= fMax) {
            // dusk complete: settle on night
            projectionalpha = 0.0f;
            *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f;
            m_v2iDawnDusk.y = 0;
            m_bDayNightTransitioning = 0;
            vSunAmb.x = m_vMoonAmbientRG.x;
            vSunAmb.y = m_vMoonAmbientRG.y;
            vSunAmb.z = m_fMoonAmbientBlue;
            vSunDiff.x = m_vMoonDiffuseRG.x;
            vSunDiff.y = m_vMoonDiffuseRG.y;
            vSunDiff.z = m_fMoonDiffuseBlue;
            vFogColor = m_vNightFogColor;
            vFog.x = m_fogNear;
            vFog.y = m_fogFar;
            if (m_pSunGob != 0) {
                CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
                if (pPlayer != 0)
                    pPlayer->GetCreatureLight(1);
            }
            m_isNight = 1;
        } else {
            // dusk in progress: day -> night
            float fRatio = fTime / fMax;
            projectionalpha = 0.05f - fRatio * 0.05f;
            vSunAmb.x = m_vSunAmbientRG.x + fRatio * (m_vMoonAmbientRG.x - m_vSunAmbientRG.x);
            vSunAmb.y = m_vSunAmbientRG.y + fRatio * (m_vMoonAmbientRG.y - m_vSunAmbientRG.y);
            vFogColor.x = m_vDayFogColor.x + fRatio * (m_vNightFogColor.x - m_vDayFogColor.x);
            vFogColor.y = m_vDayFogColor.y + fRatio * (m_vNightFogColor.y - m_vDayFogColor.y);
            vFogColor.z = m_vDayFogColor.z + fRatio * (m_vNightFogColor.z - m_vDayFogColor.z);
            vSunAmb.z = m_fSunAmbientBlue + fRatio * (m_fMoonAmbientBlue - m_fSunAmbientBlue);
            vSunDiff.x = m_vSunDiffuseRG.x + fRatio * (m_vMoonDiffuseRG.x - m_vSunDiffuseRG.x);
            vSunDiff.y = m_vSunDiffuseRG.y + fRatio * (m_vMoonDiffuseRG.y - m_vSunDiffuseRG.y);
            vSunDiff.z = m_fSunDiffuseBlue + fRatio * (m_fMoonDiffuseBlue - m_fSunDiffuseBlue);
            vFog.x = m_sunFogNear + fRatio * (m_moonFogNear - m_sunFogNear);
            vFog.y = m_sunFogFar + fRatio * (m_moonFogFar - m_sunFogFar);
        }
    } else {
        vSunDiff = Vector{0.0f, 0.0f, 0.0f};
        vSunAmb = Vector{0.0f, 0.0f, 0.0f};
        vFog = Vector{0.0f, 0.0f, 0.0f};
    }
    float fHalf = fMax * 0.5f;
    if (fTime >= fHalf && m_bDayNightSwitchPending != 0) {
        if (m_v2iDawnDusk.y != 0)
            m_isNight = 1;
        else if (m_v2iDawnDusk.x != 0)
            m_isNight = 0;
        if (m_pSunGob != 0) {
            CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pPlayer != 0)
                pPlayer->GetCreatureLight(1);
        }
        m_bDayNightSwitchPending = 0;
    }
    if (fTime < fMax) {
        if (m_bDayNightSwitchPending != 0)
            *reinterpret_cast<float*>(&shadowalpha) = static_cast<float>(m_shadowOpacity) / 100.0f -
                (fTime / fHalf) * (static_cast<float>(m_shadowOpacity) / 100.0f);
        else
            *reinterpret_cast<float*>(&shadowalpha) = (fTime / fHalf + -1.0f) * (static_cast<float>(m_shadowOpacity) / 100.0f) + 0.0f;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "sun gidy_sun_amb color %f %f %f", vSunAmb.x, vSunAmb.y, vSunAmb.z);
    AurControlPart(buf);
    snprintf(buf, sizeof(buf), "sun gidy_sun_diff color %f %f %f", vSunDiff.x, vSunDiff.y, vSunDiff.z);
    AurControlPart(buf);
    m_fogNear = vFog.x;
    m_fogFar = vFog.y;
    m_pScene->SetFogRange(vFog.x, vFog.y);
    m_pScene->SetFogColor(vFogColor);
    snprintf(buf, sizeof(buf), "%f %f %f", vFogColor.x, vFogColor.y, vFogColor.z);
    SetClearColor(buf);
    UpdateSoundObjectsTimeOfDay();
    m_pAmbientSound->UpdateTimeOfDay();
}

struct CSWCAreaVec3 {
    float x, y, z;
    CSWCAreaVec3(float a, float b, float c) : x(a), y(b), z(c) {}
};

// Runs the client area's per-frame tick. It repositions the weather emitters around the camera,
// drives the lightning state machine, ticks the wind, day/night and weather-transition timers,
// animates the scene and drains the emitter list awaiting deletion.
void CSWCArea::Update(float deltaTime)
{
    CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    unsigned int nDay, nTime;
    pTimer->GetWorldTime(&nDay, &nTime);
    pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    unsigned int dDay, dTime;
    pTimer->SubtractWorldTimes(nDay, nTime, m_nLastUpdateDay, m_nLastUpdateTime, &dDay, &dTime);
    m_nLastUpdateDay = nDay;
    m_nLastUpdateTime = nTime;
    if (m_pScene == 0)
        return;
    float fDelta = static_cast<float>(dTime) / 1000.0f;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
#if !KOTOR1
    g_pWeatherCamera = pCamera;
#endif
#if KOTOR1
    // K1 places the six weather emitters on a fixed 3x2 grid in the camera's ground-plane frame, 10
    // apart, anchored at the player's position 15 above the ground; every emitter is lowered 5 except
    // the player's own (index 4). There is no per-room weather disable, and a strike plays "flash0N"
    // with no thunder.
    if (pPlayer != 0 && pCamera != 0) {
        Vector vFwd;
        Vector vRight;
        float fPX = pPlayer->m_position.x;
        float fPY = pPlayer->m_position.y;
        float fPZ = pPlayer->m_position.z + 15.0f;
        vFwd = pCamera->GetOrientation().zdir();
        vFwd.z = 0.0f;
        vFwd = normalize(vFwd);
        vRight = cross(vFwd, Vector(0.0f, 0.0f, 1.0f));
        Vector vUnit = normalize(vRight);
        vRight.x = vUnit.x * 10.0f;
        vRight.y = vUnit.y * 10.0f;
        vRight.z = 0.0f;
        float fBX = fPX - vRight.x;
        float fBY = fPY - vRight.y;
        float fAX = vFwd.x * 10.0f;
        float fAY = vFwd.y * 10.0f;
        float fAZ = 0.0f;
        // Slot (i, j) sits at base + right*i + ahead*j, lowered 5 except the player's own slot.
#define K1_WEATHER_GOB(n, ri, aj, dz)                                                         \
        if (m_apWeatherGobs[n] != 0) {                                                       \
            m_apWeatherGobs[n]->SetOrientation(YawPitchRoll(Yaw(vFwd), 0.0f, 0.0f));         \
            Vector v(fBX + vRight.x * (ri) + fAX * (aj), fBY + vRight.y * (ri) + fAY * (aj), \
                     fPZ + vRight.z * (ri) + fAZ * (aj) dz);                                 \
            m_apWeatherGobs[n]->SetPosition(v);                                              \
        }
        K1_WEATHER_GOB(0, 0.0f, 0.0f, - 5.0f)
        K1_WEATHER_GOB(1, 1.0f, 0.0f, )
        K1_WEATHER_GOB(2, 2.0f, 0.0f, - 5.0f)
        K1_WEATHER_GOB(3, 0.0f, 1.0f, - 5.0f)
        K1_WEATHER_GOB(4, 1.0f, 1.0f, - 5.0f)
        K1_WEATHER_GOB(5, 2.0f, 1.0f, - 5.0f)
#undef K1_WEATHER_GOB
        if (m_pLightningGob != 0) {
            bool bDo;
            if (m_fLightningFlashTime == 0.0f) {
                bDo = true;
            } else if (m_fLightningFlashTime <= fDelta) {
                m_fLightningFlashTime = 0.0f;
                SetLightningDone();
                bDo = true;
            } else {
                m_fLightningFlashTime = m_fLightningFlashTime - fDelta;
                bDo = false;
            }
            bool bStrike = bDo;
            if (m_fLightningDelay != 0.0f) {
                if (m_fLightningDelay <= fDelta) {
                    m_fLightningDelay = 0.0f;
                } else {
                    m_fLightningDelay = m_fLightningDelay - fDelta;
                    bStrike = false;
                }
            }
            if (bStrike) {
                m_fLightningDelay = static_cast<float>(rand_wincompatible() % 5000) / 1000.0f + 5.0f;
                if (rand_wincompatible() % 100 < m_chanceLightning) {
                    if ((rand_wincompatible() & 1) != 0) {
                        float fX = pPlayer->m_position.x;
                        float fY = pPlayer->m_position.y;
                        float fZ = pPlayer->m_position.z + 10.0f;
                        int nSignX = rand_wincompatible();
                        int nRandX = rand_wincompatible();
                        float fDX = static_cast<float>(nRandX % 100) / 10.0f;
                        if ((nSignX & 1) == 0)
                            fDX = -fDX;
                        fX = fX + fDX;
                        int nSignY = rand_wincompatible();
                        int nRandY = rand_wincompatible();
                        float fDY = static_cast<float>(nRandY % 100) / 10.0f;
                        if ((nSignY & 1) == 0)
                            fDY = -fDY;
                        fY = fY + fDY;
                        m_pLightningGob->AttachToScene(m_pScene);
                        Vector vStrike;
                        vStrike.x = fX;
                        vStrike.y = fY;
                        vStrike.z = fZ;
                        m_pLightningGob->SetPosition(vStrike);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "flash0%i", rand_wincompatible() % 3 + 1);
                        float fParam, fDuration;
                        m_pLightningGob->GetAnimation(buf, &fParam, &fDuration);
                        m_fLightningFlashTime = fDuration;
                        m_pLightningGob->PlayAnimation(buf, 1.0f, 3, 0.0f);
                        s_shadowAlphaSaved = shadowalpha;
                        *reinterpret_cast<float*>(&shadowalpha) = *reinterpret_cast<float*>(&shadowalpha) * 0.4f;
                        g_pAppManager->m_pClientExoApp->GetInGameGui();
                    }
                }
            }
        }
    }
#else
    if (pPlayer != 0 && pCamera != 0) {
        Vector vDir;
        Vector vPos = pPlayer->m_position;
        char cDisable = 0;
#if !KOTOR1
        if (GetServerArea() != 0) {
            bool bHasWeather = m_apWeatherGobs[0] != 0 || m_apWeatherGobs[1] != 0 ||
                               m_apWeatherGobs[2] != 0 || m_apWeatherGobs[3] != 0 ||
                               m_apWeatherGobs[4] != 0 || m_apWeatherGobs[5] != 0;
            cDisable = m_pServerArea->GetRoomDisableWeather(vPos);
            if (cDisable != 0) {
                if (bHasWeather)
                    SetWeather(0, 0.0f);
            } else if (!bHasWeather) {
                SetWeather(m_weatherType, m_vWeatherPercentWindTimer.x);
            }
        }
#endif
        Vector vCamPos = pCamera->GetPosition();
        // The camera's forward axis (rotated +Z), projected onto the ground plane and normalized:
        // the emitters' placement frame.
        vDir = pCamera->GetOrientation().zdir();
        vDir.z = 0.0f;
        vDir = normalize(vDir);
        // A hexagon of radius 4.5 around the camera (rows at 90/150/210/270/330/30 degrees, in
        // the (right, forward) frame) with the four diagonal emitters raised 5.0.
        static const CSWCAreaVec3 s_weatherOffsets[6] = {
            CSWCAreaVec3(0.0f, 4.5f, 0.0f),
            CSWCAreaVec3(-3.8971145f, 2.2499998f, 5.0f),
            CSWCAreaVec3(-3.8971145f, -2.2499998f, 5.0f),
            CSWCAreaVec3(0.0f, -4.5f, 0.0f),
            CSWCAreaVec3(3.8971145f, -2.2499998f, 5.0f),
            CSWCAreaVec3(3.8971145f, 2.2499998f, 5.0f),
        };
        // Right vector = vDir x +Z in the ground plane.
        float fZero = vDir.z * 0.0f;
        float fRightX = vDir.y - fZero;
        float fRightY = fZero - vDir.x;
        float fBaseZ = vPos.z + 8.0f;
        for (int i = 0; i < 6; i++) {
            if (m_apWeatherGobs[i] != 0) {
                m_apWeatherGobs[i]->SetOrientation(YawPitchRoll(Yaw(vDir), 0.0f, 0.0f));
                const CSWCAreaVec3& off = s_weatherOffsets[i];
                Vector vEmitterPos;
                vEmitterPos.x = vCamPos.x + fRightX * off.x + vDir.x * off.y + 0.0f;
                vEmitterPos.y = vCamPos.y + fRightY * off.x + vDir.y * off.y + 0.0f;
                vEmitterPos.z = off.z + (fBaseZ + off.x * 0.0f + off.y * 0.0f);
                m_apWeatherGobs[i]->SetPosition(vEmitterPos);
            }
        }
        if (cDisable != 0)
            goto weatherDone;
        if (m_pLightningGob == 0)
            goto weatherDone;
        {
            bool bDo;
            if (m_fLightningFlashTime == 0.0f) {
                bDo = true;
            } else if (m_fLightningFlashTime <= fDelta) {
                m_fLightningFlashTime = 0.0f;
                SetLightningDone();
                bDo = true;
            } else {
                m_fLightningFlashTime = m_fLightningFlashTime - fDelta;
                bDo = false;
            }
            bool bStrike = bDo;
            if (m_fLightningDelay != 0.0f) {
                if (m_fLightningDelay <= fDelta) {
                    m_fLightningDelay = 0.0f;
                } else {
                    m_fLightningDelay = m_fLightningDelay - fDelta;
                    bStrike = false;
                }
            }
            if (bStrike) {
                m_fLightningDelay = static_cast<float>(rand_wincompatible() % 5000) / 1000.0f + 5.0f;
                if (rand_wincompatible() % 100 < m_chanceLightning) {
                    if ((rand_wincompatible() & 1) != 0) {
                        float fX = pPlayer->m_position.x;
                        float fY = pPlayer->m_position.y;
                        float fZ = pPlayer->m_position.z + 10.0f;
                        int nSignX = rand_wincompatible();
                        int nRandX = rand_wincompatible();
                        float fDX = static_cast<float>(nRandX % 100) / 10.0f;
                        if ((nSignX & 1) == 0)
                            fDX = -fDX;
                        fX = fX + fDX;
                        int nSignY = rand_wincompatible();
                        int nRandY = rand_wincompatible();
                        float fDY = static_cast<float>(nRandY % 100) / 10.0f;
                        if ((nSignY & 1) == 0)
                            fDY = -fDY;
                        fY = fY + fDY;
                        m_pLightningGob->AttachToScene(m_pScene);
                        Vector vStrike;
                        vStrike.x = fX;
                        vStrike.y = fY;
                        vStrike.z = fZ;
                        m_pLightningGob->SetPosition(vStrike);
                        char buf[16];
                        snprintf(buf, sizeof(buf), "flash0%i", rand_wincompatible() % 3 + 1);
                        float fParam, fDuration;
                        m_pLightningGob->GetAnimation(buf, &fParam, &fDuration);
                        m_fLightningFlashTime = fDuration;
                        m_pLightningGob->PlayAnimation(buf, 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimNoBlendIn | AnimRun::kAnimNoReuse, 0.0f);
                        s_shadowAlphaSaved = shadowalpha;
                        *reinterpret_cast<float*>(&shadowalpha) = *reinterpret_cast<float*>(&shadowalpha) * 0.4f;
                        m_pAmbientSound->Thunder(0);
                        g_pAppManager->m_pClientExoApp->GetInGameGui();
                    } else {
                        m_pAmbientSound->Thunder(1);
                    }
                }
            }
        }
    }
weatherDone:;
#endif
    if (m_windPower == 1 || m_windPower == 2) {
        float fWind;
        if (m_vWeatherPercentWindTimer.y <= fDelta) {
            // Deliberately re-tests the condition the enclosing branch just established.
            switch (m_windPower) {
            case 1:
            case 2:
                rand_wincompatible();
            }
            fWind = static_cast<float>(rand_wincompatible() % 5000) / 1000.0f + 5.0f;
        } else {
            fWind = m_vWeatherPercentWindTimer.y - fDelta;
        }
        m_vWeatherPercentWindTimer.y = fWind;
    }
    if (m_dayNightCycle != 0)
        UpdateDayNightCycle(fDelta);
    if (m_isNight != 0 && m_bShadowLightsChanged != 0)
        m_bShadowLightsChanged = 0;
    if (m_weatherTransitionTime != 0) {
        if (m_weatherTransitionTime > dTime) {
            m_vWeatherPercentWindTimer.x = (static_cast<float>(6000 - m_weatherTransitionTime) / 6000.0f) * 100.0f;
            switch (m_weatherType) {
            case WEATHER_RAIN:
                SetWeather(WEATHER_RAIN, m_vWeatherPercentWindTimer.x);
                break;
            case WEATHER_SNOW:
                SetWeather(WEATHER_SNOW, m_vWeatherPercentWindTimer.x);
                break;
            case WEATHER_CLEAR:
                m_vWeatherPercentWindTimer.x = (static_cast<float>(m_weatherTransitionTime) / 6000.0f) * 100.0f;
                SetWeather(m_nCurrentWeather, m_vWeatherPercentWindTimer.x);
                break;
            }
            m_weatherTransitionTime = m_weatherTransitionTime - dTime;
        } else {
            m_weatherTransitionTime = 0;
            SetWeather(m_weatherType, 100.0f);
        }
    }
    m_pAmbientSound->Update();
    CWorldTimer* pAnimTimer = g_pAppManager->m_pClientExoApp->GetAnimationTimer();
    pAnimTimer->SetSnapshotTime();
    CAurScene* pScene = m_pScene;
    pAnimTimer = g_pAppManager->m_pClientExoApp->GetAnimationTimer();
    long long nDiff = pAnimTimer->GetSnapshotTimeDifference();
    pScene->Animate(static_cast<float>(nDiff) / 1000000.0f);
    if (m_pMiniGame != 0)
        m_pMiniGame->Update(deltaTime);
#if !KOTOR1
    for (int i = m_toBeDeletedEmitters.m_count; i > 0; i--) {
        Gob* pEmitter = static_cast<Gob*>(m_toBeDeletedEmitters.m_pData[i - 1]);
        if (pEmitter != 0) {
            // The live-particle count belongs to PartEmitter, not to Part, so the part is
            // downcast.
            PartEmitter* pOmen = static_cast<PartEmitter*>(pEmitter->GetPart(const_cast<char*>("OmenEmitter01")));
            if (pOmen == 0) {
                pEmitter->AttachToScene(0);
                delete pEmitter;
                m_toBeDeletedEmitters.DelIndex(i - 1);
            } else if (pOmen->m_lParticles.m_count == 0) {
                pEmitter->AttachToScene(0);
                delete pEmitter;
                m_toBeDeletedEmitters.DelIndex(i - 1);
            }
        }
    }
#endif
}

unsigned int CSWCArea::GetSurfaceMaterial(Vector pos)
{
    if (g_pAppManager->m_pServerExoApp == 0)
        return 0;
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule == 0)
        return 0;
    CSWSArea* pArea = pModule->GetArea();
    if (pArea == 0)
        return 0;
    return pArea->GetSurfaceMaterial(pos);
}

void CSWCArea::UnhighlightAreaTransitions()
{
    int i = m_triggerIdList.m_count;
    while (i > 0) {
        i--;
        CSWCTrigger* pTrig = g_pAppManager->m_pClientExoApp->GetTriggerByGameObjectID(
            m_triggerIdList.m_pArray[i]);
        if (pTrig != 0 && pTrig->m_bIsTrap == 0 && pTrig->m_displayState != 0) {
            if (g_pAppManager->m_pClientExoApp->GetDebugMode() == 0)
                pTrig->DisplayTrigger(0);
        } else {
            CSWCDoor* pDoor = g_pAppManager->m_pClientExoApp->GetDoorByGameObjectID(
                m_triggerIdList.m_pArray[i]);
            if (pDoor != 0) {
                pDoor->GetAnimationBase()->SetOverlayAnimation(SERVER_ANIMATION_DOOR_TRANSITION, 0);
                if (pDoor->GetState() == CSWCDoor::DOOR_STATE_CLOSED)
                    pDoor->SetAnimation(SERVER_ANIMATION_CLOSED, 0, 0);
                else if (pDoor->GetState() == CSWCDoor::DOOR_STATE_OPEN1)
                    pDoor->SetAnimation(SERVER_ANIMATION_DOOR_OPEN1, 0, 0);
                else if (pDoor->GetState() == CSWCDoor::DOOR_STATE_OPEN2)
                    pDoor->SetAnimation(SERVER_ANIMATION_DOOR_OPEN2, 0, 0);
                else if (pDoor->GetState() == CSWCDoor::DOOR_STATE_DESTROYED)
                    pDoor->SetAnimation(SERVER_ANIMATION_DESTROYED, 0, 0);
                m_triggerIdList.Remove(pDoor->m_objectId);
            }
        }
    }
}

void CSWCArea::HiliteAreaObjects(int bHilite)
{
    for (int i = 0; i < m_objectIdList.m_count; i++) {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_objectIdList.m_pArray[i]);
        if (pObj == 0)
            continue;
        CSWCObject* pCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
        if (pCObj == 0)
            continue;
        if (pCObj->m_fadeState == 0)
            continue;
        switch (pCObj->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE: {
            CSWCCreature* pCreature = pCObj->AsSWCCreature();
            if (bHilite == 0) {
                pCreature->SetPopUpBubbleDecayTime(0.0f);
                pCObj->UnHilite(2);
                break;
            }
            Vector color;
            if (pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature())
                color = Vector{1.0f, 1.0f, 1.0f};
            else if (pCreature->m_nReputation == 0)
                color = Vector{0.4f, 0.4f, 1.0f};
            else if (pCreature->m_nReputation == 1)
                color = Vector{0.3f, 1.0f, 0.3f};
            else
                color = Vector{1.0f, 0.1f, 0.1f};
            pCObj->Hilite(color, 2, 0.0f, 1);
            break;
        }
        case GAME_OBJECT_TYPE_ITEM:
        case GAME_OBJECT_TYPE_PLACEABLE:
        case GAME_OBJECT_TYPE_DOOR:
            if (bHilite == 0)
                pCObj->UnHilite(2);
            else
                pCObj->Hilite(Vector{0.3f, 1.0f, 1.0f}, 2, 0.0f, 1);
            break;
        }
    }
}

int CSWCArea::ApplyFireAndForgetVisualEffect(unsigned short nEffect, Vector pos)
{
    CSWCVisualEffect* pEffect = new CSWCVisualEffect(1);
    if (pEffect->LoadEffect(nEffect) == 0) {
        delete pEffect;
        return 0;
    }
    pEffect->AttachToScene(m_pScene);
    pEffect->m_bImpactOnly = 1;
    pEffect->m_bDeleteWhenFinished = 1;
    pEffect->SetPosition(pos);
    pEffect->SetAnimation(1);
    pEffect->DoVFXRumble();
    return 1;
}

int CSWCArea::SetGlobalLightAmbient(int a, Vector color)
{
    K2_UNUSED(a);
    if (m_pScene == 0)
        return 0;
    char buf[128];
    snprintf(buf, sizeof(buf), "sun gidy_sun_amb color %f %f %f", color.x, color.y, color.z);
    AurControlPart(buf);
    return 1;
}

void CSWCArea::AddMapPin(CSWCMapPin* pPin)
{
    m_mapPins.Add(pPin);
}

void CSWCArea::RemoveMapPin(unsigned int nId)
{
    int nCount = m_mapPins.m_count;
    if (nCount < 1)
        return;
    for (int i = 0; i < nCount; i++) {
        CSWCMapPin* pPin = m_mapPins.m_pArray[i];
        if (pPin->m_nPinId != nId)
            continue;
        if (pPin->m_nPinType != 1)
            continue;
        m_mapPins.DelIndex(i);
        delete pPin;
    }
}

// Adds a corpse id to a ring of 4. On wraparound (insert caught up with oldest) the overwritten
// corpse is faded out (and its body-bag placeable flagged visible) before the slot is recycled.
void CSWCArea::AddCorpse(unsigned long long objectId)
{
    unsigned char nOld = m_nNextCorpse;
    unsigned char nInsert = nOld + 1;
    m_nNextCorpse = nInsert;
    m_aCorpseIds[nOld] = objectId;
    if (nInsert > 3) {
        m_nNextCorpse = 0;
        nInsert = 0;
    }
    if (m_nOldestCorpse == nInsert) {
        CSWCCreature* pCorpse = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            m_aCorpseIds[nInsert]);
        if (pCorpse != 0) {
            CSWCPlaceable* pPlaceable = g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(
                pCorpse->m_oidBodyBag);
            if (pPlaceable != 0) {
                CSWSPlaceable* pServerPlaceable = pPlaceable->GetServerPlaceable();
                if (pServerPlaceable != 0)
                    pServerPlaceable->m_bIsBodyBagVisible = 1;
                static_cast<CSWCObject*>(pPlaceable)->SetDesiredFadeState(1, 0, 0, 0);
            }
            static_cast<CSWCObject*>(pCorpse)->SetFadeState(1);
            static_cast<CSWCObject*>(pCorpse)->SetDesiredFadeState(0, 1, 45000, 1000);
            static_cast<CSWCObject*>(pCorpse)->m_bPendingRemoval = 1;
        }
        m_aCorpseIds[m_nOldestCorpse] = OBJECT_INVALID;
        unsigned char nOldest = m_nOldestCorpse + 1;
        if (nOldest > 3)
            nOldest = 0;
        m_nOldestCorpse = nOldest;
    }
}

int CSWCArea::ParentCorpseExists(unsigned long long objectId)
{
    for (int i = 0; i < 3; i++) {
        CSWCCreature* pCorpse = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            m_aCorpseIds[i]);
        if (pCorpse != 0 && pCorpse->m_oidBodyBag == objectId)
            return 1;
    }
    CSWCCreature* pCorpse = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
        m_aCorpseIds[3]);
    if (pCorpse != 0 && pCorpse->m_oidBodyBag == objectId)
        return 1;
    return 0;
}

#if !KOTOR1
void CSWCArea::AddToBeDeletedEmitter(CAurObject* pEmitter)
{
    if (pEmitter == 0)
        return;
    m_toBeDeletedEmitters.Add(pEmitter);
}
#endif

void CSWCArea::ClearAllMapPins()
{
    int nCount = m_mapPins.m_count;
    for (int i = 0; i < nCount; i++) {
        CSWCMapPin* pPin = m_mapPins.m_pArray[i];
        if (pPin != 0)
            delete pPin;
    }
    // inlined CExoArrayList<CSWCMapPin*>::Clear()
    if (m_mapPins.m_pArray != 0) {
        delete[] m_mapPins.m_pArray;
        m_mapPins.m_pArray = 0;
        m_mapPins.m_capacity = 0;
    }
    m_mapPins.m_count = 0;
}

void CSWCArea::ChangeTriggerDisplay(int b)
{
    for (int i = m_triggerIdList.m_count - 1; i >= 0; i--) {
        CSWCTrigger* pTrigger = g_pAppManager->m_pClientExoApp->GetTriggerByGameObjectID(
            m_triggerIdList.m_pArray[i]);
        if (pTrigger == 0)
            continue;
        if (pTrigger->m_bAreaTransition != 0)
            continue;
        if (pTrigger->m_bIsTrap != 0)
            continue;
        pTrigger->DisplayTrigger(b);
    }
}

int CSWCArea::SetGlobalLightDiffuse(int b, Vector color)
{
    if (m_pSunGob == 0)
        return 0;
    if (b != 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "sun gidy_sun_diff color %f %f %f", color.x, color.y, color.z);
        AurControlPart(buf);
    } else {
        m_pSunGob->AttachToScene(0);
    }
    return 1;
}

void CSWCArea::SetLightningDone()
{
    shadowalpha = s_shadowAlphaSaved;
    m_fLightningDelay = static_cast<float>(rand_wincompatible() % 5000) / 1000.0f + 5.0f;
    if (m_pLightningGob != 0)
        m_pLightningGob->AttachToScene(0);
}

int CSWCArea::LoadArea(CAurCamera* pCamera, CGuiAreaLoadscreen* pLoadscreen, int nFlags)
{
    K2_UNUSED(pLoadscreen); K2_UNUSED(nFlags);
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg == 0)
        return 0;

    m_objectId = pMsg->ReadOBJECTIDClient();
    CResRef resArea;
    resArea = pMsg->ReadCResRef(16);
    m_areaResRef = resArea;
    m_areaName = pMsg->ReadCExoLocStringClient();
    m_oidLastWeaponUsed = pMsg->ReadINT(32);
    m_nEntryY = pMsg->ReadINT(32);
    m_nEntryZ = pMsg->ReadINT(32);
    m_nAreaFlags = pMsg->ReadDWORD(32);
    m_cameraStyle = pMsg->ReadINT(32);
    m_defaultEnvMap = pMsg->ReadCResRef(16);

    unsigned char bHasMiniGame = pMsg->ReadBYTE(8);
    if (bHasMiniGame == 0) {
        AurEnableInterleavedAnimations();
    } else {
        m_pMiniGame = new CSWMiniGame(this);
        AurDisableInterleavedAnimations();
    }

    m_moonAmbientColor = pMsg->ReadDWORD(32);
    m_moonDiffuseColor = pMsg->ReadDWORD(32);
    m_moonFogOn = pMsg->ReadBYTE(8);
    m_moonFogColor = pMsg->ReadDWORD(32);
    m_moonFogNear = pMsg->ReadFLOAT(1.0f, 32);
    m_moonFogFar = pMsg->ReadFLOAT(1.0f, 32);
    m_moonShadows = pMsg->ReadBOOL();
    m_sunAmbientColor = pMsg->ReadDWORD(32);
    m_sunDiffuseColor = pMsg->ReadDWORD(32);
    m_sunFogOn = pMsg->ReadBYTE(8);
    m_nFogColor = pMsg->ReadDWORD(32);
    m_sunFogNear = pMsg->ReadFLOAT(1.0f, 32);
    m_sunFogFar = pMsg->ReadFLOAT(1.0f, 32);
    m_sunShadows = pMsg->ReadBOOL();
    m_dayNightCycle = pMsg->ReadBYTE(8) & 0xff;
    m_isNight = pMsg->ReadBYTE(8) & 0xff;
    m_dynAmbientColor = pMsg->ReadDWORD(32);
    m_noRest = pMsg->ReadBYTE(8) & 0xff;
    m_shadowOpacity = pMsg->ReadBYTE(8);
    m_pAmbientSound->LoadFromMessage(pMsg);

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    unsigned char nModState = pMsg->ReadBYTE(8);
    unsigned int nModTimeMs = pMsg->ReadDWORD(32);
    int nModCalendarHour = pMsg->ReadBYTE(8) & 0xff;
    int nModCalendarDay = pMsg->ReadBYTE(8) & 0xff;
    int nModCalendarMonth = pMsg->ReadBYTE(8) & 0xff;
    unsigned int nModCalendarYear = pMsg->ReadDWORD(32);
    unsigned char bModEc = pMsg->ReadBYTE(8);
    unsigned char bModEd = pMsg->ReadBYTE(8);

    m_grassTexName = pMsg->ReadCResRef(16);
    m_grassDensity = pMsg->ReadFLOAT(1.0f, 32);
    m_grassQuadSize = pMsg->ReadFLOAT(1.0f, 32);
    m_grassDiffuse = pMsg->ReadDWORD(32);
    m_grassAmbient = pMsg->ReadDWORD(32);
#if !KOTOR1
    m_grassEmissive = pMsg->ReadDWORD(32);
#endif
    m_grassProbLL = pMsg->ReadFLOAT(1.0f, 32);
    m_grassProbLR = pMsg->ReadFLOAT(1.0f, 32);
    m_grassProbUL = pMsg->ReadFLOAT(1.0f, 32);
    m_grassProbUR = pMsg->ReadFLOAT(1.0f, 32);
    CAurGrassInfo grassInfo;
    m_grassTexName.CopyToString(grassInfo.m_sName);
    grassInfo.m_fDensity = m_grassDensity;
    grassInfo.m_fQuadSize = m_grassQuadSize;
    grassInfo.m_vDiffuse.x = static_cast<float>(m_grassDiffuse & 0xff) / 255.0f;
    grassInfo.m_vDiffuse.y = static_cast<float>((m_grassDiffuse >> 8) & 0xff) / 255.0f;
    grassInfo.m_vDiffuse.z = static_cast<float>((m_grassDiffuse >> 16) & 0xff) / 255.0f;
    grassInfo.m_vAmbient.x = static_cast<float>(m_grassAmbient & 0xff) / 255.0f;
    grassInfo.m_vAmbient.y = static_cast<float>((m_grassAmbient >> 8) & 0xff) / 255.0f;
    grassInfo.m_vAmbient.z = static_cast<float>((m_grassAmbient >> 16) & 0xff) / 255.0f;
#if !KOTOR1
    grassInfo.m_vEmissive.x = static_cast<float>(m_grassEmissive & 0xff) / 255.0f;
    grassInfo.m_vEmissive.y = static_cast<float>((m_grassEmissive >> 8) & 0xff) / 255.0f;
    grassInfo.m_vEmissive.z = static_cast<float>((m_grassEmissive >> 16) & 0xff) / 255.0f;
#endif
    grassInfo.m_fProbLL = m_grassProbLL;
    grassInfo.m_fProbLR = m_grassProbLR;
    grassInfo.m_fProbUL = m_grassProbUL;
    grassInfo.m_fProbUR = m_grassProbUR;
    int nGrass;
    for (int row = 0; row < SURFACE_MATERIAL_ROWS; row++) {
        g_pRules->m_p2DArrays->m_pSurfaceMaterial->GetINTEntry(row, CExoString(const_cast<char*>("Grass")), &nGrass);
        if (nGrass != 0)
            AurGrassAddInfo(row, grassInfo);
    }

    m_chanceLightning = pMsg->ReadBYTE(8);
    m_windPower = pMsg->ReadBYTE(8);
    unsigned char nWeatherTypeByte = pMsg->ReadBYTE(8);
    int bHasWeather = pMsg->ReadBOOL();
#if !KOTOR1
    unsigned char nRainByte = pMsg->ReadBYTE(8);
#endif
    m_alphaTest = pMsg->ReadFLOAT(1.0f, 32);

    // The per-area environmental audio effects are name/type/value triples, shared and applied
    // identically to every room via SetRoomEnvAudioProps below.
    int nEnvAudio = pMsg->ReadINT(32);
    CExoString* aEnvNames = 0;
    int* aEnvTypes = 0;
    float* aEnvValues = 0;
    if (nEnvAudio != 0) {
        aEnvNames = new CExoString[nEnvAudio];
        aEnvTypes = new int[nEnvAudio];
        aEnvValues = new float[nEnvAudio];
        for (int i = 0; i < nEnvAudio; i++) {
            aEnvNames[i] = pMsg->ReadCExoString(32);
            aEnvTypes[i] = pMsg->ReadINT(32);
            aEnvValues[i] = pMsg->ReadFLOAT(1.0f, 32);
        }
    }

    // The per-room ambient sound attaches are room name / part name / event name / sound resref
    // quadruples, matched on each room's own resref in the spawn loop below.
    int nRoomSounds = pMsg->ReadINT(32);
    CExoString* aRoomNames = new CExoString[nRoomSounds];
    CExoString* aPartNames = new CExoString[nRoomSounds];
    CExoString* aEventNames = new CExoString[nRoomSounds];
    CResRef* aSoundResRefs = new CResRef[nRoomSounds];
    for (int i = 0; i < nRoomSounds; i++) {
        aRoomNames[i] = pMsg->ReadCExoString(32);
        pMsg->ReadBOOL();  // discarded
        aPartNames[i] = pMsg->ReadCExoString(32);
        aEventNames[i] = pMsg->ReadCExoString(32);
        aSoundResRefs[i] = pMsg->ReadCResRef(16);
    }

    pMsg->ReadDWORD(32);  // discarded
    m_nWidth = pMsg->ReadINT(32);
    m_nHeight = pMsg->ReadINT(32);
    if (pMsg->MessageReadOverflow(1) != 0) {
        return 0;
    }
    m_weatherTransitionTime = 6000;
    m_weatherType = bHasWeather ? nWeatherTypeByte : WEATHER_CLEAR;
#if !KOTOR1
    m_chanceRain = nRainByte;
#endif

    CAurScene* pScene = NewCAurScene(const_cast<char*>("mainscene"));
    m_pScene = pScene;
    if (pScene == 0)
        return 0;
    pScene->InitBSP(m_nWidth, m_nHeight, 5, true, 10.0f);

    CLYT* pLayout = new CLYT();
    pLayout->LoadLayout(m_areaResRef);
    int nRooms = pLayout->GetRoomCount();
    m_nRooms = nRooms;
    if (nRooms != 0)
        m_paRooms = new CSWCRoom[nRooms];

    AurTextureMakeCurrent();
    RyanTotalLevelVertices = 0;
    RyanTotalLevelVertSize = 0;
    RyanStartTextureSize = usedtexturememory;
    RyanStartParts = countpart;

    if (m_nRooms > 0) {
        int nWalkableIdx = 0;
        for (int i = 0; i < m_nRooms; i++) {
            if ((i % 5) == 0 || i == m_nRooms - 1) {
                CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
                unsigned char w1 = pApp->GetLoadStep(2);
                float fFrac = static_cast<float>(i + 1) / static_cast<float>(m_nRooms);
                unsigned char w2 = pApp->GetLoadStep(3);
                unsigned char w3 = pApp->GetLoadStep(1);
                unsigned char w4 = pApp->GetLoadStep(0);
                pApp->SetLoadBarProgress(
                    static_cast<int>(fFrac * static_cast<float>(w1) + static_cast<float>(w2) + static_cast<float>(w3) + static_cast<float>(w4)), 1);
                UpdateScreen(0.033333335f, 0, 0);
            }

            m_paRooms[i].SetRoomInfo(pLayout->GetRoomName(i), pLayout->GetRoomPosition(i));
            m_paRooms[i].SetRoomEnvAudioProps(nEnvAudio, aEnvNames, aEnvTypes, aEnvValues);
            if (m_paRooms[i].m_bRoomNameIsWildcard == 0)
                m_paRooms[i].m_nSceneRoomIndex = nWalkableIdx++;
            else
                m_paRooms[i].m_nSceneRoomIndex = -1;

            Gob* pRoomModel = m_paRooms[i].SpawnRoom(m_pScene);
            if (pRoomModel != 0) {
                pRoomModel->PlayAnimation(const_cast<char*>("animloop1"), 1.0f, 8, 0.0f);
                pRoomModel->PlayAnimation(const_cast<char*>("animloop2"), 1.0f, 8, 0.0f);
                pRoomModel->PlayAnimation(const_cast<char*>("animloop3"), 1.0f, 8, 0.0f);
            }

            // Attach this room's ambient sounds where its resref matches a table entry.
            CExoString sCurRoomName(CResRef(m_paRooms[i].m_resRoom).GetResRefStr());
            if (nRoomSounds > 0) {
                for (int k = 0; k < nRoomSounds; k++) {
                    if (aRoomNames[k] == sCurRoomName && pRoomModel != 0) {
                        Gob* pDummy = static_cast<Gob*>(NewCAurObject(const_cast<char*>("MGB_null"), const_cast<char*>("sounddummy"), 0, 0));
                        Part* pPart = pRoomModel->GetPart(static_cast<char*>(aPartNames[k].CStr()));
                        if (pPart != 0) {
                            CSWBehaviorAttachSound* pBehavior = new CSWBehaviorAttachSound(
                                static_cast<CAurObject*>(pDummy), static_cast<CAurObject*>(pRoomModel), pPart,
                                CResRef(aSoundResRefs[k].GetResRefStr()));
                            pDummy->SetBehavior(pBehavior);
                            pDummy->AttachToScene(m_pScene);
                            pDummy->EnableAlwaysRender();
#if KOTOR1  // K1: the out-of-line IsEmpty
                            if (aEventNames[k].IsEmpty()) {
                                pBehavior->PlaySound();
                            } else {
                                pRoomModel->RegisterCallBack(PlayRoomSoundEvent,
                                    static_cast<char*>(aEventNames[k].CStr()), pBehavior, 9999.0f, false);
                            }
#else
                            if (aEventNames[k].m_sString == 0 || *aEventNames[k].m_sString == 0) {
                                pBehavior->PlaySoundA();
                            } else {
                                pRoomModel->RegisterCallBack(PlayRoomSoundEvent,
                                    static_cast<char*>(aEventNames[k].CStr()), pBehavior, 9999.0f, false);
                            }
#endif
                            m_behaviorAttachSounds.AddHead(pBehavior);
                        }
                    }
                }
            }
        }
    }

    delete[] aRoomNames;
    delete[] aPartNames;
    delete[] aEventNames;
    delete[] aSoundResRefs;

    AurTextureMakeCurrent();
    RyanLevelVertices = RyanTotalLevelVertices;
    RyanEndParts = countpart;
    RyanEndTextureSize = usedtexturememory;
    RyanLevelTextureSize = usedtexturememory - RyanStartTextureSize;
    RyanLevelTotalParts = countpart - RyanStartParts;
    RyanLevelVertSize = RyanTotalLevelVertSize;

    delete[] aEnvNames;
    delete[] aEnvTypes;
    delete[] aEnvValues;

    if (m_pMiniGame != 0) {
        int nTracks = pLayout->GetTrackCount();
        for (int i = 0; i < nTracks; i++) {
            char sTrackName[17];  // CopyToString writes 17 bytes (NUL at [16])
            pLayout->GetTrackName(i).CopyToString(sTrackName);
            m_pMiniGame->AddTrack(CResRef(sTrackName), pLayout->GetTrackPosition(i));
        }
        int nObstacles = pLayout->GetObstacleCount();
        for (int i = 0; i < nObstacles; i++) {
            char sObsName[17];
            pLayout->GetObstacleName(i).CopyToString(sObsName);
            m_pMiniGame->AddObstacle(CResRef(sObsName), pLayout->GetObstaclePosition(i));
        }
        if (m_pMiniGame->Load() == 0) {
            delete pLayout;
            delete[] m_paRooms;
            m_paRooms = 0;
            return 0;
        }
    }

    pLayout->UnloadLayout();
    delete pLayout;

    if (m_pScene->LoadVisibility(m_areaResRef.GetResRefStr()) == 0) {
        m_pScene->DisableVisibilityGraph();
    } else {
        m_pScene->SetCurrentRoom(0);
        m_pScene->EnableVisibilityGraph();
    }

    AurTextureSuspendUpdates();
    g_pAppManager->m_pClientExoApp->GetInGameGui();

    int nMapPins1 = pMsg->ReadINT(32);
    for (int i = 0; i < nMapPins1; i++) {
        CSWCMapPin* pPin = new CSWCMapPin();
        pPin->m_nPinId = pMsg->ReadOBJECTIDClient();
        Vector pos;
        pos.x = pMsg->ReadFLOAT(1.0f, 32);
        pos.y = pMsg->ReadFLOAT(1.0f, 32);
        pos.z = pMsg->ReadFLOAT(1.0f, 32);
        pPin->SetPosition(pos);
        pPin->m_bActive = pMsg->ReadBOOL();
        pPin->m_sName = pMsg->ReadCExoLocStringClient();
        pPin->m_nPinType = 0;
        m_mapPins.Add(pPin);
    }
    int nMapPins2 = pMsg->ReadINT(32);
    for (int i = 0; i < nMapPins2; i++) {
        CSWCMapPin* pPin = new CSWCMapPin();
        pPin->m_nPinId = static_cast<unsigned long long>(static_cast<long long>(pMsg->ReadINT(32)));
        pPin->m_sName = pMsg->ReadCExoString(32);
        Vector pos;
        pos.x = pMsg->ReadFLOAT(1.0f, 32);
        pos.y = pMsg->ReadFLOAT(1.0f, 32);
        pos.z = pMsg->ReadFLOAT(1.0f, 32);
        pPin->SetPosition(pos);
        pPin->m_bActive = 1;
        pPin->m_nPinType = 1;
        m_mapPins.Add(pPin);
    }

    CGameObjectArray* pGameObjArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    unsigned short nSoundObjs = pMsg->ReadWORD(16);
    for (unsigned short i = 0; i < nSoundObjs; i++) {
        CSWCSoundObject* pSound = new CSWCSoundObject();
        pSound->LoadFromMessage(pMsg);
        pSound->AddToArea(this, pGameObjArray);
    }

    unsigned short nLights = pMsg->ReadWORD(16);
    for (unsigned short i = 0; i < nLights; i++) {
        unsigned long long objId = pMsg->ReadOBJECTIDClient();
#if KOTOR1  // K1: the placeable appearance id is a BYTE on the wire
        unsigned char nPlaceableId = pMsg->ReadBYTE(8);
#else
        int nPlaceableId = pMsg->ReadINT(32);
#endif
        float x = pMsg->ReadFLOAT(1.0f, 32);
        float y = pMsg->ReadFLOAT(1.0f, 32);
        float z = pMsg->ReadFLOAT(1.0f, 32);
        if (pMsg->MessageReadOverflow(1) != 0)
            break;
        Vector pos;
        pos.x = x; pos.y = y; pos.z = z;
        AddPlaceableObjectLight(objId, nPlaceableId, pos);
    }

    unsigned short nStatics = pMsg->ReadWORD(16);
    for (unsigned short i = 0; i < nStatics; i++) {
        unsigned long long objId = pMsg->ReadOBJECTIDClient();
#if KOTOR1
        unsigned char nPlaceableId = pMsg->ReadBYTE(8);
#else
        int nPlaceableId = pMsg->ReadINT(32);
#endif
        float px = pMsg->ReadFLOAT(1.0f, 32);
        float py = pMsg->ReadFLOAT(1.0f, 32);
        float pz = pMsg->ReadFLOAT(1.0f, 32);
        float fx = pMsg->ReadFLOAT(1.0f, 32);
        float fy = pMsg->ReadFLOAT(1.0f, 32);
        float fz = pMsg->ReadFLOAT(1.0f, 32);
        if (pMsg->MessageReadOverflow(1) != 0)
            break;
        Vector vPos, vFacing;
        vPos.x = px; vPos.y = py; vPos.z = pz;
        vFacing.x = fx; vFacing.y = fy; vFacing.z = fz;
        AddPlaceableObjectStatic(objId, nPlaceableId, vPos, vFacing);
    }

    AurTextureResumeUpdates();

    if (pMsg->MessageReadOverflow(1) == 0 && pMsg->MessageReadUnderflow(1) == 0) {
        Gob* pSun = static_cast<Gob*>(NewCAurObject(const_cast<char*>("gidy_sun"), const_cast<char*>("sun"), 0, 0));
        m_pSunGob = pSun;
        if (pSun != 0) {
            Vector sunPos;
            sunPos.x = 4000.0f; sunPos.y = 4500.0f; sunPos.z = 7000.0f;
            pSun->SetPosition(sunPos);
            m_pSunGob->AttachToScene(m_pScene);
            m_pScene->SetFog(m_sunFogOn);
            projectionalpha = 0.05f;

            // Night fog and ambient colors.
            m_vMoonAmbientRG.x = static_cast<float>(m_moonAmbientColor & 0xff) / 255.0f;
            m_vMoonAmbientRG.y = static_cast<float>((m_moonAmbientColor >> 8) & 0xff) / 255.0f;
            m_fMoonAmbientBlue = static_cast<float>((m_moonAmbientColor >> 16) & 0xff) / 255.0f;
            m_vMoonDiffuseRG.x = static_cast<float>(m_moonDiffuseColor & 0xff) / 255.0f;
            m_vMoonDiffuseRG.y = static_cast<float>((m_moonDiffuseColor >> 8) & 0xff) / 255.0f;
            m_fMoonDiffuseBlue = static_cast<float>((m_moonDiffuseColor >> 16) & 0xff) / 255.0f;
            m_vNightFogColor.x = static_cast<float>(m_moonFogColor & 0xff) / 255.0f;
            m_vNightFogColor.y = static_cast<float>((m_moonFogColor >> 8) & 0xff) / 255.0f;

            // Day fog and ambient colors.
            m_vDayFogColor.x = static_cast<float>(m_nFogColor & 0xff) / 255.0f;
            m_vDayFogColor.y = static_cast<float>((m_nFogColor >> 8) & 0xff) / 255.0f;
            m_vDayFogColor.z = static_cast<float>((m_nFogColor >> 16) & 0xff) / 255.0f;
            m_vNightFogColor.z = static_cast<float>((m_moonFogColor >> 16) & 0xff) / 255.0f;
            m_vSunAmbientRG.x = static_cast<float>(m_sunAmbientColor & 0xff) / 255.0f;
            m_vSunAmbientRG.y = static_cast<float>((m_sunAmbientColor >> 8) & 0xff) / 255.0f;
            m_fSunAmbientBlue = static_cast<float>((m_sunAmbientColor >> 16) & 0xff) / 255.0f;
            m_vSunDiffuseRG.x = static_cast<float>(m_sunDiffuseColor & 0xff) / 255.0f;
            m_vSunDiffuseRG.y = static_cast<float>((m_sunDiffuseColor >> 8) & 0xff) / 255.0f;
            m_fSunDiffuseBlue = static_cast<float>((m_sunDiffuseColor >> 16) & 0xff) / 255.0f;

            if (m_isNight == 0)
                SetAsDay();
            else
                SetAsNight();

            // Ambient color.
            m_vAmbientColor.x = static_cast<float>(m_dynAmbientColor & 0xff) / 255.0f;
            m_vAmbientColor.y = static_cast<float>((m_dynAmbientColor >> 8) & 0xff) / 255.0f;
            m_vAmbientColor.z = static_cast<float>((m_dynAmbientColor >> 16) & 0xff) / 255.0f;
            m_pScene->SetAmbient(m_vAmbientColor);

            m_pScene->FinishSceneGeometry();
            m_pScene->ComputeStaticLighting();
            pCamera->AttachToScene(m_pScene);
            SetWind(m_windPower);

            pModule->SetTimeOfDayState(nModState, 0);
            pModule->m_nCalendarYear = nModCalendarYear;
            pModule->m_nCalendarMonth = nModCalendarMonth;
            pModule->m_fTimeOfDay = static_cast<float>(nModTimeMs) / 1000.0f;
            pModule->m_nCalendarDay = nModCalendarDay;
            pModule->m_nDuskHour = bModEd;
            pModule->m_nCalendarHour = nModCalendarHour;
            pModule->m_nDawnHour = bModEc;

            UpdateSoundObjectsTimeOfDay();
            m_pAmbientSound->UpdateTimeOfDay();

            SetHideFadeGeometryMode(
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nHideFadeGeometryMode);

            if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() != 0)
                g_pAppManager->m_pClientExoApp->GetPlayerCreature()->RefreshVisualEffects(0);

            int nObjIds = m_objectIdList.m_count;
            for (int i = 0; i < nObjIds; i++) {
                CSWCCreature* pCre = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
                    m_objectIdList.m_pArray[i]);
                if (pCre != 0)
                    pCre->UpdateAreaId();
            }
            return 1;
        }
    }

    delete m_pScene;
    m_pScene = 0;
    return 0;
}

CSWCArea::CSWCArea()
    : CSWArea()
    , CGameObject(OBJECT_TYPE_AREA, OBJECT_INVALID)
    , m_vMoonAmbientRG(0.0f, 0.0f)
    , m_fMoonAmbientBlue(0.0f)
    , m_vMoonDiffuseRG(0.0f, 0.0f)
    , m_fMoonDiffuseBlue(0.0f)
    , m_vNightFogColor(0.0f, 0.0f, 0.0f)
    , m_vSunAmbientRG(0.0f, 0.0f)
    , m_fSunAmbientBlue(0.0f)
    , m_vSunDiffuseRG(0.0f, 0.0f)
    , m_fSunDiffuseBlue(0.0f)
    , m_vDayFogColor(0.0f, 0.0f, 0.0f)
    , m_vAmbientColor(0.0f, 0.0f, 0.0f)
{
    m_areaName = "";
    m_apWeatherGobs[5] = 0;
    m_apWeatherGobs[4] = 0;
    m_apWeatherGobs[3] = 0;
    m_apWeatherGobs[2] = 0;
    m_apWeatherGobs[1] = 0;
    m_apWeatherGobs[0] = 0;
    m_pSunGob = 0;
    m_pScene = 0;
    CSWArea::InitializeStaticObjects();
    m_pLightningGob = 0;
    m_v2iDawnDusk = CSWCAreaInt2(0, 0);
    m_isNight = 0;
    m_bDayNightTransitioning = 0;
    m_vMoonAmbientRG.x = 0.0f;
    m_vMoonAmbientRG.y = 0.0f;
    m_fMoonAmbientBlue = 0.0f;
    m_vMoonDiffuseRG.x = 0.0f;
    m_vMoonDiffuseRG.y = 0.0f;
    m_fMoonDiffuseBlue = 0.0f;
    m_vNightFogColor.x = 0.0f;
    m_vNightFogColor.y = 0.0f;
    m_vNightFogColor.z = 0.0f;
    m_vSunAmbientRG.x = 0.0f;
    m_vSunAmbientRG.y = 0.0f;
    m_fSunAmbientBlue = 0.0f;
    m_vSunDiffuseRG.x = 0.0f;
    m_vSunDiffuseRG.y = 0.0f;
    m_fSunDiffuseBlue = 0.0f;
    m_vDayFogColor.x = 0.0f;
    m_vDayFogColor.y = 0.0f;
    m_vDayFogColor.z = 0.0f;
    m_vAmbientColor.x = 0.0f;
    m_vAmbientColor.y = 0.0f;
    m_vAmbientColor.z = 0.0f;
    m_bDayNightSwitchPending = 1;
    m_pAmbientSound = new CSWCAmbientSound(this);
    m_vWeatherPercentWindTimer = CSWCAreaVector2(0.0f, 0.0f);
    m_nCurrentWeather = WEATHER_CLEAR;
    m_weatherType = WEATHER_CLEAR;
    m_fLightningDelay = 0.0f;
    m_fLightningFlashTime = 0.0f;
    m_fReserved1 = 0.0f;
    m_weatherTransitionTime = 0;
    unsigned int nDay, nTime;
    CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    pTimer->GetWorldTime(&nDay, &nTime);
    m_nLastUpdateDay = nDay;
    m_nLastUpdateTime = nTime;
    m_nReserved2 = 0xffff;
    m_nReserved3 = 0xff;
    m_nReserved1 = 0;
    m_bShadowLightsChanged = 0;
    m_bIsDark = 0;
    m_pReserved1 = 0;
    m_nReserved4 = 0;
    m_nRooms = 0;
    m_nOldestCorpse = 0;
    m_nNextCorpse = 1;
    m_paRooms = 0;
    m_pMiniGame = 0;
    m_pServerArea = 0;
}
CSWCArea::~CSWCArea()
{
#if !KOTOR1
    for (int i = 0; i < m_toBeDeletedEmitters.m_count; i++) {
        CAurObject* pObject = m_toBeDeletedEmitters.m_pData[i];
        if (pObject != 0) {
            pObject->AttachToScene(0);
            delete pObject;
            m_toBeDeletedEmitters.m_pData[i] = 0;
        }
    }
#endif
    UnloadArea();
    delete m_pAmbientSound;
    m_pAmbientSound = 0;
    delete[] m_paRooms;
    m_paRooms = 0;
    delete m_pMiniGame;
    if (m_pServerArea != 0) {
        m_pServerArea->DetachFromClientArea();
        m_pServerArea = 0;
    }
}

// The debug-console command handlers are file-scope free functions with global binding, taking the
// argument string and returning a "Success" or "Failure" string.
//
// The sethighlight* handlers are stubs. Their tunables are debug-only, so only the reply is left.

const char* sethighlightamplitude(char* pszArgs) {
    K2_UNUSED(pszArgs);
    return "Success";
}

const char* sethighlightfrequency(char* pszArgs) {
    K2_UNUSED(pszArgs);
    return "Success";
}

const char* sethighlightsize(char* pszArgs) {
    K2_UNUSED(pszArgs);
    return "Success";
}

const char* sethighlightintensity(char* pszArgs) {
    K2_UNUSED(pszArgs);
    return "Success";
}

// Per-room ambient sound callback registered by LoadArea; the user-data pointer is the behaviour.
void PlayRoomSoundEvent(CAurObject* pObject, char* pszEvent, void* pUserData) {
    K2_UNUSED(pObject); K2_UNUSED(pszEvent);
    if (pUserData != 0)
        static_cast<CSWBehaviorAttachSound*>(pUserData)->PlaySoundA();
}

// setweatherdensity <weather> <density> <unused>: parses three ints, ignores the third and passes
// the first two to the area's virtual SetWeather. The guards return "Success" rather than the
// "Failure" of daytonight/nighttoday.
const char* setweatherdensity(char* pszArgs) {
    int nWeather;
    int nDensity;
    int nUnused;
    if (sscanf(pszArgs, "%i %i %i", &nWeather, &nDensity, &nUnused) != 3)
        return "Failure";

    if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
        || g_pAppManager->m_pClientExoApp->GetModule() == 0)
        return "Success";

    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    if (pArea == 0)
        return "Success";

    pArea->SetWeather(static_cast<unsigned char>(nWeather), static_cast<float>(nDensity));
    return "Success";
}

// daytonight <seconds>: forces the area into the dusk transition and restarts the module clock with
// a new hour length.
const char* daytonight(char* pszArgs) {
    if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
        || g_pAppManager->m_pClientExoApp->GetModule() == 0
        || g_pAppManager->m_pClientExoApp->GetModule()->m_pArea == 0)
        return "Failure";

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->SetAsDusk();
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    pModule->m_fHourLengthSec = static_cast<float>(atof(pszArgs));
    g_pAppManager->m_pClientExoApp->GetModule()->m_fTimeOfDay = 0.0f;
    return "Success";
}

// nighttoday <seconds>: forces the area into the dawn transition and restarts the module clock with
// a new hour length.
const char* nighttoday(char* pszArgs) {
    if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
        || g_pAppManager->m_pClientExoApp->GetModule() == 0
        || g_pAppManager->m_pClientExoApp->GetModule()->m_pArea == 0)
        return "Failure";

    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->SetAsDawn();
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    pModule->m_fHourLengthSec = static_cast<float>(atof(pszArgs));
    g_pAppManager->m_pClientExoApp->GetModule()->m_fTimeOfDay = 0.0f;
    return "Success";
}
