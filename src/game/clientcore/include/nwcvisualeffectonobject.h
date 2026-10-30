#pragma once

#include "Vector.h"
#include "exostring.h"
#include "k2layout.h"

class CAurObject;
class CSWCObject;
class CExoSoundSource;

// A visual effect attached to an object. Holds up to three concurrent effect slots, each a
// (CAurObject*, id, duration) triple, plus a light or model attachment.
class CSWCVisualEffectOnObject {
public:
    // Hard-coded ("programmatic") effect ids, the values of visualeffects.2da's ProgFX_Impact,
    // ProgFX_Duration and ProgFX_Cessation columns. Each handler owns a band of PROGFX_BAND_SIZE ids
    // starting at its base; PROGFX_NONE marks an empty effect slot.
    enum {
        PROGFX_BAND_SIZE               = 100,
        PROGFX_ENVIRONMENT_MAP         = 100,   // vdu_envmapNNN, NNN = id - base
        PROGFX_SELF_ILLUMINATION       = 200,
        PROGFX_LIGHT_SOURCE            = 300,
        PROGFX_ALPHA                   = 400,
        PROGFX_SELF_ILLUMINATION_PULSE = 500,
        PROGFX_BEAM                    = 600,
        PROGFX_DELETE_MODEL            = 700,
        PROGFX_CHUNK                   = 800,
        PROGFX_MIRV                    = 900,
        PROGFX_TEXTURE_REPLACE         = 1000,
        PROGFX_CHEAT                   = 1100,
        PROGFX_SPELL_VISUAL            = 1200,
        PROGFX_LIGHTSABER_THROW        = 1300,
        PROGFX_BUMPED_OUT_SHELL        = 1400,
        PROGFX_FOV                     = 1500,
        PROGFX_MOTION_BLUR             = 1600,
        PROGFX_MODEL_ATTACHMENT        = 1700,
        PROGFX_DISTORTION              = 1800,
        PROGFX_NONE                    = 0xffff,
    };

    // The individual ids of the bands whose handlers test single ids.
    enum {
        // Light sources: the 33 lights (white/blue/yellow/red/orange/purple/green at 5, 10, 15 and
        // 20 m, then the vision modes), and the same 33 again PROGFX_LIGHT_BACKWARDS ids later,
        // which play the light's animation backwards.
        PROGFX_LIGHT_WHITE_5M          = 300,
        PROGFX_LIGHT_WHITE_10M         = 301,
        PROGFX_LIGHT_WHITE_15M         = 302,
        PROGFX_LIGHT_WHITE_20M         = 303,
        PROGFX_LIGHT_BLUE_5M           = 304,
        PROGFX_LIGHT_BLUE_10M          = 305,
        PROGFX_LIGHT_BLUE_15M          = 306,
        PROGFX_LIGHT_BLUE_20M          = 307,
        PROGFX_LIGHT_YELLOW_5M         = 308,
        PROGFX_LIGHT_YELLOW_10M        = 309,
        PROGFX_LIGHT_YELLOW_15M        = 310,
        PROGFX_LIGHT_YELLOW_20M        = 311,
        PROGFX_LIGHT_RED_5M            = 312,
        PROGFX_LIGHT_RED_10M           = 313,
        PROGFX_LIGHT_RED_15M           = 314,
        PROGFX_LIGHT_RED_20M           = 315,
        PROGFX_LIGHT_ORANGE_5M         = 316,
        PROGFX_LIGHT_ORANGE_10M        = 317,
        PROGFX_LIGHT_ORANGE_15M        = 318,
        PROGFX_LIGHT_ORANGE_20M        = 319,
        PROGFX_LIGHT_PURPLE_5M         = 320,
        PROGFX_LIGHT_PURPLE_10M        = 321,
        PROGFX_LIGHT_PURPLE_15M        = 322,
        PROGFX_LIGHT_PURPLE_20M        = 323,
        PROGFX_LIGHT_GREEN_5M          = 324,
        PROGFX_LIGHT_GREEN_10M         = 325,
        PROGFX_LIGHT_GREEN_15M         = 326,
        PROGFX_LIGHT_GREEN_20M         = 327,
        PROGFX_LIGHT_DARK_VISION       = 328,
        PROGFX_LIGHT_BLIND_VISION      = 329,
        PROGFX_LIGHT_LOW_LIGHT_VISION  = 330,
        PROGFX_LIGHT_ULTRA_VISION      = 331,
        PROGFX_LIGHT_DARKNESS          = 332,
        PROGFX_LIGHT_PLAYER_BLIND_VISION = 333,   // also re-points the scene's blind lights
        PROGFX_LIGHT_BACKWARDS         = 50,

        // Alpha: half alpha, a six-second fade out and back in, half alpha with a blue or a purple
        // tint, and a one-second pulse between 0.3 and 0.8.
        PROGFX_ALPHA_HALF              = 400,
        PROGFX_ALPHA_FADE              = 401,
        PROGFX_ALPHA_HALF_BLUE_TINT    = 402,
        PROGFX_ALPHA_HALF_PURPLE_TINT  = 403,
        PROGFX_ALPHA_PULSE             = 404,

