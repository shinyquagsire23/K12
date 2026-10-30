#pragma once
#include "k2layout.h"

#include "nwanimbase.h"
#include "Vector.h"
#include "exoresman.h"
#include "serveranimation.h"

class CAurObject;
class CAurScene;
class CExoString;
struct CSWGuiExtent;
class Gob;

// The five colour-shift Vectors the constructor zero-fills; read them through Color().
struct CSWCAnimBaseHiliteColors {
    Vector m_avColor[5];
    ~CSWCAnimBaseHiliteColors() {}
    Vector& Color(int which) { return m_avColor[which]; }
};

// Client-core animation base: every visible object that owns a model extends it (creatures,
// doors, placeables, triggers, projectiles, visual effects). It holds the primary CAurObject
// m_pModel, the shadow-blob decal, the wielded and head items, and the per-object animation
// state. The model slots are virtual so a derived anim base can drive a second model; most of
// them return 0 or 1 to report whether the model accepted the change.
class CSWCAnimBase : public CSWAnimBase {
public:
    // Model selectors: the `which` byte of GetModel, LoadModel, ReplaceTexture and the other
    // per-model slots. The body is the anim base's own model; the tail, wings and head are the
    // sub-objects the TW and Head bases attach; the eight below them reach the wielded items'
    // own models (right item first, each item's wings, tail, head, body).
    enum ModelSelector {
        ANIM_MODEL_RIGHT_ITEM_WINGS = 0xf4,
        ANIM_MODEL_RIGHT_ITEM_TAIL  = 0xf5,
        ANIM_MODEL_RIGHT_ITEM_HEAD  = 0xf6,
        ANIM_MODEL_RIGHT_ITEM_BODY  = 0xf7,
        ANIM_MODEL_LEFT_ITEM_WINGS  = 0xf8,
        ANIM_MODEL_LEFT_ITEM_TAIL   = 0xf9,
        ANIM_MODEL_LEFT_ITEM_HEAD   = 0xfa,
        ANIM_MODEL_LEFT_ITEM_BODY   = 0xfb,
        ANIM_MODEL_WINGS            = 0xfc,
        ANIM_MODEL_TAIL             = 0xfd,
        ANIM_MODEL_HEAD             = 0xfe,
        ANIM_MODEL_BODY             = 0xff,
    };

    // m_nAnimBaseType: which CSWCAnimBase subclass the object is (the creature's anim-base factory
    // switches on it; CSWCCreatureAppearance::GetAnimType picks one from appearance.2da's model type).
    enum AnimBaseType {
        ANIM_BASE_TYPE_PLAIN         = 0,
        ANIM_BASE_TYPE_HEAD          = 1,
        ANIM_BASE_TYPE_WIELD         = 2,
        ANIM_BASE_TYPE_HEAD_WIELD    = 3,
        ANIM_BASE_TYPE_DOOR          = 5,
        ANIM_BASE_TYPE_PROJECTILE    = 7,
        ANIM_BASE_TYPE_VISUAL_EFFECT = 8,
        ANIM_BASE_TYPE_TRIGGER       = 9,
        ANIM_BASE_TYPE_PLACEABLE     = 10,
        ANIM_BASE_TYPE_TAIL_WINGS    = 11,
    };

    // m_creatureType: which animation set the creature's model carries. A full set has the
    // humanoid per-weapon animations; a limited set (model type "L", monsters and droids) has
    // only the c-prefixed monster rows.
    enum CreatureType {
        CREATURE_TYPE_FULL    = 1,
        CREATURE_TYPE_LIMITED = 2,
    };

    // m_nWeaponClass: the wield style, which picks the per-weapon animation family (the digit in
    // the animation names: g1 stun baton, c2/g2 single saber, ..., b9 heavy carbine).
    enum WeaponClass {
        WEAPON_CLASS_STUN_BATON       = 1,
        WEAPON_CLASS_SINGLE_SABER     = 2,
        WEAPON_CLASS_TWO_HANDED_SABER = 3,
        WEAPON_CLASS_DUAL_SABER       = 4,
        WEAPON_CLASS_SINGLE_BLASTER   = 5,
        WEAPON_CLASS_DUAL_BLASTER     = 6,
        WEAPON_CLASS_RIFLE            = 7,
        WEAPON_CLASS_NATURAL          = 8,
        WEAPON_CLASS_HEAVY_CARBINE    = 9,
    };

