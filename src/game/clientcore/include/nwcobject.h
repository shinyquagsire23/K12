#pragma once


#include "gameobjectarray.h"
#include "nwcanimbase.h"
#include "CExoArrayListDtors.h"
#include "Vector.h"
#include "CExoLinkedList.h"
#include "exostring.h"
#include "exosound.h"
#include "exoresman.h"
#include "nwportrait.h"
#include "nwcvisualeffectonobject.h"

struct CExoLinkedListNode;
class CAurScene;
class CSWSObject;
class CSWCArea;
class CSWCDoor;
class CSWSDoor;
class CSWCModule;
class CSWSModule;
class CSWSArea;
class CSWCCreature;
class CSWSCreature;
class CSWCItem;
class CSWSItem;
class CSWCTrigger;
class CSWSTrigger;
class CSWCProjectile;
class CSWSPlaceable;
class CSWCPlaceable;
class CSWSAreaOfEffectObject;
class CSWCAreaOfEffectObject;
class CSWSWaypoint;
class CSWSEncounter;
class CSWCStore;
class CSWSStore;
class CSWSSoundObject;
class CSWCSoundObject;
class CSWCVisualEffectOnObject;
class CExoStreamingSoundSource;
class CGuiInGameFloatyText;
class Gob;
class CAurObject;

// One entry on the object's fire-forget animation stack, a fixed-capacity array of these.
struct CSWCFireForgetAnim {
    // An empty entry holds the pause animation (server animation 10000) for half a second.
    enum {
        PAUSE_ANIMATION     = 10000,
        DEFAULT_DURATION_MS = 500,
    };

    int            m_bActive;
    unsigned short m_nAnimation;
    char           _pad6[0x08-0x06];
    unsigned int   m_nStartDay;
    unsigned int   m_nStartTime;
    unsigned int   m_nDuration;
    float          m_fSpeed;
    void*          m_pPayload;

    CSWCFireForgetAnim()
        : m_bActive(0), m_nAnimation(PAUSE_ANIMATION), m_nStartDay(0), m_nStartTime(0),
          m_nDuration(DEFAULT_DURATION_MS), m_fSpeed(1.0f), m_pPayload(0) {}
};

// Shortest-arc orientation quaternion between two directions.
Quaternion RotationArc(Vector from, Vector to);

class CExoString;
class CResRef;
struct Vector;
class CSWCArea;
class CallBack;
class CSWCAnimBase;

class CSWCObject : public CGameObject {
public:
    // The model selectors of GetModel, LoadModel, SetAlpha and the other per-model calls
    // (CSWCAnimBase): a creature's separate head model, and the body model (the whole model of
    // anything else).
    enum {
        ANIM_MODEL_WINGS = 0xfc,   // an item keeps its property visual effect's model here
        ANIM_MODEL_HEAD  = 0xfe,
        ANIM_MODEL_BODY  = 0xff,
    };

    char                 _pad11[0x14-0x11];
    int                  m_bStoneTexture;
    int                  m_bBarkTexture;
    int                  m_bShadeTexture;
    int                  m_bNoProjectileAttach;
    char                 _pad24[0x28-0x24];
    CExoLinkedListNode*  m_pAINode;
    CSWCArea*            m_pArea;
    Vector               m_position;
    Vector               m_orientation;
    unsigned int         m_nLastAIUpdateDay;
    unsigned int         m_nLastAIUpdateTod;
    unsigned int         m_nAIUpdateDeltaMs;
    unsigned short       m_currentAnimation;
    unsigned short       m_queuedAnimation;
    float                m_animationSpeed;
    int                  m_bModelSyncDisabled;
    int                  m_bPlayingCutsceneAnimation;
    unsigned short       m_loopingAnimation;
    unsigned short       m_nFireForgetCapacity;
    CSWCFireForgetAnim*  m_paFireForgetAnims;
    unsigned char        m_nFireForgetCount;
    char                 _pad79[0x7c-0x79];
    int                  m_nFireForgetTimeLeftMs;
    CSWCAnimBase*        m_pModel;
    CExoLinkedList<CSWCVisualEffectOnObject> m_visualEffects;
    CExoArrayList<CGuiInGameFloatyText*> m_floatyText;
    int                  m_nReserved8;
    unsigned int         m_fadeStartDay;
    unsigned int         m_fadeStartTime;
    unsigned int         m_nFadeHoldMs;
    unsigned int         m_nFadeStartDelayMs;
    unsigned char        m_fadeState;
    unsigned char        m_desiredFadeState;
    char                 _padb6[0xb8-0xb6];
    int                  m_bTimedFade;
    float                m_finalFadeAlpha;
    int                  m_bSnapPositionOnUpdate;
    char                 _padc4[0xc8-0xc4];
    unsigned long long   m_nReserved9;
    float                m_fHeadHeight;
    CSWPortrait          m_portrait;
    unsigned short       m_portraitId;
    unsigned short       m_nLowerGaitAnimation;
    unsigned long long   m_oidLookAtOverride;
    int                  m_bSpokeChat;
    int                  m_bAttractsLookAt;
    CExoSoundSource*     m_pVocalization;
    CExoStreamingSoundSource* m_pStreamingVocalization;
    int                  m_vocalizationType;
    int                  m_bVisible;
    Vector               m_groundNormal;
    int                  m_bPendingRemoval;
    CExoString           m_feedbackInfo;
    int                  m_feedbackTimer;
    char                 _pad134[0x138-0x134];
    CallBack*            m_pQuededAnimationCallback;
    CSWSObject*          m_pServerObject;

