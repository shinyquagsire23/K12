#pragma once

#include "k2stdint.h"
#include "exostring.h"

class CResGFF;
struct CResStruct;

// The state an EFFECT_SETSTATE effect carries in integer 0, which CSWSCreature::m_nState mirrors
// (NONE when no state holds the creature) and an attack's feedback effect type reports.
enum {
    EFFECT_STATE_NONE           = 0,
    EFFECT_STATE_CONFUSED       = 1,
    EFFECT_STATE_FRIGHTENED     = 2,
    EFFECT_STATE_DROID_STUN     = 3,
    EFFECT_STATE_STUNNED        = 4,
    EFFECT_STATE_PARALYZED      = 5,
    EFFECT_STATE_SLEEP          = 6,
    EFFECT_STATE_CHOKE          = 7,
    EFFECT_STATE_HORRIFIED      = 8,
    EFFECT_STATE_WHIRLWIND      = 10,
    EFFECT_STATE_PSYCHIC_STATIC = 11,
    EFFECT_STATE_KNOCKBACK      = 13,
    EFFECT_STATE_KNOCKDOWN      = 14,
    EFFECT_STATE_FORCE_CRUSH    = 15,
    EFFECT_STATE_DROID_CONFUSED = 16,
    EFFECT_STATE_MIND_TRICK     = 18,
    EFFECT_STATE_DROID_SCRAMBLE = 19,
};

// Damage-type bits past nwscript.nss's DAMAGE_TYPE_* list: poison, and a weapon's base damage.
enum {
    DAMAGE_TYPE_POISON = 0x2000,
    DAMAGE_TYPE_BASE   = 0x4000,
};

// One applied game effect (damage, death, resurrection, ...): a sub-type, a duration type and a
// parameter block. Only the fields the decoded code touches are modelled. The sub-type and the
// duration-type bits are labelled as the engine's read sites treat them, not as the GFF keys spell
// them; do not rename either by bare identifier.
class CGameEffect {
public:
    uint64_ll_t    m_effectId;
    unsigned short m_subType;
    unsigned short m_durationTypeBits;
    float          m_fDuration;
    unsigned int   m_expireDay;
    unsigned int   m_expireTime;
    unsigned long long m_creatorId;
    int            m_spellId;
    int            m_bIsExposed;
    int            m_nCasterLevel;
    CGameEffect*   m_pLinkedEffect1;
    CGameEffect*   m_pLinkedEffect2;

    // The integer parameters are variable-length, a heap array of m_numIntegers ints, unlike the
    // three fixed slot runs below it.
    int            m_numIntegers;
    int*           m_pIntegers;
    float          m_fParams[4];
    CExoString     m_sParams[6];
    unsigned long long m_objectIds[4];
    int            m_bSkipOnLoad;

    static uint64_ll_t m_nNextID;

    static const unsigned short DURATION_TYPE_INSTANTANEOUS;
    static const unsigned short DURATION_TYPE_TEMPORARY;
    static const unsigned short DURATION_TYPE_PERMANENT;
    static const unsigned short DURATION_TYPE_INNATE;
    static const unsigned short DURATION_TYPE_EQUIPPED;

    // m_durationTypeBits holds the duration type in bits 0-2 and the effect's SUBTYPE_* (magical,
    // supernatural or extraordinary, nwscriptconst.h) in bits 3-4. Each clear mask keeps every bit
    // but the field it clears (SUPERNATURAL: SUBTYPE_SUPERNATURAL's bit).
    enum {
        DURATION_TYPE_MASK             = 0x7,
        SUBTYPE_MASK                   = 0x18,
        DURATION_TYPE_AND_SUBTYPE_MASK = 0x1f,
        DURATION_TYPE_CLEAR_MASK       = 0xfff8,
        SUBTYPE_CLEAR_MASK             = 0xffe7,
        SUPERNATURAL_CLEAR_MASK        = 0xffef,
        DURATION_SUBTYPE_CLEAR_MASK    = 0xffe0,
    };

    CGameEffect(int type);
    CGameEffect(CGameEffect* pOther);
    // Non-virtual, and must stay declared, since leaving it out lets a trivial destructor be
    // synthesized and drops the call real destruction needs.
    ~CGameEffect();
    void SetCreator(unsigned long long id);
    void SetNumIntegers(int n);
    int  GetInteger(int nIndex);
    void SetInteger(int nIndex, int nValue);
    float GetFloat(int nIndex);
    void SetFloat(int nIndex, float fValue);
    unsigned long long GetObjectID(int nIndex);
    void SetObjectID(int nIndex, unsigned long long objectId);
    CExoString GetString(int nIndex);
    void SetString(int nIndex, const CExoString& sValue);
    void SetExpiryTime(unsigned int nDay, unsigned int nTime);
    void GetExpiryTime(unsigned int* pnDay, unsigned int* pnTime);
    void CopyEffect(CGameEffect* pSource, int b);
    void SetLinked(CGameEffect* pLeft, CGameEffect* pRight);
    CGameEffect& operator=(const CGameEffect& other);
    int  operator==(const CGameEffect& other) const;
    int  operator!=(const CGameEffect& other) const;
    void LoadGameEffect(CResGFF* pGff, CResStruct* pStruct);
    void SaveGameEffect(CResGFF* pGff, CResStruct* pStruct);
    int  GetEffectTypeScriptConstant();
    void UpdateLinked();
};
