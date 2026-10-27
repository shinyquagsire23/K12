#pragma once

// One attack's resolved animation data, filled in by CSWCCreature::UpdateAttackData and chained
// through m_pNext when more than one attack is queued.
class CSWCCombatAttackData {
public:
    // m_nDataType: what the queued fire-and-forget animation carries (AddFireForgetAnimation's
    // dispatch). An attack's results, a cast's target and spell, a hit reaction played behind the
    // animation, or just the animation's own length (FIXED_LENGTH).
    enum {
        ATTACK_DATA_NONE         = 0,
        ATTACK_DATA_ATTACK       = 1,
        ATTACK_DATA_CAST         = 4,
        ATTACK_DATA_REACTION     = 5,
        ATTACK_DATA_FIXED_LENGTH = 6,
    };

    CSWCCombatAttackData();
    ~CSWCCombatAttackData();

    void ClearAttackData();
    void Copy(CSWCCombatAttackData* pSource);

    unsigned char       m_nDataType;
    unsigned char       m_nCastTargetType;
    char                _pad02[0x8 - 0x2];
    unsigned long long  m_attackerObjectId;
    unsigned short      m_nAnimationLength;
    unsigned short      m_nTotalDamage;
    unsigned char       m_nAttackResult;
    char                _pad15[0x18 - 0x15];
    unsigned int        m_nReserved1;
    char                _pad1c[0x20 - 0x1c];
    unsigned long long  m_weaponObjectId;
    struct {
        unsigned short  m_nReaxnDelay;
        unsigned short  m_nReaxnAnimation;
        unsigned short  m_nReaxnAnimLength;
        unsigned short  m_nFeatId;
    }                   m_reaxnGroup;
    struct { int        m_bRangedAttack; } m_rangedAttackGroup;
    struct { int        m_bKillingBlow; } m_killingBlowGroup;
    unsigned char       m_nWeaponAttackType;
    char                _pad39[0x3c - 0x39];
    struct {
        unsigned int    m_fCastTargetX;
        unsigned int    m_fCastTargetY;
        unsigned int    m_fCastTargetZ;
    }                   m_castTargetGroup;
    struct {
        unsigned int    m_nSpellId;
        unsigned int    m_nCastLevel;
    }                   m_spellGroup;
    CSWCCombatAttackData* m_pNext;
    float               m_fAnimationSpeed;
    char                _pad5c[0x60 - 0x5c];

    // Scalar members copy one at a time, the grouped runs as whole aggregates, in this order.
    CSWCCombatAttackData& operator=(const CSWCCombatAttackData& o) {
        m_nDataType = o.m_nDataType;
        m_nCastTargetType = o.m_nCastTargetType;
        m_attackerObjectId = o.m_attackerObjectId;
        m_weaponObjectId = o.m_weaponObjectId;
        m_nAnimationLength = o.m_nAnimationLength;
        m_nTotalDamage = o.m_nTotalDamage;
        m_nAttackResult = o.m_nAttackResult;
        m_nReserved1 = o.m_nReserved1;
        m_reaxnGroup = o.m_reaxnGroup;
        m_castTargetGroup = o.m_castTargetGroup;
        m_spellGroup = o.m_spellGroup;
        m_fAnimationSpeed = o.m_fAnimationSpeed;
        m_rangedAttackGroup = o.m_rangedAttackGroup;
        m_killingBlowGroup = o.m_killingBlowGroup;
        m_nWeaponAttackType = o.m_nWeaponAttackType;
        m_pNext = o.m_pNext;
        return *this;
    }
};

// The client-side creature, with its model, animation, visual effects, portrait and bark bubbles.
// Only the fields that are touched are named, and vtable slots that are never called carry
// placeholder types.

#include "nwcobject.h"
#include "nwcanimbase.h"
#include "Vector.h"
#include "CExoArrayListDtors.h"
#include "CExoLinkedList.h"
#include "nwcvisualeffectonobject.h"
#include "exoresman.h"

