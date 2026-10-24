
#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library (List.h grows from 8)
#endif
#include "AURORA/aurmodel.h"
#include <stdio.h>
#include "aurlightmanager.h"
#include "aurinternal.h"
#include "aurcamera.h"
#include "Quaternion.h"
#include "Vector.h"
#include "Matrix.h"

float cullvalueforkeys = 9.99999975e-05f;

int LightManager::m_nMaxLights = 3;
int LightManager::m_nMaxShadowLights = 1;

Gob* LightManager::m_pOnlyGob;

// qsort comparator for the render-light list: a light with no colour sorts last, then by node
// priority, the three-valued shadow preference and the per-light ratio, with the pointer to break
// ties.
int lightcmp(void const* a, void const* b)
{
    PartLight* pLightA = (*static_cast<Part* const*>(a))->AsPartLight();
    PartLight* pLightB = (*static_cast<Part* const*>(b))->AsPartLight();

    int nPriorityA = pLightA->m_nLightPriority;
    if (nPriorityA == 0)
        nPriorityA = static_cast<MdlNodeLight*>(pLightA->m_pNode)->m_nPriority;

    int nPriorityB = pLightB->m_nLightPriority;
    if (nPriorityB == 0)
        nPriorityB = static_cast<MdlNodeLight*>(pLightB->m_pNode)->m_nPriority;

    if (equal(pLightA->m_vLightColour, Vector()))
        return 1;

    int bNoColourB = equal(pLightB->m_vLightColour, Vector());

    if (nPriorityA < nPriorityB)
        return -1;
    if (bNoColourB)
        return -1;
    if (nPriorityA > nPriorityB)
        return 1;

    int nFlagA = pLightA->m_nIsDynamic;
    if (nFlagA == 0) {
        if (pLightB->m_nIsDynamic > 0)
            return 1;
    } else if (nFlagA == 1) {
        if ((pLightB->m_nIsDynamic | 2) == 2)
            return -1;
    } else if (nFlagA == 2) {
        if (pLightB->m_nIsDynamic != 0)
            goto tail;
        return -1;
    } else {
        if (pLightB->m_nIsDynamic == 1)
            return 1;
    }

tail:
    if (pLightA->m_fDistanceRatio < pLightB->m_fDistanceRatio)
        return -1;
    if (pLightA->m_fDistanceRatio > pLightB->m_fDistanceRatio)
        return 1;
    return reinterpret_cast<uintptr_t>(pLightA) <= reinterpret_cast<uintptr_t>(pLightB) ? -1 : 1;
}

// Orders shadow casters with the same comparison, casters ahead of non-casters.
int shadowlightcmp(void const* a, void const* b)
{
    PartLight* pLightA = (*static_cast<Part* const*>(a))->AsPartLight();
    PartLight* pLightB = (*static_cast<Part* const*>(b))->AsPartLight();

    if (pLightA->m_nCastsShadows == 0)
    {
        if (pLightB->m_nCastsShadows != 0)
            return 1;
    }
    else if (pLightB->m_nCastsShadows == 0)
    {
        return -1;
    }

    int nPriorityA = pLightA->m_nLightPriority;
    if (nPriorityA == 0)
        nPriorityA = static_cast<MdlNodeLight*>(pLightA->m_pNode)->m_nPriority;

    int nPriorityB = pLightB->m_nLightPriority;
    if (nPriorityB == 0)
        nPriorityB = static_cast<MdlNodeLight*>(pLightB->m_pNode)->m_nPriority;

    if (equal(pLightA->m_vLightColour, Vector()))
        return 1;

    int bNoColourB = equal(pLightB->m_vLightColour, Vector());

    if (nPriorityA < nPriorityB)
        return -1;
    if (bNoColourB)
        return -1;
    if (nPriorityA > nPriorityB)
        return 1;

    int nFlagA = pLightA->m_nIsDynamic;
    if (nFlagA == 0) {
        if (pLightB->m_nIsDynamic > 0)
            return 1;
    } else if (nFlagA == 1) {
        if ((pLightB->m_nIsDynamic | 2) == 2)
            return -1;
    } else if (nFlagA == 2) {
        if (pLightB->m_nIsDynamic != 0)
            goto tail;
        return -1;
    } else {
        if (pLightB->m_nIsDynamic == 1)
            return 1;
    }

tail:
    if (pLightA->m_fDistanceRatio < pLightB->m_fDistanceRatio)
        return -1;
    if (pLightA->m_fDistanceRatio > pLightB->m_fDistanceRatio)
        return 1;
    return reinterpret_cast<uintptr_t>(pLightA) <= reinterpret_cast<uintptr_t>(pLightB) ? -1 : 1;
}

// LightManager owns every PartLight in the scene, sorts them by priority against a query position
// and drives the fixed-function GL light slots.

extern int enablegetnearestlights;

int nosortlights;
int countenabledlights;
int containsnegative;

extern int usearbvertexprograms;

int dynamiclightscount;

int staticlightscount;

void AurDisableHeadLight();
void AurEnableHeadLight();



extern unsigned int lightTranslationTable[];
enum {
    kLightAmbient       = 0,
    kLightDiffuse       = 1,
    kLightSpecular      = 2,
    kLightPosition      = 3,
    // Entry 4 is the quadratic attenuation; 9.0f / (r*r + r*r) through it is the D3D-style
    // falloff, not a direction.
    kLightSpotDirection = 4,
    kLightQuadraticAttenuation = 4,
};

Vector headlightambient(0.2f, 0.2f, 0.2f);
Vector headlightdiffuse(0.8f, 0.8f, 0.8f);

// The engine's lvalue-returning clamp pair. Returning const T& lets the shadow-fade clamps keep a
// reference rather than a temporary.
template <class T> static inline const T& min(const T& a, const T& b) { return b < a ? b : a; }
template <class T> static inline const T& max(const T& a, const T& b) { return a < b ? b : a; }

float DeltaT = 0.100000001f;
float shadowfadespeed = 0.300000012f;

float lightfadespeed = 3.0f;
float lightshadowfadespeed = 3.0f;

extern Camera* CurrentCamera;

const float* Homogenize(const Vector& v, const float& w);

int equal(const Vector& a, const Vector& b);

void SetLightGL(int nSlot, const Vector& vPosition, const Vector& vColour,
                const float& fRadius, const int& nMode);

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
extern "C" {
}

int LightManager::cm_bLightManagerUpdated = 1;

void LightManager::DebugPrintLightInfo(int nLight, Vector v)
{
    K2_UNUSED(nLight); K2_UNUSED(v);
}

void LightManager::GroupLights()
{
}

void LightManager::CombineLights(const List<PartLight*>& lightsIn, List<PartLight*>& lightsOut)
{
    K2_UNUSED(lightsIn); K2_UNUSED(lightsOut);
}