    // Client animation ids: rows of animations.2da, whose "name" column (the trailing comment)
    // is the model animation that plays.
    enum ClientAnimation {
        CLIENT_ANIMATION_WALK                               = 0,    // walk
        CLIENT_ANIMATION_WALK_INJURED                       = 1,    // walkinj
        CLIENT_ANIMATION_RUN                                = 2,    // run
        CLIENT_ANIMATION_RUN_DUAL_SABER                     = 3,    // runds
        CLIENT_ANIMATION_RUN_INJURED                        = 4,    // runinj
        CLIENT_ANIMATION_WALK_STEALTH                       = 5,    // stealth
        CLIENT_ANIMATION_PAUSE_1                            = 6,    // pause1
        CLIENT_ANIMATION_PAUSE_2                            = 7,    // pause2
        CLIENT_ANIMATION_PAUSE_INJURED                      = 8,    // pauseinj
        CLIENT_ANIMATION_HEAD_TURN_RIGHT                    = 10,   // hturnr
        CLIENT_ANIMATION_HEAD_TURN_LEFT                     = 11,   // hturnl
        CLIENT_ANIMATION_PAUSE_SCRATCH_HEAD                 = 12,   // pausesh
        CLIENT_ANIMATION_PAUSE_BORED                        = 13,   // pausebrd
        CLIENT_ANIMATION_PAUSE_TIRED                        = 14,   // pausetrd
        CLIENT_ANIMATION_PAUSE_POISONED                     = 15,   // pausepsn
        CLIENT_ANIMATION_SALUTE                             = 16,   // salute
        CLIENT_ANIMATION_VICTORY                            = 17,   // victory
        CLIENT_ANIMATION_LISTEN                             = 18,   // listen
        CLIENT_ANIMATION_BOW                                = 19,   // bow
        CLIENT_ANIMATION_SIT_DOWN                           = 20,   // sitdown
        CLIENT_ANIMATION_STAND_UP                           = 22,   // standup
        CLIENT_ANIMATION_KNEEL                              = 23,   // kneel
        CLIENT_ANIMATION_MEDITATE                           = 24,   // meditate
        CLIENT_ANIMATION_TALK_NORMAL                        = 25,   // tlknorm
        CLIENT_ANIMATION_TALK_FORCEFUL                      = 26,   // tlkforce
        CLIENT_ANIMATION_TALK_PLEADING                      = 27,   // tlkplead
        CLIENT_ANIMATION_TALK_SAD                           = 28,   // tlksad
        CLIENT_ANIMATION_TALK_LAUGHING                      = 29,   // tlklaugh
        CLIENT_ANIMATION_GREETING                           = 31,   // greeting
        CLIENT_ANIMATION_FLIRT                              = 32,   // flirt
        CLIENT_ANIMATION_TAUNT                              = 33,   // taunt
        CLIENT_ANIMATION_TREAT_INJURED                      = 34,   // treatinj
        CLIENT_ANIMATION_TREAT_INJURED_LOOP                 = 35,   // treatinjlp
        CLIENT_ANIMATION_INJECT                             = 37,   // inject
        CLIENT_ANIMATION_ACTIVATE                           = 38,   // activate
        CLIENT_ANIMATION_GET_FROM_GROUND                    = 40,   // getfromgnd
        CLIENT_ANIMATION_GET_FROM_TABLE                     = 41,   // getfromtbl
        CLIENT_ANIMATION_USE_COMPUTER                       = 43,   // usecomp
        CLIENT_ANIMATION_USE_COMPUTER_LOOP                  = 44,   // usecomplp
        CLIENT_ANIMATION_UNLOCK_DOOR                        = 47,   // unlockdr
        CLIENT_ANIMATION_UNLOCK_CONTAINER                   = 48,   // unlockcntr
        CLIENT_ANIMATION_DISABLE_MINE                       = 51,   // disablemine
        CLIENT_ANIMATION_SET_MINE                           = 52,   // setmine
        CLIENT_ANIMATION_DANCE_1                            = 53,   // dance
        CLIENT_ANIMATION_DANCE_2                            = 54,   // dance1
        CLIENT_ANIMATION_THROW_GRENADE_1                    = 57,   // throwgren
        CLIENT_ANIMATION_THROW_GRENADE_2                    = 58,   // throwgren1
        CLIENT_ANIMATION_CAST_OUT_1                         = 62,   // castout1
        CLIENT_ANIMATION_CAST_OUT_LOOP_1                    = 63,   // castoutlp1
        CLIENT_ANIMATION_CAST_OUT_2                         = 64,   // castout2
        CLIENT_ANIMATION_CAST_OUT_LOOP_2                    = 65,   // castoutlp2
        CLIENT_ANIMATION_CAST_OUT_3                         = 66,   // castout3
        CLIENT_ANIMATION_CAST_OUT_LOOP_3                    = 67,   // castoutlp3
        CLIENT_ANIMATION_PERSUADE                           = 68,   // persuade
        CLIENT_ANIMATION_THROW_SABER                        = 69,   // throwsab
        CLIENT_ANIMATION_THROW_SABER_LOOP                   = 70,   // throwsablp
        CLIENT_ANIMATION_CATCH_SABER                        = 71,   // catchsab
        CLIENT_ANIMATION_CHOKE                              = 72,   // choke
        CLIENT_ANIMATION_HORROR                             = 74,   // horror
        CLIENT_ANIMATION_WHIRLWIND                          = 75,   // whirlwind
        CLIENT_ANIMATION_SLEEP                              = 76,   // sleep
        CLIENT_ANIMATION_SPASM                              = 77,   // spasm
        CLIENT_ANIMATION_PARALYZED                          = 78,   // paralyzed
        CLIENT_ANIMATION_PRONE                              = 79,   // prone
        CLIENT_ANIMATION_DIE_1                              = 80,   // die
        CLIENT_ANIMATION_DEAD_1                             = 81,   // dead
        CLIENT_ANIMATION_DIE_2                              = 82,   // die1
        CLIENT_ANIMATION_DEAD_2                             = 83,   // dead1
        CLIENT_ANIMATION_KNOCKED_DOWN_LOOP                  = 84,   // g1x1
        CLIENT_ANIMATION_KNOCKED_DOWN                       = 85,   // g1y1
        CLIENT_ANIMATION_STAND_FROM_KNOCKED_DOWN            = 86,   // g1z1
        CLIENT_ANIMATION_STUN_BATON_ATTACK_1                = 87,   // g1a1
        CLIENT_ANIMATION_STUN_BATON_ATTACK_2                = 88,   // g1a2
        CLIENT_ANIMATION_STUN_BATON_WIELD_DRAW              = 91,   // g1w1
        CLIENT_ANIMATION_STUN_BATON_READY                   = 92,   // g1r1
        CLIENT_ANIMATION_SINGLE_SABER_ATTACK_1              = 94,   // c2a1
        CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_1             = 109,  // c2n1
        CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_2             = 110,  // c2n2
        CLIENT_ANIMATION_SINGLE_SABER_CRITICAL_STRIKE_1     = 113,  // f2a1a
        CLIENT_ANIMATION_SINGLE_SABER_FLURRY_1              = 114,  // f2a2a
        CLIENT_ANIMATION_SINGLE_SABER_POWER_ATTACK_1        = 115,  // f2a3a
        CLIENT_ANIMATION_SINGLE_SABER_GENERIC_ATTACK_1      = 122,  // g2a1
        CLIENT_ANIMATION_SINGLE_SABER_DAMAGE                = 124,  // g2d1
        CLIENT_ANIMATION_SINGLE_SABER_ATTACK_MONSTER_1      = 125,  // m2a1
        CLIENT_ANIMATION_SINGLE_SABER_WIELD_DRAW            = 132,  // g2w1
        CLIENT_ANIMATION_SINGLE_SABER_READY                 = 133,  // g2r1
        CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_1          = 135,  // c3a1
        CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_1         = 150,  // c3n1
        CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_2         = 151,  // c3n2
        CLIENT_ANIMATION_TWO_HANDED_SABER_CRITICAL_STRIKE_1 = 154,  // f3a1a
        CLIENT_ANIMATION_TWO_HANDED_SABER_FLURRY_1          = 155,  // f3a2a
        CLIENT_ANIMATION_TWO_HANDED_SABER_POWER_ATTACK_1    = 156,  // f3a3a
        CLIENT_ANIMATION_TWO_HANDED_SABER_GENERIC_ATTACK_1  = 163,  // g3a1
        CLIENT_ANIMATION_TWO_HANDED_SABER_DAMAGE            = 165,  // g3d1
        CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_MONSTER_1  = 166,  // m3a1
        CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_DRAW        = 173,  // g3w1
        CLIENT_ANIMATION_TWO_HANDED_SABER_READY             = 174,  // g3r1
        CLIENT_ANIMATION_DUAL_SABER_ATTACK_1                = 176,  // c4a1
        CLIENT_ANIMATION_DUAL_SABER_DEFLECT_1               = 191,  // c4n1
        CLIENT_ANIMATION_DUAL_SABER_DEFLECT_2               = 192,  // c4n2
        CLIENT_ANIMATION_DUAL_SABER_CRITICAL_STRIKE_1       = 195,  // f4a1a
        CLIENT_ANIMATION_DUAL_SABER_FLURRY_1                = 196,  // f4a2a
        CLIENT_ANIMATION_DUAL_SABER_POWER_ATTACK_1          = 197,  // f4a3a
        CLIENT_ANIMATION_DUAL_SABER_GENERIC_ATTACK_1        = 204,  // g4a1
        CLIENT_ANIMATION_DUAL_SABER_DAMAGE                  = 206,  // g4d1
        CLIENT_ANIMATION_DUAL_SABER_ATTACK_MONSTER_1        = 207,  // m4a1
        CLIENT_ANIMATION_DUAL_SABER_WIELD_DRAW              = 214,  // g4w1
        CLIENT_ANIMATION_DUAL_SABER_READY                   = 215,  // g4r1
        CLIENT_ANIMATION_SINGLE_BLASTER_DAMAGE              = 220,  // g5d1
        CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_DRAW          = 222,  // g5w1
        CLIENT_ANIMATION_SINGLE_BLASTER_READY               = 223,  // g5r1
        CLIENT_ANIMATION_DUAL_BLASTER_DAMAGE                = 234,  // g6d1
        CLIENT_ANIMATION_DUAL_BLASTER_WIELD_DRAW            = 236,  // g6w1
        CLIENT_ANIMATION_DUAL_BLASTER_READY                 = 237,  // g6r1
        CLIENT_ANIMATION_RIFLE_DAMAGE                       = 242,  // g7d1
        CLIENT_ANIMATION_RIFLE_WIELD_DRAW                   = 244,  // g7w1
        CLIENT_ANIMATION_RIFLE_READY                        = 245,  // g7r1
        CLIENT_ANIMATION_NATURAL_ATTACK_1                   = 247,  // g8a1
        CLIENT_ANIMATION_NATURAL_READY                      = 249,  // g8r1
        CLIENT_ANIMATION_MONSTER_WALK                       = 253,  // cwalk
        CLIENT_ANIMATION_MONSTER_WALK_INJURED               = 254,  // cwalkinj
        CLIENT_ANIMATION_MONSTER_RUN                        = 255,  // crun
        CLIENT_ANIMATION_MONSTER_PAUSE_1                    = 256,  // cpause1
        CLIENT_ANIMATION_MONSTER_PAUSE_2                    = 257,  // cpause2
        CLIENT_ANIMATION_MONSTER_HEAD_TURN_LEFT             = 258,  // chturnl
        CLIENT_ANIMATION_MONSTER_HEAD_TURN_RIGHT            = 259,  // chturnr
        CLIENT_ANIMATION_MONSTER_VICTORY                    = 260,  // cvictory
        CLIENT_ANIMATION_MONSTER_TALK                       = 261,  // tlknorm
        CLIENT_ANIMATION_MONSTER_TAUNT                      = 263,  // ctaunt
        CLIENT_ANIMATION_MONSTER_CHOKE                      = 264,  // choke
        CLIENT_ANIMATION_MONSTER_WHIRLWIND                  = 266,  // whirlwind
        CLIENT_ANIMATION_MONSTER_SLEEP                      = 267,  // sleep
        CLIENT_ANIMATION_MONSTER_SPASM                      = 268,  // cspasm
        CLIENT_ANIMATION_MONSTER_PARALYZED                  = 269,  // paralyzed
        CLIENT_ANIMATION_DROID_DISABLED                     = 270,  // disabled
        CLIENT_ANIMATION_MONSTER_KNOCKED_BACK_LOOP          = 271,  // ckdbcklp
        CLIENT_ANIMATION_MONSTER_KNOCKED_BACK               = 272,  // ckdbck
        CLIENT_ANIMATION_MONSTER_STAND_FROM_PRONE           = 273,  // cgustandb
        CLIENT_ANIMATION_MONSTER_DIE                        = 274,  // cdie
        CLIENT_ANIMATION_MONSTER_DEAD                       = 275,  // cdead
        CLIENT_ANIMATION_MONSTER_GENERIC_ATTACK_1           = 276,  // g0a1
        CLIENT_ANIMATION_MONSTER_READY                      = 278,  // creadyr
        CLIENT_ANIMATION_MONSTER_READY_FIDGET               = 279,  // creadyrtw
        CLIENT_ANIMATION_MONSTER_DAMAGE                     = 280,  // cdamages
        CLIENT_ANIMATION_MONSTER_ATTACK_1                   = 282,  // m0a1
        CLIENT_ANIMATION_DROID_BLASTER_ATTACK_1             = 288,  // b0a1
        CLIENT_ANIMATION_DROID_BLASTER_ATTACK_2             = 289,  // b0a2
        CLIENT_ANIMATION_POWER_UP                           = 296,  // powerup
        CLIENT_ANIMATION_POWERED                            = 297,  // powered
        CLIENT_ANIMATION_POWER_DOWN                         = 298,  // powerdown
        CLIENT_ANIMATION_DEACTIVATED                        = 299,  // disabled
        CLIENT_ANIMATION_GENERIC_ATTACK                     = 300,  // attack
        CLIENT_ANIMATION_GENERIC_PARRY                      = 301,  // parry
        CLIENT_ANIMATION_GENERIC_DODGE                      = 302,  // dodge
        CLIENT_ANIMATION_GENERIC_DAMAGE                     = 303,  // damage
        CLIENT_ANIMATION_PLACEABLE_DEFAULT                  = 304,  // default
        CLIENT_ANIMATION_PLACEABLE_DAMAGE                   = 305,  // damage
        CLIENT_ANIMATION_PLACEABLE_DIE                      = 306,  // die
        CLIENT_ANIMATION_PLACEABLE_DEAD                     = 307,  // dead
        CLIENT_ANIMATION_PLACEABLE_ON                       = 308,  // on
        CLIENT_ANIMATION_PLACEABLE_OFF                      = 309,  // off
        CLIENT_ANIMATION_PLACEABLE_OPEN                     = 310,  // open
        CLIENT_ANIMATION_PLACEABLE_CLOSE                    = 311,  // close
        CLIENT_ANIMATION_PLACEABLE_CLOSE_TO_OPEN            = 312,  // close2open
        CLIENT_ANIMATION_PLACEABLE_OPEN_TO_CLOSE            = 313,  // open2close
        CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF                = 314,  // on2off
        CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON                = 315,  // off2on
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP01               = 316,  // animloop01
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP02               = 317,  // animloop02
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP03               = 318,  // animloop03
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP04               = 319,  // animloop04
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP05               = 320,  // animloop05
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP06               = 321,  // animloop06
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP07               = 322,  // animloop07
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP08               = 323,  // animloop08
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP09               = 324,  // animloop09
        CLIENT_ANIMATION_PLACEABLE_ANIMLOOP10               = 325,  // animloop10
        CLIENT_ANIMATION_DOOR_DEFAULT                       = 327,  // default
        CLIENT_ANIMATION_DOOR_DAMAGE                        = 328,  // damage
        CLIENT_ANIMATION_DOOR_DIE                           = 329,  // die
        CLIENT_ANIMATION_DOOR_DEAD                          = 330,  // dead
        CLIENT_ANIMATION_DOOR_OPENED_1                      = 331,  // opened1
        CLIENT_ANIMATION_DOOR_OPENED_2                      = 332,  // opened2
        CLIENT_ANIMATION_DOOR_CLOSED                        = 333,  // closed
        CLIENT_ANIMATION_DOOR_OPENING_1                     = 334,  // opening1
        CLIENT_ANIMATION_DOOR_OPENING_2                     = 335,  // opening2
        CLIENT_ANIMATION_DOOR_CLOSING_1                     = 336,  // closing1
        CLIENT_ANIMATION_DOOR_CLOSING_2                     = 337,  // closing2
        CLIENT_ANIMATION_WALK_SINGLE_SABER                  = 338,  // walkSS
        CLIENT_ANIMATION_WALK_DUAL_SABER                    = 339,  // walkDS
        CLIENT_ANIMATION_WALK_RIFLE                         = 340,  // walkRF
        CLIENT_ANIMATION_WALK_STUN_BATON                    = 341,  // walkST
        CLIENT_ANIMATION_RUN_RIFLE                          = 342,  // runRF
        CLIENT_ANIMATION_RUN_STUN_BATON                     = 343,  // runST
        CLIENT_ANIMATION_DOOR_TRANSITION                    = 344,  // trans
        CLIENT_ANIMATION_RUN_SINGLE_SABER                   = 345,  // runSS
        CLIENT_ANIMATION_CUSTOM01                           = 346,  // usecomp
        CLIENT_ANIMATION_TRIGGER_DEFAULT                    = 347,  // default
        CLIENT_ANIMATION_TRIGGER_ACTIVATE                   = 348,  // activate
        CLIENT_ANIMATION_TRIGGER_DEACTIVATE                 = 349,  // deactivate
        CLIENT_ANIMATION_TRIGGER_DETECT                     = 350,  // detect
        CLIENT_ANIMATION_DROID_SNIPER_SHOT                  = 351,  // b0a3
        CLIENT_ANIMATION_FORCE_BLOCK                        = 355,  // fblock
        CLIENT_ANIMATION_OFF                                = 356,  // off
        CLIENT_ANIMATION_PAUSE_FIDGET                       = 357,  // pause2
        CLIENT_ANIMATION_MONSTER_PAUSE_FIDGET               = 358,  // cpause2
        CLIENT_ANIMATION_PAUSE_3                            = 359,  // pause3
        CLIENT_ANIMATION_WELD                               = 360,  // weld
        CLIENT_ANIMATION_DROID_POWER_BLAST                  = 361,  // b0a4
        CLIENT_ANIMATION_DOOR_BUSTED                        = 366,  // busted
        CLIENT_ANIMATION_TURN_LEFT_2                        = 367,  // turnleft
        CLIENT_ANIMATION_TURN_RIGHT_2                       = 368,  // turnright
        CLIENT_ANIMATION_MONSTER_WELD                       = 369,  // weld
        CLIENT_ANIMATION_TALK_INJURED                       = 370,  // talkinj
        CLIENT_ANIMATION_LISTEN_INJURED                     = 371,  // listeninj
        CLIENT_ANIMATION_MONSTER_TALK_INJURED               = 372,  // talkinj
        CLIENT_ANIMATION_NATURAL_WIELD_DRAW                 = 373,  // equip
        CLIENT_ANIMATION_DIE_3                              = 374,  // die3
        CLIENT_ANIMATION_DEAD_PRONE                         = 375,  // dead3
        CLIENT_ANIMATION_DUAL_SABER_WIELD_SWAP              = 376,  // G4F1
        CLIENT_ANIMATION_SINGLE_SABER_WIELD_SWAP            = 377,  // G2F1
        CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_SWAP        = 378,  // G3F1
        CLIENT_ANIMATION_MONSTER_CONJURE                    = 379,  // g0a1
        CLIENT_ANIMATION_MONSTER_CONJURE_3                  = 380,  // b0a1
        CLIENT_ANIMATION_GET_UP_FROM_DEAD_1                 = 381,  // getupdead
        CLIENT_ANIMATION_GET_UP_FROM_DEAD_2                 = 382,  // getupdead1
        CLIENT_ANIMATION_KNEEL_DOWN                         = 383,  // kd
        CLIENT_ANIMATION_KNEEL_TALK_ANGRY                   = 384,  // kdtlkangry
        CLIENT_ANIMATION_KNEEL_TALK_SAD                     = 385,  // kdtlksad
        CLIENT_ANIMATION_SINGLE_SABER_FORCE_JUMP            = 386,  // f2a4
        CLIENT_ANIMATION_SINGLE_SABER_BURN_DOOR             = 389,  // c2a6
        CLIENT_ANIMATION_HAND_DEFLECT                       = 455,  // deflect
        CLIENT_ANIMATION_SIT_TO_MEDITATE                    = 456,  // sit
        CLIENT_ANIMATION_MEDITATE_SITTING                   = 457,  // meditatesit
        CLIENT_ANIMATION_MEDITATE_STANDING                  = 458,  // meditatestand
        CLIENT_ANIMATION_FORCE_CRUSH                        = 459,  // forcecrush
        CLIENT_ANIMATION_FORCE_CRUSH_DAMAGE                 = 460,  // crushdamage
        CLIENT_ANIMATION_FORCE_RAGE                         = 461,  // forcerage
        CLIENT_ANIMATION_TOUCH_HEART                        = 462,  // touchheart
        CLIENT_ANIMATION_ROLL_EYES                          = 463,  // rolleyes
        CLIENT_ANIMATION_USE_ITEM_ON_OTHER                  = 464,  // itemequip
        CLIENT_ANIMATION_STAND_AT_ATTENTION                 = 465,  // standstill
        CLIENT_ANIMATION_NOD_YES                            = 466,  // nodyes
        CLIENT_ANIMATION_NOD_NO                             = 467,  // nodno
        CLIENT_ANIMATION_POINT                              = 468,  // point
        CLIENT_ANIMATION_POINT_LOOP                         = 469,  // pointloop
        CLIENT_ANIMATION_POINT_DOWN                         = 470,  // pointdown
        CLIENT_ANIMATION_SCANNING                           = 471,  // scanning
        CLIENT_ANIMATION_SHRUG                              = 472,  // shrug
        CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_SWAP          = 473,  // g5f1
        CLIENT_ANIMATION_DUAL_BLASTER_WIELD_SWAP            = 474,  // g6f1
        CLIENT_ANIMATION_RIFLE_WIELD_SWAP                   = 475,  // g7f1
        CLIENT_ANIMATION_HEAVY_CARBINE_WIELD_SWAP           = 476,  // g9f1
        CLIENT_ANIMATION_UNARMED_ATTACK_1                   = 477,  // c10a1
        CLIENT_ANIMATION_UNARMED_DEFLECT_1                  = 492,  // c10n1
        CLIENT_ANIMATION_UNARMED_DEFLECT_2                  = 493,  // c10n2
        CLIENT_ANIMATION_UNARMED_DAMAGE                     = 526,  // g10d1
        CLIENT_ANIMATION_UNARMED_ATTACK_MONSTER_1           = 527,  // m10a1
        CLIENT_ANIMATION_UNARMED_WIELD_DRAW                 = 534,  // g10w1
        CLIENT_ANIMATION_UNARMED_READY                      = 535,  // g10r1
        CLIENT_ANIMATION_WRIST_LAUNCH                       = 536,  // b11a3
        CLIENT_ANIMATION_CUT_HAND                           = 557,  // cuthand
        CLIENT_ANIMATION_REACT_HAND_CUT_OFF                 = 558,  // lhandchop
        CLIENT_ANIMATION_COLLAPSE                           = 559,  // Collapse
        CLIENT_ANIMATION_COLLAPSE_LOOP                      = 560,  // Collapselp
        CLIENT_ANIMATION_COLLAPSE_STAND                     = 561,  // Collapsestand
        CLIENT_ANIMATION_POWER_PUNCH                        = 562,  // powerpunch
        CLIENT_ANIMATION_RAISE_ARM_TO_POINT                 = 563,  // uppoint
        CLIENT_ANIMATION_LOWER_ARM_FROM_POINT               = 564,  // downpoint
        CLIENT_ANIMATION_LOWER_HOOD                         = 565,  // offhood
        CLIENT_ANIMATION_RAISE_HOOD                         = 566,  // onhood
        CLIENT_ANIMATION_DIVE_ROLL                          = 567,  // diveroll
        CLIENT_ANIMATION_TALK_PRONE                         = 568,  // talkprone
        CLIENT_ANIMATION_TALK_DIE_PRONE                     = 569,  // talkpronedie
#if KOTOR1  // K1's table has the droid utility rows where K2 has the critical-strike variants
        CLIENT_ANIMATION_DROID_UTILITY_CONJURE              = 392,  // castout1
        CLIENT_ANIMATION_DROID_UTILITY_CAST                 = 393,  // castoutlp1
#else
        CLIENT_ANIMATION_DROID_UTILITY_CONJURE              = 537,  // castout1
        CLIENT_ANIMATION_DROID_UTILITY_CAST                 = 538,  // castoutlp1
#endif
    };