class CAurScene;
class CAurObject;
class CExoString;
class CExoSoundSource;
class CExoStreamingSoundSource;
class CallBack;
class CSWGuiInterfaceAction;
class CSWCAnimBase;
class CSWGui3DSceneView;
class CSWCArea;
class CSWCAreaOfEffectObject;
class CSWCCombatAttackData;
class CSWCCreatureAppearance;
class CSWCCreaturePartyStatus;
class CSWCCreatureStats;
class CSWCSpellCastVisual;
class CSWCDoor;
class CSWCItem;
class CSWCModule;
class CSWCObject;
class CSWCPlaceable;
class CSWCProjectile;
class CSWCSoundObject;
class CSoundSet;
struct STR_RES;

// The appearance description SetAppearance consumes; only this prefix is modelled.
struct CCAppearanceCore {
    unsigned char  m_bTypeVariant;
    unsigned char  m_bTypeNumber;
    unsigned short m_aBodyParts[6];
    unsigned char  _pad0e[0x10-0x0e];
    unsigned long long m_oidHeadItem;
    unsigned int   m_bArmorEquipped;
    unsigned short m_nAppearanceId;
    unsigned char  m_nHeadNumber;
    unsigned char  m_nGender;
    unsigned char  m_nPhenotype;
    unsigned char  m_nSkinColor;
    unsigned char  m_nHairColor;
    unsigned char  m_nTattooColor1;
    unsigned char  m_nTattooColor2;
    unsigned char  m_tailVariant;
    unsigned char  m_wingVariant;
    unsigned char  _pad27[0x34-0x27];
    unsigned int   m_bUseBackupHead;
    unsigned char  m_race;
    unsigned char  _pad39[0x40-0x39];
};
struct CCAppearanceInfo { CCAppearanceCore core; };

#include "CSWVisibilityNode.h"
#include "nwccreaturestats.h"

class CSWCStore;
class CSWCTrigger;
struct CSWVisibilityNode;
class CSWCVisualEffectOnObject;
class CSWSArea;
class CSWSAreaOfEffectObject;
class CSWSCreature;
class CSWSDoor;
class CSWSEncounter;
class CSWSItem;
class CSWSModule;
class CSWSObject;
class CSWSPlaceable;
class CSWSSoundObject;
class CSWSStore;
class CSWSTrigger;
class CSWSWaypoint;

class CResRef;
class Gob;
struct Vector;

class CSWCCreature : public CSWCObject {
public:
    // Consumed through the GOT, so they stay undefined here; an in-class value would let callers
    // fold the constant.
    static const int MODE_DEFAULT;
    static const int MODE_SWDRIVE;
    static const int MODE_SWCOMBAT;

    // GetInterfaceTargetType: the kind of target the radial menu is built for. OPENABLE is a door or
    // a placeable with an inventory, NONHOSTILE a creature that is not hostile or any other
    // placeable, HOSTILE a hostile creature (or any creature while the target is changing).
    enum {
        INTERFACE_TARGET_NONE       = 0,
        INTERFACE_TARGET_OPENABLE   = 1,
        INTERFACE_TARGET_TRIGGER    = 2,
        INTERFACE_TARGET_NONHOSTILE = 3,
        INTERFACE_TARGET_HOSTILE    = 4,
    };