// Empties the enabled-light list by zeroing its count, keeping the element buffer for reuse.
void LightManager::Reset()
{
    m_enabledLights.m_count = 0;
}

List<PartLight*>& LightManager::GetBumpMapLights()
{
    return m_bumpMapLights;
}

List<PartLight*>& LightManager::GetLensFlareLights()
{
    return m_lensFlareLights;
}

List<PartLight*>& LightManager::GetPhongLights()
{
    return m_phongLights;
}

// Uploads one light into a hardware slot. nMode swaps which of the two colours is ambient and which
// feeds diffuse and specular; the position always comes from vPosition. The light enum is
// nSlot + GL_LIGHT1 because the port reserves GL_LIGHT0 for the head light.
void SetLightGL(int nSlot, const Vector& vPosition, const Vector& vColour,
                const float& fRadius, const int& nMode)
{
    if (nMode != 0)
    {
        const float* pAmbient = Homogenize(vColour, 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightAmbient], pAmbient);
        const float* pDiffuse = Homogenize(Vector(0.0f, 0.0f, 0.0f), 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightDiffuse], pDiffuse);
        const float* pSpecular = Homogenize(Vector(0.0f, 0.0f, 0.0f), 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightSpecular], pSpecular);
    }
    else
    {
        const float* pAmbient2 = Homogenize(Vector(0.0f, 0.0f, 0.0f), 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightAmbient], pAmbient2);
        const float* pDiffuse2 = Homogenize(vColour, 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightDiffuse], pDiffuse2);
        const float* pSpecular2 = Homogenize(vColour, 1.0f);
        ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightSpecular], pSpecular2);
    }

    const float* pPosition = Homogenize(vPosition, 1.0f);
    ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightPosition], pPosition);

    float fAttenuation = fRadius;
    if (fAttenuation <= 0.1f)
        fAttenuation = 0.1f;
    float fQuadratic = 9.0f / (fAttenuation * fAttenuation + fAttenuation * fAttenuation);
    ASLgl::glLightfv(nSlot + GL_LIGHT1, lightTranslationTable[kLightQuadraticAttenuation],
                     &fQuadratic);
}

// Initialises the two raw pointers and the dirty flag.
LightManager::LightManager()
{
    m_bDynamicOnly = 0;
    m_pShadowSun = 0;
    m_pReserved1 = 0;
    m_bBumpMapLightsDisabled = false;
}

LightManager::~LightManager()
{
}

// The room-flavoured entry forwards to the shared query over the manager's own setup list. The gob
// variant below is the same call plus a cleared setup flag.
void LightManager::SetupLightsForThisRoom(const Vector& vPosition, float fRadius)
{
    SetupLightsForThisPositionAndRadius(vPosition, fRadius, m_setupLights);
}

void LightManager::SetupLightsForThisGob(const Vector& vPosition, float fRadius)
{
    SetupLightsForThisPositionAndRadius(vPosition, fRadius, m_setupLights);
    m_bDynamicOnly = 0;
}

// Releases the phong list by walking it backwards, releasing each light's hardware slot, then
// zeroing the count.
void LightManager::DisablePhongLights()
{
    for (int64_t i = m_phongLights.m_count - 1; i != -1; i--)
        EnableThisLight(m_phongLights.m_pData[i]);

    m_phongLights.m_count = 0;
}

// Commits the frame's lights, publishing the render set through the fade controller, then
// settling the head light.
void LightManager::EnableLights()
{
    if (enablegetnearestlights)
        return;

    if (m_renderLights.m_count >= 1)
        dynamiclightscount = m_renderLights.m_count;

    ControlLightsFade(m_fadingLights, m_previousRenderLights, m_renderLights, 0);

    if (CAurInternal::m_nHeadLightEnabled == 1)
        EnableHeadLight();
    else
        AurDisableHeadLight();

    m_bDynamicOnly = 0;
}

// Collects the lights that can reach the query sphere into the enabled list. The test is 2-D:
// only the horizontal distance matters. A light with a non-positive radius, or with
// neither live latch set, is skipped.
void LightManager::SetupLightsForThisPositionAndRadius(const Vector& vPosition, float fRadius)
{
    m_enabledLights.m_count = 0;

    for (int i = 0; i < m_lights.m_count; i++)
    {
        PartLight* pLight = m_lights.m_pData[i];

        if (pLight->m_fLightRadius <= 0.0f)
            continue;

        if (!pLight->m_nAffectDynamic && !pLight->m_nIsDynamic)
            continue;

        float fDx = pLight->m_vWorldPosition.x - vPosition.x;
        float fDy = pLight->m_vWorldPosition.y - vPosition.y;

        if (fDx < 0.0f)
            fDx = -fDx;
        if (fDy < 0.0f)
            fDy = -fDy;

        if (fDx - pLight->m_fLightRadius < fRadius && fDy - pLight->m_fLightRadius < fRadius)
            AddRenderLight(pLight);
    }
}

// The same query over a caller-supplied candidate list, with a doubled radius and an extra
// skip for a light whose colour is black.
void LightManager::SetupLightsForThisPositionAndRadius(const Vector& vPosition, float fRadius,
                                                       List<PartLight*>& lights)
{
    m_enabledLights.m_count = 0;

    for (int i = 0; i < lights.m_count; i++)
    {
        PartLight* pLight = lights.m_pData[i];

        if (pLight->m_fLightRadius <= 0.0f)
            continue;

        if (!pLight->m_nAffectDynamic && !pLight->m_nIsDynamic)
            continue;

        if (pLight->m_vLightColour.x == 0.0f && pLight->m_vLightColour.y == 0.0f &&
            pLight->m_vLightColour.z == 0.0f)
            continue;

        float fDx = pLight->m_vWorldPosition.x - vPosition.x;
        float fDy = pLight->m_vWorldPosition.y - vPosition.y;

        if (fDx < 0.0f)
            fDx = -fDx;
        if (fDy < 0.0f)
            fDy = -fDy;

        if (fDx - pLight->m_fLightRadius < fRadius * 2.0f &&
            fDy - pLight->m_fLightRadius < fRadius * 2.0f)
            AddRenderLight(pLight);
    }
}

// Re-derives every attached light's world-space position once per frame. Lights with no
// owning gob are static tile lights whose position never changes.
void LightManager::UpdateLights()
{
    Quaternion qOrientation(1.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < m_lights.m_count; i++)
    {
        PartLight* pLight = m_lights.m_pData[i];

#if !KOTOR1  // K1: every light is re-derived, gob or not
        if (!pLight->m_pGob)
            continue;
#endif

        pLight->GetWorld(pLight->m_vWorldPosition, qOrientation);
    }
}

