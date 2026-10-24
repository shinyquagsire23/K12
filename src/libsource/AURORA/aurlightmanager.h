#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include "List.h"

class CAurObjectLightContainer;
class Gob;
struct CAurRoom;
class PartLight;
class Scene;
struct Vector;

// The Aurora light manager collects the lights a scene needs, elects the shadow casters, assigns
// the hardware slots and feeds the vertex-program shader builders.
class LightManager {
public:
    static Gob* m_pOnlyGob;
    static int cm_bLightManagerUpdated;
    static int m_nMaxLights;
    static int m_nMaxShadowLights;

    int GetNearestLights(Scene* pScene, Vector v, float fRadius, int n);
    List<PartLight*>& GetBumpMapLights();
    List<PartLight*>& GetLensFlareLights();
    List<PartLight*>& GetPhongLights();
    // Rebuilds the shadow-caster list from scratch on every call.
    List<PartLight*>& GetShadowLights(int nMode);
    // Sums the render lights' contribution at a point; pExclude is optional.
    Vector GetEmitterLighting(Vector vPosition, List<PartLight*>* pExclude);
    // Both return 1 when the light was in the active set and its slot was touched, else 0.
    int EnableThisLight(PartLight* pLight);
    int DisableThisLight(PartLight* pLight);
    void DisablePhongLights();
    // Appends its lighting code to pszBuffer and returns the new end.
    static char* SetupVertexProgramLights(char* pszBuffer, int nLights, int nUnused);
    void SetUpVertexProgramLightShaderConstants(int nLights, int nUnused);
    void EnablePhongLights(int nMaxLights, int bSkipBumpMapped);

    LightManager();
    ~LightManager();

    virtual void PrioritizeLights(Vector const&, bool);
    virtual void EnableLights();
    virtual void EnableLightsDynamicOnly();
    virtual void AddLight(PartLight*);
    virtual void RemoveLight(PartLight*, bool);
    virtual void AddRenderLight(PartLight*);
    virtual void Reset();
    virtual void UpdateLights();
    virtual void EnableHighestPriorityLights(int);
    virtual void EnableHighestPriorityLightsFade(int, CAurObjectLightContainer*);
    virtual void DebugPrintLightInfo(int, Vector);
    virtual void DisableBumpMapLights();
    virtual void EnableBumpMapLights();
    virtual void CollectLightsForGivenRoom(CAurRoom*);
    virtual void SetupLightsForThisGob(Vector const&, float);
    virtual void SetupLightsForThisRoom(Vector const&, float);
    virtual void SetupLightsForThisPositionAndRadius(Vector const&, float, List<PartLight*>&);
    virtual void SetupLightsForThisPositionAndRadius(Vector const&, float);
    virtual void CombineLights(List<PartLight*> const&, List<PartLight*>&);
    virtual void PrioritizeIllumination(bool);
    virtual void PrioritizeShadow();
    virtual void PrioritizeBump();
    virtual void GroupLights();
    virtual void ControlLightsFade(List<PartLight*>&, List<PartLight*>&, List<PartLight*>&, int);
    virtual void EnableHeadLight();

    int m_bDynamicOnly;
    List<PartLight*> m_lights;
    List<PartLight*> m_dynamicGobLights;
    List<PartLight*> m_staticLights;
    List<PartLight*> m_enabledLights;
    List<PartLight*> m_setupLights;
    List<PartLight*> m_renderLights;
    List<PartLight*> m_shadowLights;
    List<PartLight*> m_bumpMapLights;
    List<PartLight*> m_fadingLights;
    List<PartLight*> m_fadingDynamicLights;
    List<PartLight*> m_previousRenderLights;
    List<PartLight*> m_previousDynamicLights;
    List<PartLight*> m_activeShadowLights;
    PartLight* m_pShadowSun;
    PartLight* m_pReserved1;
    List<PartLight*> m_lastShadowLights;
    List<PartLight*> m_lastDynamicShadowLights;
    List<PartLight*> m_lensFlareLights;
    List<PartLight*> m_phongLights;
    List<PartLight*> m_lastUploadedLights;
    List<float>      m_lastUploadedWeights;
    bool m_bBumpMapLightsDisabled;
};

#endif // LIGHTMANAGER_H
