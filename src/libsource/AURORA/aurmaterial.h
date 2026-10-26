#pragma once

// Per-mesh texture and blend-state record; a Part keeps one at m_pMaterial.

#include "aurblendmodes.h"

class CAurTexture;
class Part;

#include "List.h"
#include "SafePointer.h"
#include "Vector.h"

class Material {
public:
    Material();
    ~Material();

    void SetDownSampling(int nDownSample);

    int GetTextureTID();

    void ClearTextures();

    void ParseField(char* pszLine);

    void Init(char* pszName);

    void ReplaceTexture(char* pszPublicName, char* pszTextureName,
                        char* pszTexturePath, int* paSizes, int nSizes);

    void ReplaceTexture(char* pszPublicName, char* pszTextureName);

    void LoadTextures(Part* part);

    void SetBlendingMode(int nMode);
    void SetGUIBlendingMode();
    void SetMaterialColor(Vector vColor, float fAlpha);
    bool HasBumpMap();
    void BindTexture0();
    void BindTexture1();
    void BindTexture2();
    void BindTexture3();
    void BindBumpMap();
    void BindBumpyShinyMap();
    void BindEnvMap();

    // Releases the watcher's texture when it has gone bad, then hands out and advances that
    // texture's id index.
    void PurgeBadTexture(SafePointer<CAurTexture>& spr);

    bool HasEnvMap();

    // The bumpy-shiny map is this engine's cube map, the same watcher HasEnvMap and BindEnvMap use.
    bool HasBumpyShinyMap();

    int GetBumpMapTID();
    int GetBumpyShinyMapTID();
    int GetEnvMapTID();

    int BumpMapIsDiffuse();
    int BumpMapIsSpecular();

    bool HasNormalMap();

    void InitializeTextures(char* pszTexture, char* p2, char* p3, char* p4);

    // Seven separate watchers, not an array: the destructor destroys each element in place.
    SafePointer<CAurTexture> m_spr0;
    SafePointer<CAurTexture> m_spr1;
    SafePointer<CAurTexture> m_spr2;
    SafePointer<CAurTexture> m_spr3;
    SafePointer<CAurTexture> m_spr4;
    SafePointer<CAurTexture> m_spr5;
    SafePointer<CAurTexture> m_spr6;
    SafePointer<CAurTexture>& Textures(int i) { return (&m_spr0)[i]; }
    const SafePointer<CAurTexture>& Textures(int i) const { return (&m_spr0)[i]; }

    // Texture names read out of the .txi: bump map, bumpy shiny map, env map.
    char* m_apszMapNames[3] = {};

    List<Aurora::AuroraBlendModes> m_lstSrcBlend;
    List<Aurora::AuroraBlendModes> m_lstDstBlend;

    bool  m_bDecal;
    int   m_nRenderBMLMType;
    float m_fWaterAlpha;
};