// Publishes the top nMaxLights render lights into the hardware slots and switches the unused
// ones off. The port reserves GL_LIGHT0, so the scene's dynamic lights start at GL_LIGHT1.
void LightManager::EnableHighestPriorityLights(int nMaxLights)
{
    if (m_renderLights.m_count == 0)
        return;

    int nLights = m_renderLights.m_count;
    if (nLights > nMaxLights)
        nLights = nMaxLights;

    int i = 0;
    while (i < nLights)
    {
        int nSlot = i++;

        ASLgl::glEnable(GL_LIGHT1 + nSlot);

        PartLight* pLight = m_renderLights.m_pData[nSlot];
        Vector vColour = pLight->m_vLightColour * pLight->m_fLightScale;

        SetLightGL(nSlot, pLight->m_vWorldPosition, vColour,
                   m_renderLights.m_pData[nSlot]->m_fLightRadius,
                   m_renderLights.m_pData[nSlot]->m_nAmbientOnly);
    }

    while (i < 3)
    {
        int nSlot = i++;
        ASLgl::glDisable(GL_LIGHT1 + nSlot);
    }
}

// Removes a light from every list it entered, mirroring AddLight's three-list filing, and from
// the per-frame derived lists when bRemoveFromRenderLists is set. Each list is tested for
// membership first, because List::Remove has no not-found exit.
void LightManager::RemoveLight(PartLight* pLight, bool bRemoveFromRenderLists)
{
    if (m_lights.Contains(pLight))
    {
        m_lights.Remove(pLight);

        if (!pLight->m_nIsDynamic)
            m_staticLights.Remove(pLight);

        if (pLight->m_pGob && (pLight->m_nIsDynamic || pLight->m_nAffectDynamic))
            m_dynamicGobLights.Remove(pLight);
    }

    if (m_lensFlareLights.Contains(pLight))
        m_lensFlareLights.Remove(pLight);

    if (bRemoveFromRenderLists)
    {
        if (m_renderLights.Contains(pLight))
            m_renderLights.Remove(pLight);

        if (m_fadingLights.Contains(pLight))
            m_fadingLights.Remove(pLight);

        if (m_fadingDynamicLights.Contains(pLight))
            m_fadingDynamicLights.Remove(pLight);

        if (m_previousRenderLights.Contains(pLight))
            m_previousRenderLights.Remove(pLight);

        if (m_previousDynamicLights.Contains(pLight))
            m_previousDynamicLights.Remove(pLight);

        if (m_lastShadowLights.Contains(pLight))
            m_lastShadowLights.Remove(pLight);

        if (m_lastDynamicShadowLights.Contains(pLight))
            m_lastDynamicShadowLights.Remove(pLight);
    }
}

// Files a light into up to three of the pool's lists, the master list always, the non-shadow
// list when the light has no shadow preference, and the gob list when it hangs off a gob
// and either live latch is set.
void LightManager::AddLight(PartLight* pLight)
{
    if (m_lights.Contains(pLight))
        return;

    Quaternion qOrientation;
    pLight->GetWorld(pLight->m_vWorldPosition, qOrientation);

    m_lights.Add(pLight);

    if (!pLight->m_nIsDynamic)
        m_staticLights.Add(pLight);

    if (pLight->m_pGob && (pLight->m_nIsDynamic || pLight->m_nAffectDynamic))
        m_dynamicGobLights.Add(pLight);
}

// Publishes an object's fading lights. Reconciles this frame's top-N render lights against the set
// the object was using last frame, ramping each weight over a few frames rather than
// popping, and re-uploads the GL slots only when the result changed.
void LightManager::EnableHighestPriorityLightsFade(int nMaxLights, CAurObjectLightContainer* pContainer)
{
    static List<PartLight*> listHighestPriorityLights;

    List<PartLight*>& lights = pContainer->m_lights;
    List<float>& weights = pContainer->m_lightWeights;

    listHighestPriorityLights.m_count = 0;

    for (int64_t i = 0; i < nMaxLights && i < m_renderLights.m_count; i++)
        listHighestPriorityLights.Add(m_renderLights.m_pData[i]);

    int64_t nIdx = lights.m_count;
    while (nIdx)
    {
        nIdx--;

        PartLight* pLight = lights.m_pData[nIdx];
        MdlNodeLight* pNode = pLight->m_pNode->AsMdlNodeLight();

        if (listHighestPriorityLights.Contains(pLight))
        {
            if (pNode->m_nFadingEnabled)
                weights.m_pData[nIdx] = min(1.0f, weights.m_pData[nIdx] + DeltaT * lightfadespeed);
            else
                weights.m_pData[nIdx] = 1.0f;
        }
        else
        {
            if (pNode->m_nFadingEnabled)
                weights.m_pData[nIdx] = max(0.0f, weights.m_pData[nIdx] - DeltaT * lightfadespeed);
            else
                weights.m_pData[nIdx] = 0.0f;

            if (weights.m_pData[nIdx] <= 0.0f)
            {
                lights.DelIndex(static_cast<int>(nIdx));
                weights.DelIndex(static_cast<int>(nIdx));
            }
        }
    }

    for (int64_t i = 0; lights.m_count < nMaxLights
                     && i < listHighestPriorityLights.m_count; i++)
    {
        PartLight* pLight = listHighestPriorityLights.m_pData[i];

        if (lights.Contains(pLight))
            continue;

        lights.Add(pLight);
        weights.Add(0.0f);
    }

    PartLight* pShadowLight = 0;
    for (int64_t i = 0; i < m_renderLights.m_count; i++)
    {
        if (m_renderLights.m_pData[i]->m_nCastsShadows)
        {
            pShadowLight = m_renderLights.m_pData[i];
            break;
        }
    }

    if (pShadowLight)
    {
        if (!pContainer->m_pSelectedLight)
        {
            pContainer->m_pSelectedLight = pShadowLight;
            pContainer->m_fSelectedWeight =
                min(1.0f, pContainer->m_fSelectedWeight + DeltaT * lightshadowfadespeed);
        }
        else if (pContainer->m_pSelectedLight == pShadowLight)
        {
            pContainer->m_fSelectedWeight =
                min(1.0f, pContainer->m_fSelectedWeight + DeltaT * lightshadowfadespeed);
        }
        else
        {
            pContainer->m_fSelectedWeight =
                max(0.0f, pContainer->m_fSelectedWeight - DeltaT * lightshadowfadespeed);

            if (pContainer->m_fSelectedWeight <= 0.0f)
            {
                pContainer->m_pSelectedLight = pShadowLight;
                pContainer->m_fSelectedWeight =
                    min(1.0f, pContainer->m_fSelectedWeight + DeltaT * lightshadowfadespeed);
            }
        }
    }
    else if (pContainer->m_pSelectedLight)
    {
        pContainer->m_fSelectedWeight =
            max(0.0f, pContainer->m_fSelectedWeight - DeltaT * lightshadowfadespeed);

        if (pContainer->m_fSelectedWeight <= 0.0f)
            pContainer->m_pSelectedLight = 0;
    }

    int bChanged = 1;
    if (lights.m_count == m_lastUploadedLights.m_count)
    {
        bChanged = 0;
        for (int64_t i = 0; i < lights.m_count; i++)
        {
            if (lights.m_pData[i] != m_lastUploadedLights.m_pData[i]
                || weights.m_pData[i] != m_lastUploadedWeights.m_pData[i])
            {
                bChanged = 1;
                break;
            }
        }
    }

    if (bChanged)
    {
        m_lastUploadedLights = lights;
        m_lastUploadedWeights = weights;
        cm_bLightManagerUpdated = 1;

        for (int64_t i = 0; i < lights.m_count; i++)
        {
            PartLight* pLight = lights.m_pData[i];
            pLight->m_fEmitterScale = weights.m_pData[i];

            ASLgl::glEnable(static_cast<unsigned int>(GL_LIGHT1 + i));

            Vector vColour = pLight->m_vLightColour * pLight->m_fEmitterScale;

            SetLightGL(static_cast<int>(i), pLight->m_vWorldPosition, vColour,
                       pLight->m_fLightRadius, pLight->m_nAmbientOnly);
        }

        for (int j = lights.m_count; j < 3; j++)
            ASLgl::glDisable(GL_LIGHT1 + j);
    }

    m_renderLights = lights;
}