    Gob*           m_aHitVisualGobs[12];
    Gob*           m_pNastyVisualGob;
    Gob*           m_pReserved1;
    Gob*           m_pReserved2;
    int            m_bInCombat;
    int            m_engagedState;
    int            m_bDisableLookAt;
    unsigned int   m_nGold;
#if !KOTOR1
    unsigned int   m_nItemComponent;
    unsigned int   m_nChemicals;
#endif
    int            m_nReserved1;
    int            m_bSuppressWeaponDraw;
    int            m_nReserved2;
    int            m_nRoundTimer;
    int            m_nWeaponOverlayTimer;
    char           _pad1ec[0x1f0-0x1ec];
    Gob*           m_pReserved3;
    Gob*           m_aSpecialAttackGobs[4];
    Gob*                m_pMonkVisuals;
    int            m_bMonkEyesLoaded;   // LoadMonkVisuals' two "already loaded" latches, tested by
    int            m_bMonkEyes01Loaded;   // alignment band and cleared by UnloadMonkVisuals
    CExoSoundSource* m_pLoopingCreatureSound;
    int            m_bLoopingSoundStarted;
    short          m_nReserved3;
    char           _pad236[0x238-0x236];
    int            m_bPlot;
    int            m_nReserved4;
    int            m_bDetectMode;
    int            m_nStealthState;
    int            m_bParryMode;
    int            m_bPowerAttackMode;
    int            m_bImprovedPowerAttackMode;
    int            m_nReserved5;
    int            m_bRapidShotMode;
    int            m_bFlurryMode;
    int            m_bVisibilityEffectApplied;
    int            m_nPlayerInfoExtra;
    int            m_bMovementSpeedLimited;
    char           _pad26c[0x270-0x26c];
    Vector*        m_pWayPoints;
    void*          m_pCurrentWayPoint;
    Vector         m_vMovingOrientation;
    Vector         m_vStationaryOrientation;
    Vector         m_desiredPosition;
    int            m_desiredPositionTimer;
    unsigned short m_wayPointCount;
    unsigned char  m_commandTag;
    unsigned char  m_appliedCommandTag;
    unsigned char  m_nClientPathId;
    char           _pad2ad[0x2b0-0x2ad];
    int            m_bServerWayPoints;
    float          m_fMoveSpeedScale;
    float          m_fWayPointSpeed;
    Vector         m_vReserved1;
    float          m_fWalkRate;
    float          m_fRunRate;
    int            m_bHasWayPointSpeed;
    unsigned char  m_creatureType;
    char           _pad2d5[0x2d8-0x2d5];
    CSWCCreatureAppearance* m_pCreatureAppearance;
#if KOTOR1
    unsigned long long m_aEquippedItemIds[18];
#else
    unsigned long long m_aEquippedItemIds[20];
#endif
    unsigned long long m_wieldedItemIdRight;
    unsigned long long m_wieldedItemIdLeft;
#if KOTOR1
    int            m_aLastInstant[18];
#else
    int            m_aLastInstant[20];
#endif
    CSWCCombatAttackData* m_pCurrentAttackData;
    CSWCCombatAttackData* m_pCurrentCastData;
    int            m_bConjureActive;
    int            m_conjureSpellId;
    unsigned long long m_conjureTargetId;
    int            m_blurWeapon;
    unsigned short m_nAIStateFlags;
    unsigned char  m_nAIStateAction;
    char           _pad407[0x408-0x407];
    unsigned short m_nActivityFlags;
    char           _pad40a[0x410-0x40a];
    unsigned long long m_oidCombatTarget;
    unsigned char  m_nReputation;
    CResRef        m_resReserved1;
    char           _pad429[0x430-0x429];
    CSWCSpellCastVisual* m_pSpellCastVisual;
    unsigned long long m_lockOrientationObjectId;
    CSWCCreatureStats* m_pStats;
    short          m_nSoundSetFile;
    char           _pad44a[0x44c-0x44a];
    int            m_bLastFootstepVisual;
    float          m_fFootstepGroundZ;
    int            m_nReserved6;
    CSWCCreaturePartyStatus* m_pPartyStatus;
    int            m_nAppearanceRequestTimer;
    int            m_bCameraReattachPending;
    int            m_bCameraFindClearView;
    char           _pad46c[0x470-0x46c];
    CCAppearanceInfo m_appearanceInfo;
    CSWCAnimBase*  m_pNewAnimBase;
    float          m_fPendingMoveSpeedScale;
    char           _pad4bc[0x4c0-0x4bc];
    CExoArrayList<CSWVisibilityNode*> m_visibilityList;
    CSoundSet*     m_pSoundSet;
    int            m_bDisguised;
    int            m_bCommandable;
    int            m_bExcited;
    char           _pad4e4[0x4e8-0x4e4];
    unsigned long long m_aLookAtTargetIds[4];
    unsigned short m_nAppearanceId;
    char           _pad50a[0x50c-0x50a];
    unsigned int   m_nAmbientPauseElapsed;
    unsigned int   m_nAmbientPauseDelay;
    int            m_bAmbientPausePlaying;
    unsigned long long m_areaObjectId;
    CExoArrayList<CSWCStats_SpellLikeAbility>* m_pSpellLikeAbilityList;
    int            m_bInParty;
    int            m_switchMode;
    int            m_nReserved7;
    float          m_swAnimationSpeed;
    float          m_fSWStrafeSpeed;
    int            m_fTurnAngle;
    int            m_nSWAnimation;
    int            m_attackIndex;
    int            m_lastAttackIndex;
    char           _pad54c[0x550-0x54c];
    unsigned long long m_thrownLightsaber;
    int            m_bEquippedItemsHidden;
    int            m_bHeadItemHidden;
    unsigned short m_nInitialAnimation;
    char           _pad562[0x564-0x562];
    int            m_bOverrideLightsaberState;
    unsigned long long m_oidBodyBag;
    unsigned char  m_bRunning;
    char           _pad571[0x578-0x571];
    unsigned long long m_lookAtTargetId;
    int            m_nForcedAttackAnimation;
    char           _pad584[0x588-0x584];
    CallBack*      m_aAnimationCallbacks[17];
    CExoSoundSource* m_pRollingFootstepSound;
    float          m_rollingFootstepVariance;
    int            m_bRollingFootstepPlaying;
    int            m_rollingFootstepType;
    unsigned char  m_nStateFlags;
    char           _pad625[0x626-0x625];
    unsigned short m_pendingDrawAnimation;
    float          m_fFreeLookTurnVel;
#if !KOTOR1
    int            m_nSwitchWeaponsCooldown;
    int            m_nLastAIUpdateTime;
    int            m_bIsPuppet;
#endif

