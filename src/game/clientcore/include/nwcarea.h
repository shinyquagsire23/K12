#pragma once

// Client-side area object, the rendering and sound side of CSWSArea.

#include "nwarea.h"
#include "gameobjectarray.h"
#include "exoresman.h"
#include "Vector.h"
#include "Quaternion.h"
#include "CExoArrayListDtors.h"
#include "exolinkedlistinternal.h"
#include "CExoLinkedList.h"
#include "List.h"
#include "swcroom.h"
#include "exostring.h"

class CSWSArea;
class CAurScene;
class Gob;
class CExoString;
class CSWMiniGame;
class CAurObject;
class CSWCAmbientSound;
class CSWCMapPin;
class CSWCCreature;
struct CWalkHitInfo;
class CAurCamera;
class CSWBehaviorAttachSound;
class CGuiAreaLoadscreen;

// One placeable-attached light entry, a placeable object id plus its light Gob.
struct CSWCPlaceableObjectLight {
    unsigned long long m_objectId;
    Gob*               m_pLight;
};

// A two-float xy pair.
struct CSWCAreaVector2 {
    float x;
    float y;
    CSWCAreaVector2(float a, float b) : x(a), y(b) {}
    CSWCAreaVector2() {}
};

// A two-int pair.
struct CSWCAreaInt2 {
    int x;
    int y;
    CSWCAreaInt2(int a, int b) : x(a), y(b) {}
    CSWCAreaInt2() {}
};

// Two non-virtual public bases, CSWArea first and then CGameObject. SetFog below hides CSWArea's
// by-value overload in this class's scope.
class CSWCArea : public CSWArea, public CGameObject {
public:
    // The weather SetWeather draws (m_nCurrentWeather, m_weatherType): the fx_rain or fx_snow
    // emitters, none when clear.
    enum {
        WEATHER_CLEAR = 0,
        WEATHER_RAIN  = 1,
        WEATHER_SNOW  = 2,
    };

    CSWCArea();
    virtual ~CSWCArea();
    // CSWArea's slots 2-4 are not redeclared here.
    virtual int SetWind(unsigned char nWind);
    virtual CSWCArea* AsSWCArea();
    virtual int SetGlobalLightDiffuse(int b, Vector color);
    virtual int SetGlobalLightAmbient(int a, Vector color);
    virtual int SetWeather(unsigned char nWeather, float f);
    virtual int SetFog(float fNear, float fFar, const Vector& color);

    CResRef       m_areaResRef;
    CExoArrayList<unsigned long long> m_triggerIdList;  // ids of the area's triggers
    CSWCAreaVector2 m_vMoonAmbientRG;
    float         m_fMoonAmbientBlue;
    CSWCAreaVector2 m_vMoonDiffuseRG;
    float         m_fMoonDiffuseBlue;
    Vector        m_vNightFogColor;
    CSWCAreaVector2 m_vSunAmbientRG;
    float         m_fSunAmbientBlue;
    CSWCAreaVector2 m_vSunDiffuseRG;
    float         m_fSunDiffuseBlue;
    Vector        m_vDayFogColor;
    Vector        m_vAmbientColor;
    CExoString    m_areaName;
    CAurScene*    m_pScene;
    Gob*          m_pSunGob;              // the sun Gob
    Gob*          m_apWeatherGobs[6];       // the weather emitter Gobs
    Gob*          m_pLightningGob;          // the lightning Gob
    int           m_oidLastWeaponUsed;
    int           m_nEntryY;
    int           m_nEntryZ;
    CSWCAreaInt2  m_v2iDawnDusk;            // day-night transition step
    int           m_bDayNightTransitioning;               // the transition-active latch
    int           m_bDayNightSwitchPending;               // set to 1 by the SetAs* family
    CExoArrayList<CSWCMapPin*> m_mapPins;
    CExoArrayList<unsigned long long> m_soundObjectIdList;
    CSWCAmbientSound* m_pAmbientSound;
    float         m_fLightningDelay;               // the lightning delay
    float         m_fLightningFlashTime;
    float         m_fReserved1;
    unsigned int  m_weatherTransitionTime;
    unsigned char m_nCurrentWeather;
    unsigned char m_weatherType;            // the weather start argument, 0 when stopped
    unsigned char m_aReserved2[0x274-0x272];
    CSWCAreaVector2 m_vWeatherPercentWindTimer;
    unsigned int  m_nLastUpdateDay;               // last world-time day seen by Update
    unsigned int  m_nLastUpdateTime;               // last world-time time seen by Update
    CExoLinkedList<CAurObject> m_hitCheckIgnoreList;
    int           m_nReserved1;
    CExoArrayList<unsigned long long> m_objectIdList;  // ids of the objects in this area
    CExoArrayList<CSWCPlaceableObjectLight*> m_placeableObjectLights;
    CExoArrayList<unsigned long long> m_staticPlaceableIdList;  // ids of the static placeables
    int           m_bShadowLightsChanged; // bumped when the shadow-light count changes
    int           m_bIsDark;               // cleared by GetIsDark
    List<Vector>  m_tilePathNodesA;
    List<int>     m_tilePathNodesB;
    void*         m_pReserved1;              // freed by UnloadArea
    unsigned short m_nReserved2;
    unsigned char m_nReserved3;
    unsigned char _pad2fb;
    int           m_nReserved4;
    int           m_nRooms;
    CSWCRoom*     m_paRooms;                // the contiguous room array
    CSWMiniGame*  m_pMiniGame;
    unsigned long long m_aCorpseIds[4];
    unsigned char m_nOldestCorpse;               // corpse-ring oldest index
    unsigned char m_nNextCorpse;               // corpse-ring insert index
    CSWSArea*     m_pServerArea;
    CExoLinkedList<CSWBehaviorAttachSound> m_behaviorAttachSounds;
#if !KOTOR1  // K1 deletes emitters at once, with no deferred list
    List<CAurObject*> m_toBeDeletedEmitters;
#endif