    CSWCObject(unsigned char nObjectType);

    // -- vtable (slots 0..82, in declaration order) ----------------------------------------------
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    virtual ~CSWCObject();
    virtual void SetId(unsigned long long id);
    virtual CSWCObject* AsSWCObject();
    virtual CExoString GetName();
    // These four are pure: CSWCObject is abstract.
    virtual void AIUpdate() = 0;
    virtual int LoadModel(const CResRef& r, unsigned char a, unsigned char b) = 0;
    virtual void UnloadModel() = 0;
    virtual void UnloadModel(unsigned char a) = 0;
    virtual void SetArea(CSWCArea* pArea);
    virtual void SetOrientation(const Vector& v);
    virtual void SetPosition(const Vector& v);
    virtual void UpdateWalkMesh();
    virtual void AttachToScene(CAurScene* pScene);
    virtual Gob* GetModel(unsigned char a, int b);
    // Returns whether the animation was accepted.
    virtual int SetAnimation(unsigned short a, int b, int c);
    virtual void AddFireForgetAnimation(unsigned short a, void* p);
    virtual void Hilite(const Vector& v, unsigned char a, float f, int b);
    virtual void UnHilite(unsigned char a);
    virtual int SetEnvMap(int a, int b, const CResRef& r);
    virtual int SetEnvMap(unsigned char a, int b, int c, const CResRef& r);
    virtual int AddVisualEffect(unsigned short a, int b, unsigned long long c, unsigned long long d, unsigned char e, unsigned char f, const Vector& v);
    virtual int DeleteVisualEffect(unsigned short a);
    virtual void KillAllVisualEffects();
    virtual int RefreshVisualEffects(int a);
    virtual void UpdateVisualEffectsList();
    virtual void StopAllVisualEffects();
    virtual int HasVisualEffectApplied(unsigned int a);
    // The model part the camera follows.
    virtual void* GetCameraFollowPart();
    virtual int GetFollowingBody();
    virtual float GetHeadHeightOffset();
    virtual void ActivateHardCodedVisualWithHighestPriority(unsigned char a, CSWCVisualEffectOnObject* pParam);
    virtual void DetachVisualEffects();
    virtual void ReAttachVisualEffects();
    virtual void UpdatePopUpBubble();
    virtual void SetPortrait(const CResRef& r);
    virtual CResRef GetPortrait(unsigned char a);
    virtual void SetPortraitId(unsigned short a);
    virtual unsigned short GetPortraitId();
    virtual int ApplyShadowBlob();
    virtual void RemoveShadowBlob();
    virtual void ShowShadowBlob();
    virtual void HideShadowBlob();
    virtual void SetGroundNormal(const Vector& v);
    virtual void TurnOnShadows();
    virtual void TurnOffShadows();
    virtual void SetQuededAnimationCallback();
    virtual int ClearQuededAnimationCallback();
    virtual int BarkString(unsigned long long a, const CExoString& s, const CResRef& r, int b);
    virtual int BarkBubble(unsigned long long a, const CExoString& s, const CResRef& r, int b);
    virtual int LipSync(const CResRef&);
    virtual void ClientServerSyncHandler_Position(const Vector& v, int, float);
    virtual void ClientServerSyncHandler_Orientation(const Vector& v);
    virtual void ClientServerSyncHandler_Animation(int a, float speed);
    virtual int IsHostile();
    // 0, 1 or 2, selecting the reticle image.
    virtual char GetReticlePoint(Vector& v);
    virtual float GetCanSeeHeight();
    virtual float GetStickyMouseThreshhold();
    // </VTABLE-GEN>