    CSWCCreature();
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    virtual ~CSWCCreature();
    virtual void ResetUpdateTimes(unsigned int a, unsigned int b);
    virtual CSWCCreature* AsSWCCreature();
    virtual CExoString GetName();
    virtual void AIUpdate();
    virtual int LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char a);
    virtual void SetArea(CSWCArea* pArea);
    virtual void SetPosition(const Vector& v);
    virtual void AttachToScene(CAurScene* pScene);
    virtual Gob* GetModel(unsigned char a, int b);
    virtual void AddFireForgetAnimation(unsigned short a, void* p);
    virtual void Hilite(const Vector& v, unsigned char a, float f, int b);
    virtual void UnHilite(unsigned char a);
    virtual void* GetCameraFollowPart();
    virtual int GetFollowingBody();
    virtual float GetHeadHeightOffset();
    virtual void UpdatePopUpBubble();
    virtual void SetPortrait(const CResRef& r);
    virtual CResRef GetPortrait(unsigned char a);
    virtual void SetPortraitId(unsigned short a);
    virtual unsigned short GetPortraitId();
    virtual int ApplyShadowBlob();
    virtual void RemoveShadowBlob();
    virtual void TurnOnShadows();
    virtual void TurnOffShadows();
    virtual void SetQuededAnimationCallback();
    virtual int ClearQuededAnimationCallback();
    virtual int BarkString(unsigned long long a, const CExoString& s, const CResRef& r, int b);
    virtual int BarkBubble(unsigned long long a, const CExoString& s, const CResRef& r, int b);
    virtual int LipSync(const CResRef& r);
    virtual void ClientServerSyncHandler_Position(const Vector& v, int a, float f);
    virtual void ClientServerSyncHandler_Orientation(const Vector& v);
    virtual void ClientServerSyncHandler_Animation(int a, float f);
    virtual int IsHostile();
    virtual char GetReticlePoint(Vector& v);
    virtual CResRef GetPortrait(int a);
    virtual void* GetCurrentWayPoint();
    virtual Vector* GetEndWayPoint();
    virtual Vector* GetStartWayPoint();
    virtual int RequestModel(const CResRef& r1, const CResRef& r2, unsigned char a);
    int CanSee(CSWCObject* pTarget);
    virtual int  MoveToNextWayPoint();
    // </VTABLE-GEN>

    CSWSCreature* GetServerCreature();
    void SetLockOrientationToObject(unsigned long long objId, int flag);
    void ReAttachCamera();
    void ResetCameraToFollow();
    void SetNewLoopingCreatureSound(const CExoString& s);
    void SetName(const CExoString& first, const CExoString& last);
    // pos is an out-parameter; returns 1 when the server was reached.

    int  ClientSideWalkCommand(Vector& pos, unsigned char a, unsigned char b);
    int  LoadRollingFootstepSound();
    void Setup3DScene(CSWGui3DSceneView& view, CSWCCreature* pCreature, CSWSCreature* pServer,
                      int bSetAnimation);
    void FollowAnimationUpdate();
    void DriveAnimationUpdate();
    float GetDriveAcceleration();
    void PlayDrawAnimation(unsigned short anim);
    int  GetFreeLookVideoEffect();
    void PerformUnEquipItem(unsigned long long itemId);
    void PartyMenuStealth(unsigned int n, CSWCCreature* pTarget);
    void SetSoundSet(unsigned short nSoundSet);
    void SetWayPoints(unsigned short nCount, Vector* pPoints);
    void EvaluateNewWayPoints(int nCount, Vector* pPoints, int nId, const Vector& vNew);
    float ComputeSpeedFactor(float f);
    Vector GetRandomPoint(float f);
    int  UpdateRangedAttackData(int a, int b);
    void MoveCreatureToPosition(const Vector& vPos);
    // No-ops, kept for the interface.

    void HandleSwitchWeapon();
    void UpdateMonkVisuals();
    void ShowFeedbackBubbleText();
    void SetPopUpBubbleDecayTime(float f);
    int  CanSwapWeapons();
    int  CanUseMonkVisuals(int a);
    int  Moving();
    float GetCameraOffset();
    void SetEngagedState(int engagedState);
    void SetThrownLightsaber(unsigned long long id);
    void ResetDesiredPositionTimer();
    void StopAmbientPauseAnim();
    void CGPauseCycle(float f, int b);
    void SetStealthState(int nState, int b);
    void HandleConjuringCreature();
    CExoString GetSaveCreatureFilename(const CExoString& s1, const CExoString& s2);
    void SetSWAnimationSpeed(float speed, float speed2);
    void SwitchMode(int mode);
    void SetCreatureType(unsigned char type);
    void SetAlignmentGoodEvil(short a);