    int  LoadBorder();
    int  TranslateBitCompressedAddress(int a, int b, int* outRem, int* outQuo);
    int  GetRoomIndex(const Vector& pos, int a);
    void EmptyCAurObjectToHitCheckIgnoreList();
    void AddCAurObjectToHitCheckIgnoreList(CAurObject* pObject);
    void RemovePlaceableObjectLight(unsigned long long objectId, int bDeleteLight);
#if KOTOR1  // K1 passes the placeables.2da row as a byte
    int  AddPlaceableObjectLight(unsigned long long objectId, unsigned char nPlaceableId, Vector pos);
    int  AddPlaceableObjectStatic(unsigned long long objectId, unsigned char nPlaceableId,
                                  Vector vPos, Vector vFacing);
#else
    int  AddPlaceableObjectLight(unsigned long long objectId, int nPlaceableId, Vector pos);
    int  AddPlaceableObjectStatic(unsigned long long objectId, int nPlaceableId,
                                  Vector vPos, Vector vFacing);
#endif
    int  ParentCorpseExists(unsigned long long objectId);
    void ClearTilePathNodes();
    void UpdateShadowingLights();
    int  GetIsObjectCorpse(unsigned long long objectId);
    void* GetPlaceableObjectLight(unsigned long long objectId);
    void ForceBrightSunAndMoon();
    int  ClearLineOfSightOneWay(Vector from, Vector to, Vector* pOut);
    int  TestLineWalkable(CSWCCreature* pCreature, Vector from, Vector to, float f);  // stub,
                                                                                     // always 1
    // Finds or attaches the server-side area.
    CSWSArea* GetServerArea();
    void SetHideFadeGeometryMode(unsigned char mode);
    void StartSounds();
    void AttachmentFromServerArea(CSWSArea* pServerArea);
    void DetachFromServerArea();
    void StartWeather(unsigned char nWeatherType);
    void StopWeather(unsigned char nWeatherType);
    void UnloadArea();
    CSWCRoom* GetRoom(int nRoom);
    CSWCRoom* GetRoom(const Vector& pos);
    CSWCRoom* GetRoom(const CExoString& sRoom);
    CSWMiniGame* RestartMiniGame();
    void HiliteAreaObjects(int bHilite);
    void UnhighlightAreaTransitions();
    CSWCMapPin* GetMapPin(unsigned int nId, int nType);
    void ClearAllMapPins();
    void AddMapPin(CSWCMapPin* pPin);
    void RemoveMapPin(unsigned int nId);
    void AddCorpse(unsigned long long objectId);
#if !KOTOR1
    void AddToBeDeletedEmitter(CAurObject* pEmitter);
#endif
    int  SetFogColor(const Vector& color);
    void SetLightningDone();
    void DisplayAllTriggers(int b);
    void ChangeTriggerDisplay(int b);
    int  GetIsDark();
    int  GetObjectsAtPositionVisible(const Vector& pos);
    // Client-side direct-line walk test; 1 is clear, 0 blocked, -3 creature-blocked.
    int  TestDirectLine(unsigned long long objectId, const Vector& from, const Vector& to,
                        const float& hitFraction, float f, int g, CWalkHitInfo* pHit);
    void SetAsDay();
    void SetAsNight();
    void SetAsDawn();
    void SetAsDusk();
    void UpdateDayNightCycle(float deltaTime);
    void UpdateSoundObjectsTimeOfDay();
    void Update(float deltaTime);
    int  ApplyFireAndForgetVisualEffect(unsigned short nEffect, Vector pos);
    unsigned int GetSurfaceMaterial(Vector pos);
    int  LoadArea(CAurCamera* pCamera, CGuiAreaLoadscreen* pLoadscreen, int nFlags);
};