    int  AnimationDialog(unsigned short a);
    void SetFadeState(unsigned char state);
    void SetDesiredFadeState(unsigned char state, int a, unsigned int b, unsigned int c);
    float GetFinalFadeAlpha();
    int  FadeObject();
    void SetObjectVisibilityOnSetArea(CSWCArea* pArea);

    void           AttachmentFromServerObject(CSWSObject* pServerObject);
    void           DetachFromServerObject();

    // Pop-up bubble and floaty-text surface; every one of these is a no-op.
    void HidePopUpBubble();
    void ShowPopUpBubble(const Vector& v);
    void KillPopUpBubble();
    void SetPopUpBubbleText(const CExoString& s, int a, float f, float g);
    void SetPopUpBubbleDefaultText(const CExoString& s);
    void SetPopUpBubbleDefaultCreatureID(unsigned long long id);
    void SetPopUpBubblePortrait(const CResRef& r);
    void UpdateFloatyText();
    int  RemoveAnimation(unsigned short a);
    void RemoveAllFireForgetAnimations();
    void RemoveTopFireForgetAnimation();
    void QueuedCallback(CAurObject* pObject, char* pszEvent);
    void TestAndReEquipweapons(int a, unsigned short nAnimation);
    void InsertFireForgetAnimation(unsigned short a, unsigned int nDuration,
                                   float fSpeed, void* p, unsigned int b);
    int  GetTopFireForgetAnimation(unsigned short* pAnimation, float* pSpeed,
                                   unsigned int* pDuration, void** ppPayload);

    CSWSObject* GetServerObject();
    float GetVisualEffectAlpha();
    void DetachVisualEffectsMatchingHardCodedClass(CSWCVisualEffectOnObject* pParam);
    void AbortFireForgetAnimation(int a);
    void NewVocalization(const CResRef& r, int a);
    void SetWind(int a, float f);
    void AcquireAnimationBase(CSWCObject* pSource);
    Vector GetCutsceneDummyPosition();
    unsigned int CalculateProjectileTimeToTarget(const Vector& target, int a);
    void SetFeedbackInfo(const CExoString& s);
    void GetFeedbackInfo(CExoString& out, unsigned int& outTimer);
    int  AnimationPlayOutOfPlace(unsigned short a);

    int AnimationStationary(unsigned short a);
    int AnimationPause(unsigned short a);
    int AnimationWalking(unsigned short a);
    int AnimationRunning(unsigned short a);
    int AnimationOverlay(unsigned short a);
    int AnimationDamage(unsigned short a);
    int AnimationParry(unsigned short a);
    int AnimationDodge(unsigned short a);
    int AnimationAttack(unsigned short a);
    int AnimationHideEquippedItems(unsigned short a);
    int AnimationLooping(unsigned short a);
    int AnimationFireAndForget(unsigned short a);
    void SetLoopingAnimation(unsigned short a);

    void ClearAllActions();
    void ClearAllQueuedCombatActions();
    void MenuActionUseSecuritySpike(unsigned int a, CSWCCreature* pCreature);
    unsigned short GetCurrentAnimation();
    CSWCAnimBase*  GetAnimationBase();
    unsigned short GetLoopingAnimation();

    void  ReplaceTexture(unsigned char a, const CResRef& r1,
                         const CResRef& r2, int b);
    void  ReplaceTexture(const CResRef& r1, const CResRef& r2, int b);
    void  ReplaceTexture(unsigned char a, const CResRef& r1, const CResRef& r2,
                         unsigned int c, unsigned short* p, int d, unsigned int e);
    void  ReplaceTexture(const CResRef& r1, const CResRef& r2, unsigned int c,
                         unsigned short* p, int d, unsigned int e);
    void  RestoreTexture();
    void  RestoreTexture(unsigned char a);
    void  EnableBumpedOutShell(CResRef r, float f);
    void  DisableBumpedOutShell();
    void  SetMotionBlur(int a);
    float GetAnimationLength(unsigned short a);
    void  SetAlpha(float alpha, unsigned char a);
    void  RestoreEnvMap();
    float GetAlpha();
    void  EnableDistortion();
    void  DisableDistortion();
};