#if !KOTOR1
    void SetBaseCNPCAlignment(short a);
#endif
    void UnRegisterCallbacks();
    void RegisterCallbacks();
    void HideEquippedHeadItem();
    void ShowEquippedHeadItem();

    void PerformSkill(unsigned char skill, unsigned char subType, unsigned long long objId);
    void PerformUseItem(unsigned long long itemId, int a, unsigned long long objId,
                        const Vector& pos);
    void PerformGiveItem(unsigned long long itemId, unsigned long long objId, int a);
    void PerformAttack(unsigned long long objId);
    void ShowEquippedItems();
    int  SetInParty(int partyFlag);
    int LookAt(unsigned long long objId, float f);
    void UpdateLookAt();
    int  IsInjured();
    int  SetPrecalculatedPosition(const Vector& v);
    void AnimateFireAndForget();
    void AnimationUpdate();
    unsigned long long GetEquippedItemID(unsigned int nSlot);
#if KOTOR1  // K1 has a 3-argument form; K2 only the wider one
    void EquipToInventorySlot(unsigned int nSlot, CSWCItem* pItem);
    void EquipToInventorySlot(unsigned int nSlot, CSWCItem* pItem, int) { EquipToInventorySlot(nSlot, pItem); }
#else
    void EquipToInventorySlot(unsigned int nSlot, CSWCItem* pItem, int bFlag);