// Adds a light to the enabled list unless it is already there.
void LightManager::AddRenderLight(PartLight* pLight)
{
    if (m_enabledLights.Contains(pLight))
        return;

    m_enabledLights.Add(pLight);
}

// Releases the lights in the bump-map list. It walks a private copy because releasing a light
// can mutate that list.
void LightManager::DisableBumpMapLights()
{
#if KOTOR1  // K1: the body is empty
}
#else
    List<PartLight*> lights(m_bumpMapLights);

    for (int64_t i = 0; i < lights.m_count; i++)
    {
        DisableThisLight(lights.m_pData[i]);
        m_bBumpMapLightsDisabled = true;
        cm_bLightManagerUpdated = 1;
    }
}
#endif

// Takes the lights in the bump-map list back for shading.
void LightManager::EnableBumpMapLights()
{
    List<PartLight*> lights(m_bumpMapLights);

    for (int64_t i = 0; i < lights.m_count; i++)
    {
        EnableThisLight(lights.m_pData[i]);
        cm_bLightManagerUpdated = 1;
    }

    m_bBumpMapLightsDisabled = false;
}

// Uploads the head light's ambient, diffuse and position into GL_LIGHT0. The position is
// pinned to the camera, so the head light sits exactly where the viewer is.
void LightManager::EnableHeadLight()
{
    const float* pAmbient = Homogenize(headlightambient, 1.0f);
    ASLgl::glLightfv(GL_LIGHT0, lightTranslationTable[kLightAmbient], pAmbient);

    const float* pDiffuse = Homogenize(headlightdiffuse, 1.0f);
    ASLgl::glLightfv(GL_LIGHT0, lightTranslationTable[kLightDiffuse], pDiffuse);

    const float* pPosition = Homogenize(CurrentCamera->m_vPosition, 1.0f);
    ASLgl::glLightfv(GL_LIGHT0, lightTranslationTable[kLightPosition], pPosition);

    AurEnableHeadLight();
}

// Like EnableLights, but the render set is first filtered down to the dynamic lights, and
// only those take part in the fade.
void LightManager::EnableLightsDynamicOnly()
{
    if (enablegetnearestlights)
        return;

    List<PartLight*> lights;
    for (int64_t i = 0; i < m_renderLights.m_count; i++)
    {
        PartLight* pLight = m_renderLights.m_pData[i];

        if (pLight->m_nIsDynamic == 1)
            lights.Add(pLight);
    }

    staticlightscount = lights.m_count;

    ControlLightsFade(m_fadingDynamicLights, m_previousDynamicLights, lights, 1);

    if (CAurInternal::m_nHeadLightEnabled)
        EnableHeadLight();
    else
        AurDisableHeadLight();

    m_bDynamicOnly = 1;
}

// Refills the phong slots, skipping lights whose radius covers the whole room and, optionally,
// those already being bump-mapped.
void LightManager::EnablePhongLights(int nMaxLights, int bSkipBumpMapped)
{
    for (int64_t i = m_phongLights.m_count - 1; i != -1; i--)
        EnableThisLight(m_phongLights.m_pData[i]);

    m_phongLights.m_count = 0;

    for (int64_t i = 0; i < m_fadingDynamicLights.m_count; i++)
    {
        PartLight* pLight = m_fadingDynamicLights.m_pData[i];

        if (pLight->m_fLightRadius > 5000.0f)
            continue;

        if (bSkipBumpMapped)
        {
            if (m_bumpMapLights.Contains(pLight))
                continue;
        }

        if (!pLight->m_nAmbientOnly)
        {
            m_phongLights.Add(pLight);
            DisableThisLight(pLight);
        }

        if (m_phongLights.m_count >= nMaxLights)
            return;
    }
}

// The non-virtual entry Scene::RenderOnlyGobs uses: uploads up to three nearby lights straight
// into the hardware slots as it walks and returns how many it filled. It uses a true 3-D
// distance, unlike the Setup queries.
int LightManager::GetNearestLights(Scene* pScene, Vector vPosition, float fRadius, int nDynamicOnly)
{
    K2_UNUSED(pScene);
    if (!enablegetnearestlights)
        return 0;

    int64_t iLight = 0;
    int nLights = 0;
    for (; iLight < m_lights.m_count && nLights <= 2; iLight++)
    {
        PartLight* pLight = m_lights.m_pData[iLight];

        if (nDynamicOnly)
        {
            if (!pLight->m_nIsDynamic)
                continue;
        }
        else
        {
            if (!pLight->m_nIsDynamic && !pLight->m_nAffectDynamic)
                continue;
        }

        float fDistance = magnitude(pLight->m_vWorldPosition - vPosition);

        if (pLight->m_fLightRadius <= 0.0f)
            continue;

        if (fDistance > pLight->m_fLightRadius + fRadius)
            continue;

        int nSlot = nLights++;

        ASLgl::glEnable(GL_LIGHT1 + nSlot);

        Vector vColour = pLight->m_vLightColour * pLight->m_fLightScale;

        SetLightGL(nSlot, pLight->m_vWorldPosition, vColour,
                   pLight->m_fLightRadius, pLight->m_nAmbientOnly);
    }

    int i = nLights;
    while (i < 3)
    {
        int nSlot = i++;
        ASLgl::glDisable(GL_LIGHT1 + nSlot);
    }

    return nLights;
}