    CSWCAnimBase();
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 63 slots
    virtual ~CSWCAnimBase();
    virtual void AttachToScene(CAurScene*);
    virtual Gob* GetModel(unsigned char which);
    virtual int LoadModel(CResRef const&, unsigned char);
    virtual int ReplaceTexture(unsigned char, CResRef const&, CResRef const&, int);
    virtual int ReplaceTexture(CResRef const&, CResRef const&, int);
    virtual int ReplaceTexture(unsigned char, CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);
    virtual int ReplaceTexture(CResRef const&, CResRef const&, unsigned int, unsigned short*, int, unsigned int);
    virtual int RestoreTexture();
    virtual int RestoreTexture(unsigned char);
    virtual int EnableBumpedOutShell(CResRef const&, float);
    virtual int DisableBumpedOutShell();
    virtual int SetMotionBlur(int);
    virtual bool SetMaxMotionBlurLength(float length);
    virtual float GetAnimationLength(unsigned short);
    virtual float GetAnimationTime(unsigned short);
    virtual CExoString GetAnimationName(unsigned short);
    virtual int SetOverlayAnimation(unsigned short, int);
    int SetOverlayAnimation(CAurObject*, unsigned short, int);  // shared body; the vtable overload passes m_pModel
    virtual int GetOverlayAnimation(unsigned short);
    virtual int GetOverlayAnimationPlaying(unsigned short);
    virtual int AnimationExists(unsigned short);
    virtual int SetAnimation(unsigned short, float, int, float);
    virtual int SetAnimationOnSubObject(unsigned char, unsigned short, float, int, float);
    virtual int SetAnimationVisemes(float*, float*, int, float);
    virtual int SetOrientation(Vector const&);
    virtual int SetOrientation(Quaternion const&);
    virtual int SetPosition(Vector const&);
    virtual void SetAlwaysRender(int);
    virtual void SetInCutscene(int);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char);
    virtual int Wield(unsigned long long, CExoString const&, unsigned char, float);
    virtual void HideWieldedItems(CResRef const&, CResRef const&);
    virtual void ShowWieldedItems();
    virtual void HideHeadItem();
    virtual void ShowHeadItem();
    virtual void SetWieldLimited(int);
    virtual int SetAlpha(float, unsigned char);
    virtual int SetEnvMap(int, int, CResRef const&);
    virtual int SetEnvMap(unsigned char, int, int, CResRef const&);
    virtual int RestoreEnvMap();
    virtual int ApplyShadowBlob(CExoString const&);
    virtual void RemoveShadowBlob();
    virtual void ShowShadowBlob(int);
    virtual void HideShadowBlob();
    virtual void SetShadowBlobOrientation(Quaternion const&);
    virtual void TurnOnShadows();
    virtual void TurnOffShadows();
    virtual Quaternion GetOrientation();
    virtual void UpdateSound(int);
    virtual int AddAnimationFlag(int);
    virtual int RemoveAnimationFlag(int);
    virtual void SetLean(float);
    virtual void Lean();
    virtual int GetBoundingRect(CSWGuiExtent&);
    virtual char GetCenterPoint(Vector&);
    virtual unsigned short ServerToClientAnimation(unsigned short);
    virtual void EnableDistortion();
    virtual void DisableDistortion();
    virtual void SetHeadItem(unsigned long long);
    virtual int SetAnimationInternal(CAurObject*, unsigned short, float, int, float);
    virtual int SetAnimationOnSubObjectInternal(CAurObject*, unsigned char, unsigned short, float, int, float);
    // </VTABLE-GEN>

    int AddAnimationFlagInternal(CAurObject*, int);
    int RemoveAnimationFlagInternal(CAurObject*, int);

    void SetGroundTilt(Vector const& vTilt, Vector const& vFacing);
    int GetBoundingRect(CAurObject*, CSWGuiExtent&);  // shared body; the vtable overload passes m_pModel
    void RefreshHilite();
    int Hilite(const Vector& v, unsigned char which, int b);
    int UnHilite(unsigned char which);
    Vector GetCutsceneDummyPosition();

    unsigned short m_anModelTextureData[10];
    unsigned char m_nModelTextureCount;
    unsigned char m_nModelTextureOffset;
    char          _pad2e[0x30 - 0x2e];
    CResRef       m_resModelTexture;
    int           m_bModelTextureDataCached;
    CResRef       m_resModelEnvMap;
    int           m_bModelEnvMapEnabled;
    CSWCAnimBaseHiliteColors m_hiliteColors;
    int           m_anHiliteSlot[10];
    CAurObject*   m_pModel;
    CAurObject*   m_pShadowBlob;
    float         m_alpha;
    unsigned char m_nAnimBaseType;
    unsigned char m_nWeaponClass;
    unsigned char m_creatureType;
    unsigned char m_nWeaponType;
    unsigned char m_specialWalkAnimation;
    char          _padd9[0xdc - 0xd9];
    int           m_runLooksAwful;
#if !KOTOR1  // K1 has no lightsaber flag, feat latch or stance mirror
    unsigned char m_bLightsaber;
    unsigned char _pade1[0xe4 - 0xe1];
    int           m_bHasUnarmedFeat;
    int           m_stanceId;
#endif
    int           m_nDestroyedAnimVariant;
    int           m_bShadowBlobVisible;
    float         m_fLean;
    int           m_nPoweredHandMask;
    Quaternion    m_quatFc;
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWCAnimBase) == 0x100, "K1 CSWCAnimBase");
#endif