#endif
    void SetEquippedItemID(unsigned int nSlot, unsigned long long itemId);
    void PerformEquipItem(unsigned long long itemId, unsigned int nSlot);
    void PerformToggleEquip(unsigned long long itemId1, unsigned long long itemId2);
    void StripEquippedItems();
    int  PlaySwingSound(const CExoString& s, CSWCCombatAttackData* pData,
                        int nCount);
    void PlayFootstep();
    void SetDesiredOrientationForLock();
    void SetCombatState(int bCombat);
    int  GetSpellLikeAbilityUsesLeft(unsigned int spellId, unsigned char nClass);
    int  GetMenuPoint(Vector& vOut);
    int  CanDualWield(CSWCItem* pItem);
    void ActionMenuSetMine(unsigned int itemId, CSWCCreature* pTarget);
    int  HasUsableItems(CSWCCreature* pOther);
    int  UpdateAttackData(unsigned short* pOutAnim, CSWCCombatAttackData* pData);
    int  UpdateMeleeAttackData(unsigned short a, int b, int c, int d);
    unsigned short ResolveReactionAnimation(unsigned short a, unsigned short b,
                                            int c);
    void ResolveSpecialAttackVisuals(int a, int b, int c);
    int  IsStealthCapable();
    void UpdateRelativePerception(CSWVisibilityNode* pNode);
    void UpdateRelativePerception(unsigned long long objId);
    void* GetCreatureLight(int a);
    void DoFootstepAudio(unsigned char flag);
    void DoFootstepVisual(unsigned char a, int b);
    void UnloadMonkVisuals();
    void LoadMonkVisuals();
    void LoadSpecialAttacks(unsigned char nType);
    void UpdateWeaponClass();
    void PlaySoundSetSound(unsigned short nSound, int b3D);
    int  PlayHitSound(CSWCCombatAttackData* pData, int a);
    int  SpawnHitVisuals(unsigned char a, CSWCCombatAttackData* pData, const Vector& v,
                         const CExoString& s);
    int  GetMatchingEquippedItemCount(CSWSItem* pItem);
    void UpdateShadowBlob();
    // ShowEquippedItems reaches this through a one-argument prototype, so only the first argument
    // is set at that site; the body never reads the second.
    int  ComputeWeaponOverlays(int a, int b);
#ifdef K2_HOST_BUILD
    void ComputeWeaponOverlays(int a) { ComputeWeaponOverlays(a, 0); }
#endif
    void HideEquippedItems(const CResRef& r1, const CResRef& r2);
    float GetLightSaberThrowRate();
    void SetLightSaberThrowRate(float rate);
    void DoFootstepRumble(int a);
    void SetRunLooksAwful(int a);
    void UpdateAreaId();
    void ComputeZValue(CSWCArea* pArea, Vector* pPos, float f, Vector* pVec);
    int  TestAIStateActionGroup(int group);
    void ClearVisibilityList();
    void SetOverrideLightsaberState(int on, int a, int b);
    int  GetForceShield();
    void PlaySpecialAttacks(unsigned char slot);
    void ActionMenuSwitchWeapons(unsigned int a, CSWCCreature* pTarget);
    void SetItemUnEquippedByMe(unsigned long long itemId);
    int  ShowLightSaberContactVisual(CSWCCombatAttackData* pData);
    void StopSpecialAttacks(unsigned char slot);
    void PerformCastSpell(unsigned int spellId, unsigned long long objId, const Vector& pos,
                          unsigned char a, unsigned char b, unsigned char c, int d);
    int  GetRelativeWeaponSize(CSWCItem* pItem);
    void PerformCombatFeat(unsigned short nFeat, CSWCCreature* pTarget);
    void ResolveAttackIndex(unsigned short anim, int a, int exclude);
    void SetSpecialWalkAnimation(unsigned char a);
    int  GetLastInstant(unsigned int nInstant);
    void SetLastInstant(unsigned int nInstant, int value);
    int  GetTargetInterfaceTargetType(CSWCObject* pTarget);
    int  IsRangedWeapon(unsigned char type);
    void SetDisableLookAt(int disable);
    void StopRollingFootstepSound();
    void PlayRollingFootstepSound(int type);
    void DeleteLoopingCreatureSound();
    int  ResetAnimationState();
    void GetWayPoints(unsigned short* pCount, Vector** ppPoints,
                      unsigned short* pCurrent);
    void PerformExamine(unsigned long long objId);
    void PerformQuickChat(int nChat);

    void PerformEmote(int emote);

    static const unsigned short CCREATURE_EMOTE_ANIMATION[13];  // indexed by emote id
    static const unsigned short CCREATURE_EMOTE_SOUNDID[13];    // 0xffff marks an emote with no sound
    void PerformDialog(unsigned long long objId);
    void PerformCancelDisguise();
    void PerformToggleMode(int mode, unsigned long long objId);
    void PerformFeat(int nFeat, unsigned long long objId);
    void PerformPickupItem(unsigned long long objId);
    void PerformCastSpellLikeAbility(unsigned int spellId, unsigned char subType,
                                     unsigned long long objId, const Vector& pos);
    void CreateNewSpellLikeAbilityList();
    void DeleteSpellLikeAbilityList();
    int  IsCreature();
    int  InCreatureCombat(CSWCObject* pObj);
    int  InCreatureCombat(unsigned long long objId);
    unsigned long long GetAttackingWeapon(int nAttack);
    int  IsDriveAnimationBlocked();
    // Whether the body model is up.
    int  ModelIsLoaded() { return m_pModel != 0 && m_pModel->GetModel(ANIM_MODEL_BODY) != 0; }
    // The facing the creature turns toward while it moves.
    void SetDesiredWalkOrientation(const Vector& v) { m_vMovingOrientation = normalize(v); }
    // The facing the creature turns toward while it stands.
    void SetDesiredStationaryOrientation(const Vector& v) { m_vStationaryOrientation = normalize(v); }
    int  GetSoundSetStrres(unsigned short nIdx, STR_RES& out);
    void PartyMenuAwareness(unsigned int n, CSWCCreature* p);
    void BlurWeapon(int blur);
    float GetHeadTurnHorizontal();
    float GetHeadTurnVertical();
    void RotateCreatureToOrientation(const Vector& v);
    void ForceAppearanceUpdate();