// Stamps every enabled light with its distance-to-radius ratio against the query position, then
// runs the illumination pass that thresholds it.
void LightManager::PrioritizeLights(const Vector& vPosition, bool bIgnoreIllumination)
{
    Vector vZero = { 0.0f, 0.0f, 0.0f };

    GroupLights();

    for (int64_t i = 0; i < m_enabledLights.m_count; i++)
    {
        PartLight* pLight = m_enabledLights.m_pData[i];

        Vector vDelta;
        vDelta.x = pLight->m_vWorldPosition.x - vPosition.x;
        vDelta.y = pLight->m_vWorldPosition.y - vPosition.y;
        vDelta.z = 0.0f;

        if (!equal(pLight->m_vLightColour, vZero) && pLight->m_fLightRadius > 0.0f)
            pLight->m_fDistanceRatio = magnitude(vDelta) / pLight->m_fLightRadius;
        else
            pLight->m_fDistanceRatio = 100000.0f;
    }

    PrioritizeIllumination(bIgnoreIllumination);
}

// Rebuilds the render list from the enabled set and sorts it, then picks at most one bump-map
// caster out of the top three.
void LightManager::PrioritizeIllumination(bool bIgnoreIllumination)
{
    m_renderLights.m_count = 0;

    Vector vZero = { 0.0f, 0.0f, 0.0f };

    for (int64_t i = 0; i < m_enabledLights.m_count; i++)
    {
        PartLight* pLight = m_enabledLights.m_pData[i];

        if (!bIgnoreIllumination && pLight->m_fDistanceRatio > 1.0f)
            continue;

        if (pLight->m_fLightRadius == 0.0f)
            continue;

        if (equal(pLight->m_vLightColour, vZero))
            continue;

        m_renderLights.Add(m_enabledLights.m_pData[i]);
    }

    qsort(m_renderLights.m_pData, m_renderLights.m_count, sizeof(PartLight*), lightcmp);

    m_bumpMapLights.m_count = 0;

    for (int64_t i = 0; i < m_renderLights.m_count && i <= 2; i++)
    {
        PartLight* pLight = m_renderLights.m_pData[i];

        if (!pLight->m_nAmbientOnly && pLight->m_nIsDynamic)
        {
            m_bumpMapLights.Add(pLight);
            break;
        }
    }
}

// Rebuilds the shadow-caster list from the enabled set and sorts it.
void LightManager::PrioritizeShadow()
{
    m_shadowLights.m_count = 0;

    for (int i = 0; i < m_enabledLights.m_count; i++)
    {
        PartLight* pLight = m_enabledLights.m_pData[i];

        if (!pLight->m_nCastsShadows)
            continue;

        if (pLight->m_nAmbientOnly)
            continue;

        if (pLight->m_vLightColour.x > 0.1f || pLight->m_vLightColour.y > 0.1f ||
            pLight->m_vLightColour.z > 0.1f)
            m_shadowLights.Add(pLight);
    }

    qsort(m_shadowLights.m_pData, m_shadowLights.m_count, sizeof(PartLight*), lightcmp);
}

// Rebuilds the shadow list for this frame. nMode 2 elects the single shadow sun from the master
// list; nMode 1 takes at most one light off the sorted shadow list; anything else fills up
// to the shadow budget. A light entering or leaving the set ramps its fade rather than
// popping.
List<PartLight*>& LightManager::GetShadowLights(int nMode)
{
    m_activeShadowLights.m_count = 0;

    if (nMode == 2)
    {
        int nBestPriority = 6;
        PartLight* pBest = 0;

        for (int64_t i = 0; i < m_lights.m_count; i++)
        {
            PartLight* pLight = m_lights.m_pData[i];

            if (!pLight->m_nCastsShadows)
                continue;
            if (pLight->m_nAmbientOnly)
                continue;
            if (pLight->m_fLightRadius < 1000.0f && pLight->m_nIsDynamic)
                continue;

            int nPriority = static_cast<MdlNodeLight*>(pLight->m_pNode)->m_nPriority;
            if (nPriority < nBestPriority)
            {
                nBestPriority = nPriority;
                pBest = pLight;
            }
        }

        if (pBest)
        {
            m_activeShadowLights.Add(pBest);
            m_pShadowSun = pBest;
        }

        return m_activeShadowLights;
    }

    for (int64_t i = 0; m_activeShadowLights.m_count < m_nMaxShadowLights && i < m_shadowLights.m_count; i++)
    {
        PartLight* pLight = m_shadowLights.m_pData[i];

        if (nMode == 1 && !pLight->m_nIsDynamic)
            continue;
        if (pLight->m_fDistanceRatio > 1.0f)
            continue;

        m_activeShadowLights.Add(pLight);

        if (nMode == 1)
            break;
    }

    List<PartLight*> lastShadowLights;
    int nRemember;

    if (nMode == 1)
    {
        lastShadowLights = m_lastDynamicShadowLights;
        nRemember = 1;
    }
    else
    {
        lastShadowLights = m_lastShadowLights;
        nRemember = 0;
    }

    for (int64_t i = 0; i < m_activeShadowLights.m_count; i++)
    {
        PartLight* pLight = m_activeShadowLights.m_pData[i];
        if (!lastShadowLights.Contains(pLight))
            pLight->m_fShadowFade = 0.0f;
    }

    for (int64_t i = 0; i < m_nMaxShadowLights && i < lastShadowLights.m_count; i++)
    {
        PartLight* pLight = lastShadowLights.m_pData[i];

        if (m_activeShadowLights.Contains(pLight))
            continue;

        pLight->m_fShadowFade = max(0.0f, pLight->m_fShadowFade + DeltaT * -2.0f / shadowfadespeed);

        if (pLight->m_fShadowFade > 0.0f)
            m_activeShadowLights.Insert(pLight, 0);
    }

    if (m_activeShadowLights.m_count > m_nMaxShadowLights)
        m_activeShadowLights.m_count = m_nMaxShadowLights;

    for (int64_t i = 0; i < m_activeShadowLights.m_count; i++)
    {
        PartLight* pLight = m_activeShadowLights.m_pData[i];
        pLight->m_fShadowFade = min(1.0f, pLight->m_fShadowFade + DeltaT / shadowfadespeed);
    }

    if (nRemember)
        m_lastDynamicShadowLights = m_activeShadowLights;
    else
        m_lastShadowLights = m_activeShadowLights;

    return m_activeShadowLights;
}