// idiom-exempt: canonical
// One selectable entry in an in-game radial or action menu: the action id, the callback to run on
// it and its target. Most Get*Actions methods fill a list of these.
class CSWCCreature;

class CSWGuiInterfaceAction {
public:
    // The kind bits or-ed into m_nActionId above the id they carry: a feat, a spell (a power or a
    // stance), or an item's object id naming an item to use (the menu callbacks clear the spell and
    // item bits again with the _CLEAR masks).
    enum {
        ACTION_ID_FEAT = 0x10000000,
    };
    enum : unsigned int {
        ACTION_ID_SPELL       = 0x20000000u,
        ACTION_ID_SPELL_CLEAR = 0xdfffffffu,
        ACTION_ID_ITEM        = 0x40000000u,
        ACTION_ID_ITEM_CLEAR  = 0xbfffffffu,
    };

    // m_nActionId of the action menus' plain entries; a target-action entry's low 28 bits pick the
    // mouse cursor. An implant entry is IMPLANT_BASE + the implant (1-4), and a behaviour entry
    // BEHAVIOR_AGGRESSIVE + its AI state less AI_STATE_AGGRESSIVE.
    enum {
        ACTION_ID_USE_OBJECT              = 1001,
        ACTION_ID_TALK                    = 1002,
        ACTION_ID_ATTACK                  = 1003,
        ACTION_ID_OPEN_DOOR               = 1010,
        ACTION_ID_SECURITY                = 1011,
        ACTION_ID_DISABLE_MINE            = 1012,
        ACTION_ID_BASH                    = 1013,
        ACTION_ID_USE_PLACEABLE           = 1015,
        ACTION_ID_STEALTH                 = 1021,
        ACTION_ID_RECOVER_MINE            = 1026,
        ACTION_ID_NONE                    = 1028,   // "No Action"
        ACTION_ID_IMPLANT_BASE            = 1031,
        ACTION_ID_DOOR_SABER              = 1031,   // reuses the implant entries' base id
        ACTION_ID_IMPLANT_REGENERATION    = 1032,
        ACTION_ID_IMPLANT_STRENGTH        = 1033,
        ACTION_ID_IMPLANT_ENDURANCE       = 1034,
        ACTION_ID_IMPLANT_ALACRITY        = 1035,
        ACTION_ID_SWITCH_WEAPONS          = 1036,
        ACTION_ID_STANCE_0                = 1037,   // sends stance 0 after arming combat mode
        ACTION_ID_STANCE_1                = 1038,   // sends stance 1
        ACTION_ID_BEHAVIOR_AGGRESSIVE     = 1040,
        ACTION_ID_BEHAVIOR_DEFENSIVE      = 1041,
        ACTION_ID_BEHAVIOR_RANGED_SUPPORT = 1042,
        ACTION_ID_BEHAVIOR_STATIONARY     = 1043,
        ACTION_ID_BEHAVIOR_SUPPORT        = 1044,
    };

    // m_nFlags: bit 0 enables the entry, bits 1-4 say why it is disabled, and Clear keeps the top
    // three bits.
    enum {
        FLAG_ENABLED     = 0x01,
        FLAG_REASON_MASK = 0x1e,
        FLAG_REASON_NOT_ENOUGH_FORCE  = 0x02,
        FLAG_REASON_PARTY_MEMBER_DEAD = 0x0c,
        FLAG_REASON_NOT_ENOUGH_HEALTH = 0x0e,
        FLAG_KEEP_MASK   = 0xe0,
    };

    CExoString   m_sName;
    unsigned int m_nActionId;
    unsigned int m_nReserved1;
    void (CSWCCreature::*m_pfnAction)(unsigned int, CSWCCreature*);
    unsigned long long m_nObjectId;
    CResRef      m_resRef;
    unsigned char m_nFlags;
    int          m_nCount;
#if !KOTOR1  // K1 has no trailing byte
    unsigned char m_bChangesState;
#endif

    // Resets the entry to its empty state.
    void Clear();

    CSWGuiInterfaceAction() { Clear(); }
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiInterfaceAction) == 0x48, "K1 CSWGuiInterfaceAction size");
#endif