        PROGFX_SELF_ILLUMINATION_PULSE_RED  = 500,
        PROGFX_SELF_ILLUMINATION_PULSE_BLUE = 501,

        // Beams, named for the visualeffects.2da rows that use them.
        PROGFX_BEAM_LIGHTNING_DARK_S   = 608,
        PROGFX_BEAM_LIGHTNING_DARK_L   = 609,
        PROGFX_BEAM_DROID_DISABLE      = 610,
        PROGFX_BEAM_DROID_DESTROY      = 611,
        PROGFX_BEAM_DEATH_FIELD        = 612,
        PROGFX_BEAM_DRAIN_LIFE         = 613,
        PROGFX_BEAM_FLAME_SPRAY        = 614,
        PROGFX_BEAM_STUN_RAY           = 615,
        PROGFX_BEAM_COLD_RAY           = 616,
        PROGFX_BEAM_ION_RAY_01         = 617,
        PROGFX_BEAM_ION_RAY_02         = 618,
        PROGFX_BEAM_FORCE_STORM_ARC    = 619,
        PROGFX_BEAM_DROID_STUN         = 620,
        PROGFX_BEAM_SHOCK              = 621,
        PROGFX_BEAM_POISON_SPRAY       = 622,
        PROGFX_BEAM_PLASMA_SPRAY       = 623,

        PROGFX_MIRV_MAGIC_MISSILE      = 900,   // vpr_magmisl
        PROGFX_MIRV_FIRE_ARROW         = 901,   // vpr_arofire
        PROGFX_MIRV_ACID_ARROW         = 902,   // vpr_aroacid

        PROGFX_CHEAT_KNGPOW            = 1100,  // vpr_kngpow

        // Spell visuals: the force fizzle at the head or the hand, and the force-resisted flash.
        PROGFX_SPELL_FIZZLE_HEAD       = 1200,
        PROGFX_SPELL_FIZZLE_HAND       = 1201,
        PROGFX_SPELL_RESISTED          = 1202,

        PROGFX_FOV_WIDE_ANGLE          = 1500,

        // Model attachments: the medal, Revan's two masks and (K2) the breath mist.
        PROGFX_ATTACH_MEDAL            = 1700,
        PROGFX_ATTACH_REVAN_MASK_1     = 1701,
        PROGFX_ATTACH_REVAN_MASK_2     = 1702,
        PROGFX_ATTACH_BREATH           = 1703,
    };

    // m_nState and m_aSlotId: which of the effect's three stages is running, the stages of the
    // ProgFX_Impact/_Duration/_Cessation ids and of the slot models' "impact", "duration" and
    // "cessation" animations.
    enum {
        STAGE_NONE      = 0,
        STAGE_IMPACT    = 1,
        STAGE_DURATION  = 2,
        STAGE_CESSATION = 3,
    };

    // GetHardCodedEffectClass: the effects of one class compete for the same object state, and only
    // the highest-priority one of a class is attached (CSWCObject::ActivateHardCodedVisualWithHighestPriority).
    enum {
        HARDCODED_CLASS_NONE            = 0,
        HARDCODED_CLASS_TEXTURE_REPLACE = 1,
        HARDCODED_CLASS_ENVIRONMENT_MAP = 2,
        HARDCODED_CLASS_LIGHT_SOURCE    = 4,
        HARDCODED_CLASS_BEAM            = 7,
        HARDCODED_CLASS_DELETE_MODEL    = 8,
        HARDCODED_CLASS_CHUNK           = 9,
        HARDCODED_CLASS_MIRV            = 10,
        HARDCODED_CLASS_CHEAT           = 11,
        HARDCODED_CLASS_SPELL_VISUAL    = 12,
        HARDCODED_CLASS_TINT_ALPHA      = 13,   // self-illumination, alpha and the pulse
        HARDCODED_CLASS_THROW_SHELL_FOV = 15,   // lightsaber throw, bumped-out shell and FOV
    };