// Switches off the hardware slot a light currently occupies. The slot is the light's position
// in the active list, so it has to be searched for; returns 0 when the light is not in that
// list.
int LightManager::DisableThisLight(PartLight* pLight)
{
    List<PartLight*> lights;

    if (m_bDynamicOnly)
        lights = m_fadingDynamicLights;
    else
        lights = m_renderLights;

    if (lights.Contains(pLight) == 0)
        return 0;

    int nLight = GL_LIGHT0;
    for (int64_t i = 0; i < lights.m_count; i++)
    {
        if (lights.m_pData[i] == pLight)
        {
            nLight = static_cast<int>(GL_LIGHT1 + i);
            break;
        }
    }

    ASLgl::glDisable(nLight);
    return 1;
}

// The enable twin of the above.
int LightManager::EnableThisLight(PartLight* pLight)
{
    List<PartLight*> lights;

    if (m_bDynamicOnly)
        lights = m_fadingDynamicLights;
    else
        lights = m_renderLights;

    if (lights.Contains(pLight) == 0)
        return 0;

    int nLight = GL_LIGHT0;
    for (int64_t i = 0; i < lights.m_count; i++)
    {
        if (lights.m_pData[i] == pLight)
        {
            nLight = static_cast<int>(GL_LIGHT1 + i);
            break;
        }
    }

    ASLgl::glEnable(nLight);
    return 1;
}

// Rebuilds the bump-map list from the enabled set and sorts it, keeping only the
// highest-priority caster.
void LightManager::PrioritizeBump()
{
    m_bumpMapLights.m_count = 0;

    for (int i = 0; i < m_enabledLights.m_count; i++)
    {
        PartLight* pLight = m_enabledLights.m_pData[i];

        if (pLight->m_nAmbientOnly)
            continue;

        if (!pLight->m_nIsDynamic)
            continue;

        m_bumpMapLights.Add(pLight);
    }

    qsort(m_bumpMapLights.m_pData, m_bumpMapLights.m_count, sizeof(PartLight*), lightcmp);

    if (m_bumpMapLights.m_count < 2)
        return;

    m_bumpMapLights.m_count = 1;
}

// Answers the particle and emitter pass's point-light query. Sums the linear falloff contribution
// of every live light at vPosition, adds the head light's two colours when it is on, and clamps
// each component to 1.
Vector LightManager::GetEmitterLighting(Vector vPosition, List<PartLight*>* pExclude)
{
    Vector vTotal = { 0.0f, 0.0f, 0.0f };

    for (int64_t i = 0; i < m_fadingLights.m_count; i++)
    {
        PartLight* pLight = m_fadingLights.m_pData[i];

        if (pExclude && pExclude->Contains(pLight))
            continue;

        Vector vWorld = pLight->m_vWorldPosition;
        Vector vDelta = { vWorld.x - vPosition.x,
                          vWorld.y - vPosition.y,
                          vWorld.z - vPosition.z };

        float fDistance = magnitude(vDelta);
        if (fDistance == 0.0f)
            fDistance = 1.0e-6f;

        float fRadius = pLight->m_fLightRadius;
        if (fRadius == 0.0f)
            fRadius = 0.01f;

        float fFalloff = 1.0f - fDistance / fRadius;
        if (fFalloff < 0.0f)
            continue;

        vTotal.x = fFalloff * pLight->m_vLightColour.x * pLight->m_fEmitterScale + vTotal.x;
        vTotal.y = fFalloff * pLight->m_vLightColour.y * pLight->m_fEmitterScale + vTotal.y;
        vTotal.z = fFalloff * pLight->m_vLightColour.z * pLight->m_fEmitterScale + vTotal.z;
    }

    if (CAurInternal::m_nHeadLightEnabled)
    {
        vTotal.x = headlightdiffuse.x + vTotal.x + headlightambient.x;
        vTotal.y = headlightdiffuse.y + vTotal.y + headlightambient.y;
        vTotal.z = headlightdiffuse.z + vTotal.z + headlightambient.z;
    }

    Vector vResult;
    vResult.x = fminf(vTotal.x, 1.0f);
    vResult.y = fminf(vTotal.y, 1.0f);
    vResult.z = fminf(vTotal.z, 1.0f);

    return vResult;
}

// Runs the fading pipeline. Reconciles the lights currently ramped up against the set the frame
// wants, ramps each weight toward 1 or 0 by the frame delta, drops the ones that reached
// zero, admits new ones up to the hardware budget, sorts what is left and publishes it.
void LightManager::ControlLightsFade(List<PartLight*>& fading, List<PartLight*>& previous,
                                     List<PartLight*>& wanted, int bAlternateSet)
{
    if (nosortlights)
        return;

    // The budget is the hardware slot count, unless a debug mode caps it at the lights belonging
    // to one gob.
    int nMax = m_nMaxLights;
    if (m_pOnlyGob != 0) {
        int nOwned;
        for (nOwned = 0; nOwned < wanted.m_count; nOwned++)
            if (wanted.m_pData[nOwned]->m_pGob != m_pOnlyGob)
                break;
        if (nOwned < nMax)
            nMax = nOwned;
    }

    // Pass 1: ramp every already-fading light and drop the ones that hit zero.
    int i = fading.m_count;
    while (i) {
        i--;
        PartLight* pLight = fading.m_pData[i];
        float* pfWeight = bAlternateSet != 0 ? &pLight->m_fFadeWeightAlt : &pLight->m_fEmitterScale;

        if (wanted.Contains(pLight)) {
            if (wanted.GetIndex(pLight) < nMax) {
                if (*pfWeight < 1.0f)
                    *pfWeight = min(1.0f, *pfWeight + DeltaT);
            } else {
                *pfWeight -= DeltaT;
                if (*pfWeight < 0.0f) {
                    *pfWeight = 0.0f;
                    fading.DelIndex(i);
                }
            }
        } else if (m_lights.Contains(pLight)) {
            *pfWeight -= DeltaT;
            if (*pfWeight < 0.0f) {
                *pfWeight = 0.0f;
                fading.DelIndex(i);
            }
        } else {
            fading.DelIndex(i);
        }
    }

    // Pass 2: count the wanted lights that are not fading yet.
    int nNew = 0;
    for (int64_t k = 0; k < nMax && k < wanted.m_count; k++) {
        PartLight* pLight = wanted.m_pData[k];
        if (static_cast<MdlNodeLight*>(pLight->m_pNode)->m_nFadingEnabled != 0)
            continue;
        if (!fading.Contains(pLight))
            nNew++;
    }

    // Pass 3: evict from the back until the newcomers fit. The victim is the last light that is
    // either gone from the wanted set or sitting past the budget in it; if there is none the
    // reconciliation gives up.
    int nRoom = nMax - fading.m_count;
    while (nNew > nRoom) {
        int j;
        for (j = fading.m_count; j > 0; j--) {
            PartLight* pLight = fading.m_pData[j - 1];
            if (!wanted.Contains(pLight))
                break;
            if (wanted.GetIndex(pLight) > nMax)
                break;
        }
        if (j <= 0)
            break;
        fading.DelIndex(j - 1);
        nRoom++;
    }

    // Pass 4: admit the newcomers, seeding their weight from the model node's fade flag.
    for (int64_t k = 0; k < nMax && k < wanted.m_count && fading.m_count < nMax; k++) {
        PartLight* pLight = wanted.m_pData[k];
        float* pfWeight = bAlternateSet != 0 ? &pLight->m_fFadeWeightAlt : &pLight->m_fEmitterScale;

        float fWeight;
        if (fading.Contains(pLight)) {
            fWeight = *pfWeight;
        } else {
            fading.Add(pLight);
            if (static_cast<MdlNodeLight*>(pLight->m_pNode)->m_nFadingEnabled != 0) {
                *pfWeight = 0.0f;
                fWeight = 0.0f;
            } else {
                *pfWeight = 1.0f;
                fWeight = 1.0f;
            }
        }
        *pfWeight = min(1.0f, fWeight + DeltaT);
    }

    // Pass 5: sort, publish to the GL slots and disable the rest.
    qsort(fading.m_pData, fading.m_count, sizeof(PartLight*), lightcmp);

    countenabledlights = fading.m_count < nMax ? fading.m_count : nMax;
    containsnegative = 0;

    int64_t nSlot;
    for (nSlot = 0; nSlot < fading.m_count; nSlot++) {
        PartLight* pLight = fading.m_pData[nSlot];
        float fWeight = bAlternateSet != 0 ? pLight->m_fFadeWeightAlt : pLight->m_fEmitterScale;

        Vector vColour;
        vColour.x = fWeight * pLight->m_vLightColour.x * pLight->m_fLightScale;
        vColour.y = fWeight * pLight->m_vLightColour.y * pLight->m_fLightScale;
        vColour.z = fWeight * pLight->m_vLightColour.z * pLight->m_fLightScale;
        if (vColour.x < 0.0f)
            containsnegative = 1;

        ASLgl::glEnable(static_cast<unsigned int>(GL_LIGHT1 + nSlot));
        SetLightGL(static_cast<int>(nSlot), fading.m_pData[nSlot]->m_vWorldPosition, vColour,
                   fading.m_pData[nSlot]->m_fLightRadius, fading.m_pData[nSlot]->m_nAmbientOnly);
    }

    for (unsigned int nOff = static_cast<unsigned int>(nSlot); nOff < 3; nOff++)
        ASLgl::glDisable(GL_LIGHT1 + nOff);

    previous = wanted;
}