#if KOTOR1  // K1 has a 3-argument form that forwards to this one
    int  SetAppearance(CCAppearanceInfo* pInfo, unsigned char a, int b);
    int  SetAppearance(CCAppearanceInfo* pInfo, unsigned char a, int b, int) { return SetAppearance(pInfo, a, b); }
#else
    int  SetAppearance(CCAppearanceInfo* pInfo, unsigned char a,
                       int b, int c);
#endif

    void PerformUseHostileItem(unsigned long long itemId, CSWCCreature* pTarget);
    void PerformDropItem(unsigned long long objId);
    int  GetPauseReadyAnimation();
    void PartyMenuUseItem(unsigned int itemId, CSWCCreature* pTarget);
    void PartyMenuUseItemSelect(unsigned int itemId, CSWCCreature* pTarget);
    void GetUsableItems(CExoArrayList<CSWGuiInterfaceAction>& list,
                        unsigned char nType);
    void GetPersonalActions(int nType,
                            CExoArrayList<CSWGuiInterfaceAction>& list);
    void ActionMenuUsePassiveFeat(unsigned int nFeat, CSWCCreature* p);
    int  ResolveSpecialAttackVisualsMelee(int rank, int v2, int v3, int v4);

    void GetStances(CExoArrayList<CSWGuiInterfaceAction>& list, int nType);
#if !KOTOR1
    void GetBehaviors(CExoArrayList<CSWGuiInterfaceAction>& list);