    Vector         m_vSelfIllum;
    Vector         m_vSelfIllumTarget;
    Vector         m_vSelfIllumBlend;
    CExoString     m_sSourceNode;
    CExoString     m_sTargetNode;
    Vector         m_vTargetPosition;
    unsigned long long m_gameObjectId;
    unsigned long long m_sourceObjectId;
    unsigned long long m_ownerObjectId;
    int            m_anSlotTimer[3];
    unsigned short m_aSlotId[3];
    char           _pad082[2];
    int            m_aReserved1[3];
    short          m_aReserved2[3];
    CAurObject*    m_apSlotEffect[3];
    CAurObject*    m_apReserved1[3];
    CAurObject*    m_pLight;
    int            m_nEffectDuration0;
    int            m_nEffectDuration1;
    int            m_nEffectDuration2;
    unsigned int   m_nEffectTimer;
    unsigned short m_nEffectId0;
    unsigned short m_nEffectId1;
    unsigned short m_nEffectId2;
    int            m_bShown;
    int            m_bEffect0Expired;
    int            m_bEffect1Expired;
    int            m_bEffect2Expired;
    int            m_bExpiring;
    int            m_bStopped;
    int            m_bFinished;
    unsigned short m_effectId;
    unsigned char  m_nState;
    unsigned char  m_nPhase;
    unsigned char  m_bHasDuration;
    char           _pad109[0x10c-0x109];
    float          m_fAlpha;
    float          m_fFixedAlpha;
    int            m_bCastShadows;
    unsigned char  m_nPriority;
    unsigned char  m_hardCodedEffectClass;
    char           _pad11a[0x11c-0x11a];
    int            m_bDarkOnly;
    int            m_bAttached;
    float          m_fSavedViewAngle;
    float          m_fSavedCameraDistance;
    CExoSoundSource* m_pSoundSource;
    int            m_bRumbleDone;
    int            m_bHardCodedApplied;
#if !KOTOR1
    // K1 has no mask hook, so this field does not exist there.
    unsigned char  m_bOnHeadModel;
#endif

    static unsigned short m_nMotionBlurVFXCount;

    CSWCVisualEffectOnObject();
    ~CSWCVisualEffectOnObject();
    CSWCObject* GetTargetCSWCObject();
    CSWCObject* GetSourceCSWCObject();
    CSWCObject* GetOwnerCSWCObject();
    int  LoadVisualEffect(unsigned short a, int b, unsigned long long objId,
                          unsigned long long c, unsigned long long d,
                          unsigned char e, unsigned char f, Vector v);
    void StartVisualEffect();
    void UnloadVisualEffect();
    void UnloadHardCodedVisualEffect();
    void KillVisualEffect();
    void StopVisualEffect(int a);
    void DetachVisualEffect();
    void ReAttachVisualEffect();
    void HideVisualEffect();
    int  ShowVisualEffect(int a);
    void Update(unsigned int a);
    void UpdateAlpha(unsigned int a);
    void UpdateFOV(unsigned short a, unsigned int b);
    void UpdateMotionBlur(unsigned int a);
    void UpdateSelfIlluminationPulse(unsigned int a);
    void UpdateHardCodedVisualEffect(unsigned short a, unsigned int b);
    void ApplyHardCodedVisualEffect(unsigned short a);
    void ApplyEnvironmentMap(unsigned short a);
    void ApplySelfIllumination(unsigned short a);
    void ApplyLightSource(unsigned short a);
    void ApplyAlpha(unsigned short a);
    void ApplySelfIlluminationPulse(unsigned short a);
    void ApplyBeam(unsigned short a);
    void ApplyDeleteModel(unsigned short a);
    void ApplyChunk(unsigned short a);
    void ApplyMirv(unsigned short a);
    void ApplyTextureReplace(unsigned short a);
    void ApplyCheat(unsigned short a);
    void ApplySpellVisual(unsigned short a);
    void ApplyLightsaberThrow(unsigned short a);
    void ApplyBumpedOutShell(unsigned short a);
    void AttachVFXModelToTargetModel(CAurObject* pTarget, CAurObject* pModel,
                                     const CExoString& sPart);
    void ApplyFOV(unsigned short a);
    void ApplyMotionBlur(unsigned short a);
    void ApplyModelAttachment(unsigned short a);
    void ApplyDistortion(unsigned short a);
    int  LoadHardCodedVisualEffect(unsigned short a);
    void RemoveHardCodedVisualEffect(unsigned short a);
    int  IsDone();
    unsigned char GetHardCodedEffectClass();
    unsigned char GetHardCodedEffectClass(unsigned short nId);
    int  LoadAlpha(unsigned short animId);
    int  LoadBeam(unsigned short a);
    int  LoadLightSource(unsigned short a);
    int  LoadModelAttachment(unsigned short a);
    int  LoadSpellVisual(unsigned short a);
    void AttachBeam();
    void AttachLight();
    void RemoveAlpha(unsigned short a);
    void RemoveBeam(unsigned short a);
    void RemoveBumpedOutShell();
    void RemoveDistortion(unsigned short a);
    void RemoveEnvironmentMap(unsigned short a);
    void RemoveFOV(unsigned short a, int b);
    void RemoveLightSource(unsigned short a);
    void RemoveModelAttachment(unsigned short a);
    void RemoveMotionBlur();
    void RemoveSelfIllumination(unsigned short a);
    void RemoveSelfIlluminationPulse(unsigned short a);
    void RemoveTextureReplace(unsigned short a);
    void RestoreObjectPosition();
    void SetEffectDone(CAurObject* pEffect);
    void EnableHardCodedEffectShadow(int b);
    int  GetHardCodedCessationLength(unsigned short nType);
    int  GetHardCodedImpactLength(unsigned short nType);
    void DoVFXRumble();
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWCVisualEffectOnObject) == 0x140, "CSWCVisualEffectOnObject K1 size drift");
#endif