// Uploads the ARB vertex-program env parameters the matching shader text expects, the camera
// basis, the head light when it is enabled, then five slots per light counting down from the
// light's position. Every slot the light walk does not reach is zeroed by the tail loop.
void LightManager::SetUpVertexProgramLightShaderConstants(int nLights, int bFadeList)
{
    if (CurrentCamera == 0)
        return;

    Matrix matCamera = CurrentCamera->m_qOrientation.getmatrix();
    Vector vCamera = CurrentCamera->m_vPosition;

    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 92, matCamera.m_f[0], matCamera.m_f[3],
                                          matCamera.m_f[6], vCamera.x);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 91, matCamera.m_f[1], matCamera.m_f[4],
                                          matCamera.m_f[7], vCamera.y);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 90, matCamera.m_f[2], matCamera.m_f[5],
                                          matCamera.m_f[8], vCamera.z);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 89, 0.0f, 0.0f, 0.0f, 1.0f);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 0.0f);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 88, CurrentCamera->m_vPosition.x,
                                          CurrentCamera->m_vPosition.y,
                                          CurrentCamera->m_vPosition.z, 1.0f);

    int nEnv = 87;
    if (CAurInternal::m_nHeadLightEnabled) {
        nLights--;
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv, CurrentCamera->m_vPosition.x,
                                              CurrentCamera->m_vPosition.y,
                                              CurrentCamera->m_vPosition.z, 0.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 1, headlightambient.x,
                                              headlightambient.y, headlightambient.z, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 2, headlightdiffuse.x,
                                              headlightdiffuse.y, headlightdiffuse.z, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 3, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 4, 1.0f, 1.0f, 1.0f, 1.0f);
        nEnv -= 5;
    }

    List<PartLight*>& lights = bFadeList == 0 ? m_renderLights : m_fadingDynamicLights;

    int nUploaded = 0;
    for (int64_t i = 0; i < lights.m_count; i++) {
        if (nUploaded >= nLights - (m_phongLights.m_count > 0 ? 1 : 0))
            break;

        PartLight* pLight = lights.m_pData[i];
        if (m_bBumpMapLightsDisabled && m_bumpMapLights.Contains(pLight))
            continue;
        if (m_phongLights.Contains(pLight))
            continue;

        nUploaded++;

        float fScale = 1.0f;
        if (bFadeList == 0)
            fScale = pLight->m_fEmitterScale;

        float fRed = fScale * pLight->m_vLightColour.x;
        float fGreen = fScale * pLight->m_vLightColour.y;
        float fBlue = fScale * pLight->m_vLightColour.z;

        Vector vSet;
        Vector vClear;
        if (pLight->m_nAmbientOnly != 0) {
            vSet.x = fRed;
            vSet.y = fGreen;
            vSet.z = fBlue;
        } else {
            vClear.x = fRed;
            vClear.y = fGreen;
            vClear.z = fBlue;
        }

        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv, pLight->m_vWorldPosition.x,
                                              pLight->m_vWorldPosition.y,
                                              pLight->m_vWorldPosition.z, 0.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 1, vSet.x, vSet.y, vSet.z, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 2, vClear.x, vClear.y, vClear.z, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 3, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 4, 1.0f, 0.0f,
                                              9.0f / (pLight->m_fLightRadius
                                                      * pLight->m_fLightRadius * 2.0f), 1.0f);

        nEnv -= 5;
    }

    for (int i = nUploaded; i < nLights; i++) {
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv, 0.0f, 0.0f, 0.0f, 0.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 1, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 2, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 3, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 4, 1.0f, 1.0f, 1.0f, 1.0f);
        nEnv -= 5;
    }
}