#endif
    void GetSelfForcePowers(CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetHostileForcePowers(CSWCObject* pTarget,
                               CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetTargetActions(CSWCObject* pTarget, int nType,
                          CExoArrayList<CSWGuiInterfaceAction>& list);
    int  GetInterfaceTargetType(CSWCObject* pTarget);
    void GetCombatFeatsSkills(CExoArrayList<CSWGuiInterfaceAction>& list,
                              CSWCCreature* pCreature);
    int  CheckFeat(int a, int b, int c);
    void EnableFeatForMenu(int nFeat, CExoArrayList<CSWGuiInterfaceAction>& list,
                           void (CSWCObject::*pfnAction)(unsigned int, CSWCCreature*),
                           CSWCCreature* pCreature);
    void ActionMenuUseAttackFeat(unsigned int n, CSWCCreature* p);
    void GetFriendlyInterfaceActions(CSWCObject* pTarget, int nType,
                                     CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetMines(CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetNonCombatFeatsSkills(CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetHostileInterfaceActions(CSWCObject* pTarget, int nType,
                                    CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetHostileItems(CExoArrayList<CSWGuiInterfaceAction>& list,
                         CSWCCreature* pCreature);
    void CreateUsableItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list, CSWSItem* pItem,
                               int nEquipped, unsigned char nType);
    void GetHostileItems(CExoArrayList<CSWGuiInterfaceAction>& list,
                         CSWCPlaceable* pPlaceable);
    void CreateHostileItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list, CSWSItem* pItem,
                                CSWCPlaceable* pPlaceable);
    void CreateHostileItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list, CSWSItem* pItem,
                                CSWCCreature* pCreature);
    void GetSwappableImplants(CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetWookieeRage(CExoArrayList<CSWGuiInterfaceAction>& list);
    void GetDroidConfusion(CExoArrayList<CSWGuiInterfaceAction>& list,
                           CSWCObject* pTarget);
    void GetEquipmentForcePowers(CExoArrayList<CSWGuiInterfaceAction>& list, unsigned int a,
                                 int nExcludeMask, unsigned int nForceItemMask);
    void ActionMenuForm(unsigned int n, CSWCCreature* p);
    void ActionMenuAttack(unsigned int n, CSWCCreature* p);
#if !KOTOR1
    void ActionMenuSelectBehavior(unsigned int n, CSWCCreature* p);
#endif
    void ActionInitiateDialog(unsigned int n, CSWCCreature* p);
    void ActionPushAway(unsigned int n, CSWCCreature* p);
    void ActionMenuStance(unsigned int nStance, CSWCCreature* pTarget);
    void UpdateRollingFootstepSound();
#if KOTOR1
    void UpdateVisibleWeapons(unsigned int nSlot, CSWCItem* pItem, int a);
#endif
    void UpdateVisibleWeapons(unsigned int nSlot, CSWCItem* pItem, int a, int b);
    int  IsHostileUsableItem(CSWSItem* pItem);
#ifndef K2_HOST_BUILD
#endif
    // The middle argument is unused; callers pass 0 for it.
    int  IsLeaderUsableItem(CSWSItem* pItem, int unused, unsigned char nType);
#ifdef K2_HOST_BUILD
    int  IsLeaderUsableItem(CSWSItem* pItem, unsigned char nType)
    {
        return IsLeaderUsableItem(pItem, 0, nType);
    }
#endif
    int  IsFriendlyUsableItem(CSWSItem* pItem);
    int  CheckFeat(int a, int b, int c, int d);
    int  GetArmourClass();
    void HideEquippedItemsForCasting(const CResRef& r1, const CResRef& r2);
    void HandleItemProperties(CSWCItem* pItem, unsigned int n);
    void StopLipSync();
    void ActionInitiateCombatMode(unsigned int n, CSWCCreature* p);
    void PartyMenuUseHostileItem(unsigned int n, CSWCCreature* p);
    void ForceMenuAction(unsigned int n, CSWCCreature* p);
    void ForceMenuHostileAction(unsigned int n, CSWCCreature* p);
    void SetCombatMode(int a);
    void ActionMenuSwapImplantRegen(unsigned int n, CSWCCreature* p);
    void ActionMenuSwapImplantStr(unsigned int n, CSWCCreature* p);
    void ActionMenuSwapImplantAgi(unsigned int n, CSWCCreature* p);
    void ActionMenuSwapImplantEnd(unsigned int n, CSWCCreature* p);
    void ActionSwapImplant(int a);
    void SaveCharGenCreature();
    int  RequestAppearance(CCAppearanceInfo* pInfo, unsigned char flags);
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWCCreature) == 0x610, "CSWCCreature layout drift; K1's new sites pin 0x610");
#else
K2_LAYOUT_ASSERT(sizeof(CSWCCreature) == 0x638, "CSWCCreature layout drift; both new sites pin 0x638");
#endif

#include "exostring.h"

class CSWCCreaturePartyStatus {
public:    int                m_bPlayerCharacter;
    int                m_bAIStateFlag;
    int                m_bSingletonParty;
    int                m_bFactionLeader;
    int                m_bHasSelectable;

    char               _pad14[0x18 - 0x14];
    unsigned long long m_objectId;
    CExoString         m_sName;

    CSWCCreaturePartyStatus();
    ~CSWCCreaturePartyStatus();
};