// Appends the per-light half of a vertex program to pszBuffer and returns it. Two dialects come
// out of the same control flow: the ARB_vertex_program text when the capability is set, and
// the older fixed-function combiner text otherwise.
char* LightManager::SetupVertexProgramLights(char* pszBuffer, int nLights, int nUnused)
{
    K2_UNUSED(nUnused);
    static char temp[4096];
    static char temp2[1024];
    static char temp3[1024];

    if (nLights < 1)
        return const_cast<char*>("");

    Matrix matCamera;
    Vector vCamera;
    if (CurrentCamera != 0) {
        matCamera = CurrentCamera->m_qOrientation.getmatrix();
        vCamera = CurrentCamera->m_vPosition;
    }

    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 92, matCamera.m_f[0], matCamera.m_f[3],
                                          matCamera.m_f[6], vCamera.x);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 91, matCamera.m_f[1], matCamera.m_f[4],
                                          matCamera.m_f[7], vCamera.y);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 90, matCamera.m_f[2], matCamera.m_f[5],
                                          matCamera.m_f[8], vCamera.z);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 89, 0.0f, 0.0f, 0.0f, 1.0f);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 1.0f);
    if (usearbvertexprograms)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 88, vCamera.x, vCamera.y, vCamera.z, 0.0f);

    if (usearbvertexprograms)
        sprintf(temp, "MOV vFinalColour, program.env[89].xxxw;\n"
                      "DP4 vReg0.x, vReg10, program.env[92];\n"
                      "DP4 vReg0.y, vReg10, program.env[91];\n"
                      "DP4 vReg0.z, vReg10, program.env[90];\n"
                      "DP4 vReg0.w, vReg10, program.env[89];\n"
                      "MOV vReg10, vReg0;\n"
                      "DP3 vReg0.x, vNormalWorld, program.env[92];\n"
                      "DP3 vReg0.y, vNormalWorld, program.env[91];\n"
                      "DP3 vReg0.z, vNormalWorld, program.env[90];\n"
                      "MOV vNormalWorld, vReg0;\n");
    else
        sprintf(temp, "MOV R9.xyzw, c[89].xxxw;DP4 R0.x, R10, c[92];DP4 R0.y, R10, c[91];DP4 R0.z, R10, c[90];DP4 R0.w, R10, c[89];MOV R10, R0;DP3 R0.x, R11, c[92];DP3 R0.y, R11, c[91];DP3 R0.z, R11, c[90];MOV R11, R0;");
    strcat(pszBuffer, temp);

    int nEnv = 87;
    for (int i = 0; i < nLights; i++) {
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv, 0.0f, 0.0f, 0.0f, 0.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 1, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 2, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 3, 0.0f, 0.0f, 0.0f, 1.0f);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nEnv - 4, 1.0f, 1.0f, 1.0f, 1.0f);

        if (usearbvertexprograms) {
            sprintf(temp, "ADD vReg0, -vReg10, program.env[%d];\n"
                          "DP3 vReg0.w, vReg0, vReg0;\n"
                          "RSQ vReg1.w, vReg0.w;\n"
                          "MUL vReg2, vReg0, vReg1.w;\n"
                          "DST vReg3, vReg0.w, vReg1.w;\n"
                          "DP3 vReg3.w, vReg3, program.env[%d];\n"
                          "RCP vReg3.w, vReg3.w;\n"
                          "DP3 vReg0.x, vNormalWorld, vReg2;\n"
                          "LIT vReg0, vReg0;\n"
                          "MUL vReg0, vReg0, vReg3.w;\n", nEnv, nEnv - 4);
            sprintf(temp2, "MUL vReg1, vReg0.x, program.env[%d];\n"
                           "MAD vFinalColour.xyzw, vReg1, program.env[95], vFinalColour;\n", nEnv - 1);
            sprintf(temp3, "MUL vReg1, vReg0.y, program.env[%d];\n"
                           "MAD vFinalColour.xyzw, vReg1, program.env[94], vFinalColour;\n", nEnv - 2);
        } else {
            sprintf(temp, "ADD R0, -R10, c[%d];DP3 R0.w, R0, R0;RSQ R1.w, R0.w;MUL R2, R0, R1.w;DST R3, R0.w, R1.w;DP3 R3.w, R3, c[%d];RCP R3.w, R3.w;DP3 R0.x, R11, R2;LIT R0, R0;MUL R0, R0, R3.w;", nEnv, nEnv - 4);
            sprintf(temp2, "MUL R1, R0.x, c[%d];MAD R9.xyzw, R1, c[95], R9;", nEnv - 1);
            sprintf(temp3, "MUL R1, R0.y, c[%d];MAD R9.xyzw, R1, c[94], R9;", nEnv - 2);
        }

        strcat(pszBuffer, temp);
        strcat(pszBuffer, temp2);
        strcat(pszBuffer, temp3);

        nEnv -= 5;
    }

    const char* pszAmbient;
    const char* pszAlpha;
    const char* pszOutput;
    if (usearbvertexprograms) {
        pszAmbient = "ADD vFinalColour, vFinalColour, program.env[93];\n";
        pszAlpha = "MOV vFinalColour.w, program.env[94];\n";
        pszOutput = "MOV result.color, vFinalColour;\n";
    } else {
        pszAmbient = "ADD R9.xyzw, R9.xyzw, c[93].xyzw;";
        pszAlpha = "MOV R9.w, c[94].w;";
        pszOutput = "MUL o[COL0].xyzw, R9.xyzw, c[15].xxxx;";
    }
    strcat(pszBuffer, pszAmbient);
    strcat(pszBuffer, pszAlpha);
    strcat(pszBuffer, pszOutput);

    return pszBuffer;
}

void AurSetHeadLightProperties(Vector vDiffuse, Vector vAmbient)
{
    headlightdiffuse = vDiffuse;
    headlightambient = vAmbient;
}

void AurSetMaxLights(int nLights)
{
    LightManager::m_nMaxLights = nLights;
}

void AurSetNumShadowLights(int nLights)
{
    LightManager::m_nMaxShadowLights = nLights;
}

const char* numbershadowlights(char* pszArgs)
{
    int nShadowLights;

    if (sscanf(pszArgs, "%d", &nShadowLights) != 1)
        return "usage: numbershadowlights x";

    LightManager::m_nMaxShadowLights = nShadowLights;
    return "Success.";
}

const char* enableheadlight(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    AurEnableHeadLight();
    return "Success";
}

const char* disableheadlight(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    AurDisableHeadLight();
    return "Success";
}

const char* setheadlightproperties(char* pszArgs)
{
    Vector vDiffuse;
    Vector vAmbient;

    if (sscanf(pszArgs, "%f %f %f %f %f %f",
               &vDiffuse.x, &vDiffuse.y, &vDiffuse.z,
               &vAmbient.x, &vAmbient.y, &vAmbient.z) != 6)
        return "Usage: setheadlightproperties <diffuse rgb> <ambient rgb>";

    AurSetHeadLightProperties(vDiffuse, vAmbient);
    return "Success";
}

const char* setmaxlights(char* pszArgs)
{
    int nLights;

    if (sscanf(pszArgs, "%d", &nLights) != 1)
        return "Usage: setmaxlights <maxlights>";

    LightManager::m_nMaxLights = nLights;
    return "Success";
}
