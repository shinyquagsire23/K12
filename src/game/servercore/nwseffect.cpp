#include "k2stdint.h"
#include "nwseffect.h"
#include "nwscriptconst.h"

#include "appmanager.h"
#include "serverexoapp.h"
#include "nwsobject.h"
#include "nwsspellimpact.h"

#include "resgff.h"
#include "nwseffectlist.h"

const unsigned short CGameEffect::DURATION_TYPE_INSTANTANEOUS = 0;
const unsigned short CGameEffect::DURATION_TYPE_EQUIPPED = 3;
const unsigned short CGameEffect::DURATION_TYPE_TEMPORARY = 1;
const unsigned short CGameEffect::DURATION_TYPE_PERMANENT = 2;
const unsigned short CGameEffect::DURATION_TYPE_INNATE = 4;
unsigned long long CGameEffect::m_nNextID = 1ULL;

#include "port/libcstring.h"

// Resolves the creator id through the server app's object table and, when the creator is an
// area-of-effect object, re-points the effect at the AoE's own creator.
void CGameEffect::SetCreator(unsigned long long oidCreator) {
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidCreator);
    m_creatorId = oidCreator;
    if (pObject && pObject->AsSWSObject()) {
        m_spellId = pObject->AsSWSObject()->GetEffectSpellId();
        if (pObject->AsSWSAreaOfEffectObject()) {
            CSWSAreaOfEffectObject* pAreaOfEffect = pObject->AsSWSAreaOfEffectObject();
            (void)pAreaOfEffect;
            m_creatorId = pObject->AsSWSAreaOfEffectObject()->m_oidCreator;
        }
    }
}

// The argument doubles as m_bIsExposed and as the "give me a real id" flag. Only an exposed effect
// draws from the global counter; an internal one keeps id 0, the invalid object id.
CGameEffect::CGameEffect(int bIsExposed)
{
    if (bIsExposed == 1)
        m_effectId = m_nNextID++;
    else
        m_effectId = 0;
    m_subType = 0;
    m_durationTypeBits = 0;
    m_fDuration = 0.0f;
    m_expireDay = 0;
    m_expireTime = 0;
    m_creatorId = OBJECT_INVALID;
    m_bSkipOnLoad = 0;
    m_nCasterLevel = -1;
    m_pIntegers = 0;
    m_pLinkedEffect1 = 0;
    m_pLinkedEffect2 = 0;
    m_spellId = -1;
    m_bIsExposed = bIsExposed;
    m_numIntegers = 8;
    // Value-initialised, so the zero-fill is a compiler-synthesised memset.
    m_pIntegers = new int[8]();
    for (int i = 0; i < 4; i++)
        m_fParams[i] = 0.0f;
    for (int i = 0; i < 6; i++)
        m_sParams[i] = "";
    for (int i = 0; i < 4; i++)
        m_objectIds[i] = OBJECT_INVALID;
}

// The clone constructor does not delegate to the default constructor and it is not CopyEffect. The
// default-init prologue is written out again and only the header scalars are lifted off the source,
// so m_subType, m_bIsExposed and m_nCasterLevel stay as the prologue left them. The
// effect-application hooks that use it overwrite m_subType and the integers themselves right
// afterwards.
CGameEffect::CGameEffect(CGameEffect* pOther)
{
    m_effectId = 0;
    m_subType = 0;
    m_durationTypeBits = 0;
    m_fDuration = 0.0f;
    m_expireDay = 0;
    m_expireTime = 0;
    m_creatorId = OBJECT_INVALID;
    m_pIntegers = 0;
    m_pLinkedEffect1 = 0;
    m_pLinkedEffect2 = 0;
    m_spellId = -1;
    m_bIsExposed = 0;
    m_nCasterLevel = 0;
    SetNumIntegers(pOther->m_numIntegers);
    for (int i = 0; i < 4; i++)
        m_fParams[i] = 0.0f;
    for (int i = 0; i < 6; i++)
        m_sParams[i] = "";
    for (int i = 0; i < 4; i++)
        m_objectIds[i] = OBJECT_INVALID;

    m_effectId = pOther->m_effectId;
    m_durationTypeBits = pOther->m_durationTypeBits;
    m_fDuration = pOther->m_fDuration;
    m_expireDay = pOther->m_expireDay;
    m_expireTime = pOther->m_expireTime;
    m_creatorId = pOther->m_creatorId;
    m_spellId = pOther->m_spellId;
    m_bSkipOnLoad = 0;
}

// Tears down the two linked children, through a plain recursive call rather than a virtual delete,
// and the heap integer block; the six string parameters are destroyed implicitly, in reverse index
// order.
CGameEffect::~CGameEffect() {
    if (m_pLinkedEffect1) {
        delete m_pLinkedEffect1;
        m_pLinkedEffect1 = 0;
    }
    if (m_pLinkedEffect2) {
        delete m_pLinkedEffect2;
        m_pLinkedEffect2 = 0;
    }
    if (m_pIntegers) {
        // The same scalar-delete-against-array-new mismatch as SetNumIntegers.
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pIntegers;
#else
        delete m_pIntegers;
#endif
        m_pIntegers = 0;
    }
}

// Resizes the heap integer-parameter block, discarding whatever was in it. The count is stored
// first and re-read after the free, so the allocation is sized off the member rather than the
// argument.
void CGameEffect::SetNumIntegers(int nCount) {
    m_numIntegers = nCount;
    if (m_pIntegers) {
        // Scalar delete against the array new below, kept as it is. Harmless for a trivially
        // destructible element type on this allocator.
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pIntegers;
#else
        delete m_pIntegers;
#endif
        m_pIntegers = 0;
    }
    // The count goes through a local rather than being re-read off the member, so the memset size
    // does not cost a reload and a fresh shift.
    int nSize = m_numIntegers;
    m_pIntegers = new int[nSize];
    memset(m_pIntegers, 0, sizeof(int) * nSize);
}

int CGameEffect::GetInteger(int nIndex) {
    return m_pIntegers[nIndex];
}

void CGameEffect::SetInteger(int nIndex, int nValue) {
    m_pIntegers[nIndex] = nValue;
}

float CGameEffect::GetFloat(int nIndex) {
    return m_fParams[nIndex];
}

void CGameEffect::SetFloat(int nIndex, float fValue) {
    m_fParams[nIndex] = fValue;
}

unsigned long long CGameEffect::GetObjectID(int nIndex) {
    return m_objectIds[nIndex];
}

void CGameEffect::SetObjectID(int nIndex, unsigned long long objectId) {
    m_objectIds[nIndex] = objectId;
}

CExoString CGameEffect::GetString(int nIndex) {
    return m_sParams[nIndex];
}

void CGameEffect::SetString(int nIndex, const CExoString& sValue) {
    m_sParams[nIndex] = sValue;
}

// Sets the expiry day and time.
void CGameEffect::SetExpiryTime(unsigned int nDay, unsigned int nTime) {
    m_expireDay = nDay;
    m_expireTime = nTime;
}

void CGameEffect::GetExpiryTime(unsigned int* pnDay, unsigned int* pnTime) {
    *pnDay = m_expireDay;
    *pnTime = m_expireTime;
}

// Attaches the two child effects, then propagates this node's duration and type down to them.
void CGameEffect::SetLinked(CGameEffect* pLeft, CGameEffect* pRight) {
    m_pLinkedEffect1 = pLeft;
    m_pLinkedEffect2 = pRight;
    UpdateLinked();
}

int CGameEffect::operator!=(const CGameEffect& other) const {
    return !(*this == other);
}

// Deep-copies the source effect onto this one. The second argument suppresses everything below the
// integer block: with it set, the parameter values are left untouched and only the header plus the
// freshly resized integer block are written. The effect id is copied only when this effect does not
// already have one.
void CGameEffect::CopyEffect(CGameEffect* pSource, int bHeaderOnly)
{
    if (!pSource)
        return;

    if (m_effectId == 0)
        m_effectId = pSource->m_effectId;
    m_subType = pSource->m_subType;
    m_durationTypeBits = pSource->m_durationTypeBits;
    m_fDuration = pSource->m_fDuration;
    m_expireDay = pSource->m_expireDay;
    m_expireTime = pSource->m_expireTime;
    m_creatorId = pSource->m_creatorId;
    m_spellId = pSource->m_spellId;
    m_bIsExposed = pSource->m_bIsExposed;
    m_nCasterLevel = pSource->m_nCasterLevel;

    if (pSource->m_pLinkedEffect1)
    {
        m_pLinkedEffect1 = new CGameEffect(0);
        m_pLinkedEffect1->CopyEffect(pSource->m_pLinkedEffect1, 0);
    }
    if (pSource->m_pLinkedEffect2)
    {
        m_pLinkedEffect2 = new CGameEffect(0);
        m_pLinkedEffect2->CopyEffect(pSource->m_pLinkedEffect2, 0);
    }

    SetNumIntegers(pSource->m_numIntegers);
    if (bHeaderOnly)
        return;

    for (int i = 0; i < m_numIntegers; i++)
        m_pIntegers[i] = pSource->m_pIntegers[i];
    for (int i = 0; i < 4; i++)
        m_fParams[i] = pSource->m_fParams[i];
    for (int i = 0; i < 4; i++)
        m_objectIds[i] = pSource->m_objectIds[i];
    for (int i = 0; i < 6; i++)
        m_sParams[i] = pSource->m_sParams[i];
}

// Almost CopyEffect(other, 0), but it does not copy m_nCasterLevel and it has no header-only escape. The
// two linked children are deep-copied through CopyEffect.
CGameEffect& CGameEffect::operator=(const CGameEffect& other)
{
    if (m_effectId == 0)
        m_effectId = other.m_effectId;
    m_subType = other.m_subType;
    m_durationTypeBits = other.m_durationTypeBits;
    m_fDuration = other.m_fDuration;
    m_expireDay = other.m_expireDay;
    m_expireTime = other.m_expireTime;
    m_creatorId = other.m_creatorId;
    m_spellId = other.m_spellId;
    m_bIsExposed = other.m_bIsExposed;

    if (other.m_pLinkedEffect1)
    {
        m_pLinkedEffect1 = new CGameEffect(0);
        m_pLinkedEffect1->CopyEffect(other.m_pLinkedEffect1, 0);
    }
    if (other.m_pLinkedEffect2)
    {
        m_pLinkedEffect2 = new CGameEffect(0);
        m_pLinkedEffect2->CopyEffect(other.m_pLinkedEffect2, 0);
    }

    SetNumIntegers(other.m_numIntegers);
    // Frees anything left in the parameter block.
    for (int i = 0; i < m_numIntegers; i++)
        m_pIntegers[i] = other.m_pIntegers[i];
    for (int i = 0; i < 4; i++)
        m_fParams[i] = other.m_fParams[i];
    for (int i = 0; i < 4; i++)
        m_objectIds[i] = other.m_objectIds[i];
    for (int i = 0; i < 6; i++)
        m_sParams[i] = other.m_sParams[i];
    return *this;
}

// Field-by-field equality. Note what is not compared: the duration, the expiry pair, m_bIsExposed
// and m_bSkipOnLoad. Only identity, type, creator, spell and parameter state count.
int CGameEffect::operator==(const CGameEffect& other) const
{
    if (m_effectId != other.m_effectId)
        return 0;
    if (m_subType != other.m_subType)
        return 0;
    if (m_durationTypeBits != other.m_durationTypeBits)
        return 0;
    if (m_creatorId != other.m_creatorId)
        return 0;
    if (m_spellId != other.m_spellId)
        return 0;
    if (m_numIntegers != other.m_numIntegers)
        return 0;
    for (int i = 0; i < m_numIntegers; i++)
    {
        if (m_pIntegers[i] != other.m_pIntegers[i])
            return 0;
    }
    for (int i = 0; i < 4; i++)
    {
        if (m_fParams[i] != other.m_fParams[i])
            return 0;
    }
    for (int i = 0; i < 4; i++)
    {
        if (m_objectIds[i] != other.m_objectIds[i])
            return 0;
    }
    for (int i = 0; i < 6; i++)
    {
        if (m_sParams[i] != other.m_sParams[i])
            return 0;
    }
    return 1;
}

// Pushes this node's duration, type, expiry, spell and id down onto both linked children,
// recursively. The two bits above the 3-bit duration type are propagated first, before the
// recursion, and only three of the four combinations are handled: the all-bits case ORs them in
// without clearing first, the other two clear and then set.
void CGameEffect::UpdateLinked()
{
    if (m_pLinkedEffect1)
    {
        if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_MAGICAL)
            m_pLinkedEffect1->m_durationTypeBits = (m_pLinkedEffect1->m_durationTypeBits & ~SUBTYPE_MASK) | SUBTYPE_MAGICAL;
        else if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_SUPERNATURAL)
            m_pLinkedEffect1->m_durationTypeBits = (m_pLinkedEffect1->m_durationTypeBits & ~SUBTYPE_MASK) | SUBTYPE_SUPERNATURAL;
        else if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_EXTRAORDINARY)
            m_pLinkedEffect1->m_durationTypeBits |= SUBTYPE_EXTRAORDINARY;

        m_pLinkedEffect1->UpdateLinked();
        m_pLinkedEffect1->SetCreator(m_creatorId);
        // Operand order is load-bearing: swapping the operands gives the same instructions with the two
        // registers exchanged.
        m_pLinkedEffect1->m_durationTypeBits =
            (m_durationTypeBits & DURATION_TYPE_MASK) | (m_pLinkedEffect1->m_durationTypeBits & ~DURATION_TYPE_MASK);
        m_pLinkedEffect1->m_fDuration = m_fDuration;
        m_pLinkedEffect1->m_expireDay = m_expireDay;
        m_pLinkedEffect1->m_expireTime = m_expireTime;
        m_pLinkedEffect1->m_spellId = m_spellId;
        m_pLinkedEffect1->m_effectId = m_effectId;
    }

    if (m_pLinkedEffect2)
    {
        if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_MAGICAL)
            m_pLinkedEffect2->m_durationTypeBits = (m_pLinkedEffect2->m_durationTypeBits & ~SUBTYPE_MASK) | SUBTYPE_MAGICAL;
        else if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_SUPERNATURAL)
            m_pLinkedEffect2->m_durationTypeBits = (m_pLinkedEffect2->m_durationTypeBits & ~SUBTYPE_MASK) | SUBTYPE_SUPERNATURAL;
        else if ((m_durationTypeBits & SUBTYPE_MASK) == SUBTYPE_EXTRAORDINARY)
            m_pLinkedEffect2->m_durationTypeBits |= SUBTYPE_EXTRAORDINARY;

        m_pLinkedEffect2->UpdateLinked();
        m_pLinkedEffect2->SetCreator(m_creatorId);
        // Operand order is load-bearing: swapping the two operands gives the same instructions
        // with the two registers exchanged.
        m_pLinkedEffect2->m_durationTypeBits =
            (m_durationTypeBits & DURATION_TYPE_MASK) | (m_pLinkedEffect2->m_durationTypeBits & ~DURATION_TYPE_MASK);
        m_pLinkedEffect2->m_fDuration = m_fDuration;
        m_pLinkedEffect2->m_expireDay = m_expireDay;
        m_pLinkedEffect2->m_expireTime = m_expireTime;
        m_pLinkedEffect2->m_spellId = m_spellId;
        m_pLinkedEffect2->m_effectId = m_effectId;
    }
}

// Rebuilds one effect from its GFF struct, with four parameter lists: 21 ints, 4 floats, 6 strings
// and 4 object ids.
//
// The GFF "Type" and "SubType" labels are shifted one slot against the field names, so "Type" is
// m_subType and "SubType" is m_durationTypeBits. Do not "fix" it.
//
// The damage arm is a save-format migration. An older damage effect stored fewer than 21 integers,
// with six trailing parameters that have since moved to the top of the block. Slots up to 14 are
// zero-filled and the last six values land at slots 15..20.
void CGameEffect::LoadGameEffect(CResGFF* pGff, CResStruct* pStruct)
{
    CExoString sValue;
    int bSuccess;
    CResStruct sElement;
    CResList sList;

    m_effectId = pGff->ReadFieldDWORD64(pStruct, const_cast<char*>("Id"), bSuccess, 0);
    m_subType = pGff->ReadFieldWORD(pStruct, const_cast<char*>("Type"), bSuccess, 0);
    m_durationTypeBits = pGff->ReadFieldWORD(pStruct, const_cast<char*>("SubType"), bSuccess, 0);
    m_fDuration = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("Duration"), bSuccess, 0.0f);
    m_bSkipOnLoad = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("SkipOnLoad"), bSuccess, 0);
    SetExpiryTime(pGff->ReadFieldDWORD(pStruct, const_cast<char*>("ExpireDay"), bSuccess, 0),
                  pGff->ReadFieldDWORD(pStruct, const_cast<char*>("ExpireTime"), bSuccess, 0));
    SetCreator(pGff->ReadFieldDWORD(pStruct, const_cast<char*>("CreatorId"), bSuccess, 0));
    // After SetCreator, which writes the spell id itself when the creator resolves to a real object;
    // the stored spell id wins.
    m_spellId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("SpellId"), bSuccess, 0);
    m_bIsExposed = pGff->ReadFieldINT(pStruct, const_cast<char*>("IsExposed"), bSuccess, 0);
    SetNumIntegers(pGff->ReadFieldINT(pStruct, const_cast<char*>("NumIntegers"), bSuccess, 0));

#if KOTOR1  // K1: poison effects take the short-IntList migration too
    if ((m_subType == CSWSEffectListHandler::EFFECT_DAMAGE ||
         m_subType == CSWSEffectListHandler::EFFECT_POISON) && m_numIntegers <= 20)
#else
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE && m_numIntegers <= 20)
#endif
    {
        int nSrc = 0;
        unsigned int nDst = 0;
        if (pGff->GetList(&sList, pStruct, const_cast<char*>("IntList")) == 1)
        {
            while (nDst < 21)
            {
                // The read block appears once per arm; only the store and increment tail is shared.
                if (nDst < static_cast<unsigned int>(m_numIntegers - 6))
                {
                    // No else, so a list shorter than m_numIntegers would spin forever. The loop only
                    // terminates because it consumes exactly m_numIntegers elements.
                    if (static_cast<unsigned int>(nSrc) < pGff->GetListCount(&sList))
                    {
                        pGff->GetListElement(&sElement, &sList, nSrc);
                        m_pIntegers[nSrc] = pGff->ReadFieldINT(&sElement, const_cast<char*>("Value"), bSuccess, 0);
                        nSrc++;
                        nDst++;
                    }
                }
                else if (nDst <= 14)
                {
                    m_pIntegers[nSrc] = 0;
                    nDst++;
                }
                else
                {
                    if (static_cast<unsigned int>(nSrc) < pGff->GetListCount(&sList))
                    {
                        pGff->GetListElement(&sElement, &sList, nSrc);
                        m_pIntegers[nSrc] = pGff->ReadFieldINT(&sElement, const_cast<char*>("Value"), bSuccess, 0);
                        nSrc++;
                        nDst++;
                    }
                }
            }
        }
        // After the fill loop, so it frees and re-zeroes everything just read. An engine bug,
        // reproduced as-is.
        SetNumIntegers(21);
    }
    else if (m_numIntegers > 0)
    {
        if (pGff->GetList(&sList, pStruct, const_cast<char*>("IntList")) == 1)
        {
            // The counter is unsigned and the values go through the SetInteger accessor.
            for (unsigned int i = 0; i < pGff->GetListCount(&sList); i++)
            {
                pGff->GetListElement(&sElement, &sList, i);
                SetInteger(i, pGff->ReadFieldINT(&sElement, const_cast<char*>("Value"), bSuccess, 0));
            }
        }
    }

    if (pGff->GetList(&sList, pStruct, const_cast<char*>("FloatList")) == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            pGff->GetListElement(&sElement, &sList, i);
            m_fParams[i] = pGff->ReadFieldFLOAT(&sElement, const_cast<char*>("Value"), bSuccess, 0.0f);
        }
    }

    if (pGff->GetList(&sList, pStruct, const_cast<char*>("StringList")) == 1)
    {
        for (int i = 0; i < 6; i++)
        {
            pGff->GetListElement(&sElement, &sList, i);
            sValue = pGff->ReadFieldCExoString(&sElement, const_cast<char*>("Value"), bSuccess, CExoString(""));
            m_sParams[i] = sValue;
        }
    }

    if (pGff->GetList(&sList, pStruct, const_cast<char*>("ObjectList")) == 1)
    {
        for (int i = 0; i < 4; i++)
        {
            pGff->GetListElement(&sElement, &sList, i);
            m_objectIds[i] = pGff->ReadFieldDWORD(&sElement, const_cast<char*>("Value"), bSuccess, 0);
        }
    }
}

// Saves the effect as the exact inverse of LoadGameEffect, writing the same keys in the same order,
// then the four parameter lists. There is no damage special case on this side, since the migration
// is load-only.
void CGameEffect::SaveGameEffect(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct sElement;
    CResList sList;

    pGff->WriteFieldDWORD64(pStruct, m_effectId, const_cast<char*>("Id"));
    pGff->WriteFieldWORD(pStruct, m_subType, const_cast<char*>("Type"));
    pGff->WriteFieldWORD(pStruct, m_durationTypeBits, const_cast<char*>("SubType"));
    pGff->WriteFieldFLOAT(pStruct, m_fDuration, const_cast<char*>("Duration"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bSkipOnLoad), const_cast<char*>("SkipOnLoad"));
    // Through the accessor, so the two loads fold into one 8-byte load.
    unsigned int nExpireDay;
    unsigned int nExpireTime;
    GetExpiryTime(&nExpireDay, &nExpireTime);
    pGff->WriteFieldDWORD(pStruct, nExpireDay, const_cast<char*>("ExpireDay"));
    pGff->WriteFieldDWORD(pStruct, nExpireTime, const_cast<char*>("ExpireTime"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_creatorId), const_cast<char*>("CreatorId"));
    pGff->WriteFieldDWORD(pStruct, m_spellId, const_cast<char*>("SpellId"));
    pGff->WriteFieldINT(pStruct, m_bIsExposed, const_cast<char*>("IsExposed"));
    pGff->WriteFieldINT(pStruct, m_numIntegers, const_cast<char*>("NumIntegers"));

    if (m_numIntegers > 0)
    {
        pGff->AddList(&sList, pStruct, const_cast<char*>("IntList"));
        // The counter is unsigned, matching the member.
        for (unsigned int i = 0; i < static_cast<unsigned int>(m_numIntegers); i++)
        {
            pGff->AddListElement(&sElement, &sList, 3);
            pGff->WriteFieldINT(&sElement, GetInteger(i), const_cast<char*>("Value"));
        }
    }

    pGff->AddList(&sList, pStruct, const_cast<char*>("FloatList"));
    for (int i = 0; i < 4; i++)
    {
        pGff->AddListElement(&sElement, &sList, 4);
        pGff->WriteFieldFLOAT(&sElement, m_fParams[i], const_cast<char*>("Value"));
    }

    pGff->AddList(&sList, pStruct, const_cast<char*>("StringList"));
    for (int i = 0; i < 6; i++)
    {
        pGff->AddListElement(&sElement, &sList, 5);
        // GetString() returns by value, so this materialises a temporary that is destroyed after the
        // write.
        pGff->WriteFieldCExoString(&sElement, GetString(i), const_cast<char*>("Value"));
    }

    pGff->AddList(&sList, pStruct, const_cast<char*>("ObjectList"));
    for (int i = 0; i < 4; i++)
    {
        pGff->AddListElement(&sElement, &sList, 6);
        pGff->WriteFieldDWORD(&sElement, static_cast<unsigned int>(m_objectIds[i]), const_cast<char*>("Value"));
    }
}

// Maps the engine's internal effect subtype onto the NWScript EFFECT_TYPE_* constant the script side
// sees. A flat equality chain rather than a switch, because every label is a GOT-loaded static const
// int and so not a compile-time case value.
int CGameEffect::GetEffectTypeScriptConstant()
{
    if (m_subType == CSWSEffectListHandler::EFFECT_INVALIDEFFECT)
        return EFFECT_TYPE_INVALIDEFFECT;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE)
        return EFFECT_TYPE_DAMAGE_RESISTANCE;
    if (m_subType == CSWSEffectListHandler::EFFECT_REGENERATE)
        return EFFECT_TYPE_REGENERATE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION)
        return EFFECT_TYPE_DAMAGE_REDUCTION;
    if (m_subType == CSWSEffectListHandler::EFFECT_TEMPORARY_HITPOINTS)
        return EFFECT_TYPE_TEMPORARY_HITPOINTS;
    if (m_subType == CSWSEffectListHandler::EFFECT_ENTANGLE)
        return EFFECT_TYPE_ENTANGLE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DEAF)
        return EFFECT_TYPE_DEAF;
    if (m_subType == CSWSEffectListHandler::EFFECT_IMMUNITY)
        return EFFECT_TYPE_IMMUNITY;
    if (m_subType == CSWSEffectListHandler::EFFECT_ENEMY_ATTACK_BONUS)
        return EFFECT_TYPE_ENEMY_ATTACK_BONUS;
    if (m_subType == CSWSEffectListHandler::EFFECT_ARCANE_SPELL_FAILURE)
        return EFFECT_TYPE_ARCANE_SPELL_FAILURE;
    if (m_subType == CSWSEffectListHandler::EFFECT_AREA_OF_EFFECT)
        return EFFECT_TYPE_AREA_OF_EFFECT;
    if (m_subType == CSWSEffectListHandler::EFFECT_BEAM)
        return EFFECT_TYPE_BEAM;
    // The one arm that is not a plain constant. A set-state effect reports the script constant of
    // the state it sets, which rides in integer parameter 0.
    if (m_subType == CSWSEffectListHandler::EFFECT_SETSTATE) {
        switch (GetInteger(0)) {
        case EFFECT_STATE_CONFUSED: return EFFECT_TYPE_CONFUSED;
        case EFFECT_STATE_FRIGHTENED: return EFFECT_TYPE_FRIGHTENED;
        case EFFECT_STATE_DROID_STUN: return EFFECT_TYPE_TURNED;
        case EFFECT_STATE_STUNNED: return EFFECT_TYPE_STUNNED;
        case EFFECT_STATE_PARALYZED: return EFFECT_TYPE_PARALYZE;
        case EFFECT_STATE_SLEEP: return EFFECT_TYPE_SLEEP;
#if !KOTOR1  // K1 has states 1..6 only
        case EFFECT_STATE_DROID_CONFUSED: return EFFECT_TYPE_DROID_CONFUSED;
        case EFFECT_STATE_MIND_TRICK: return EFFECT_TYPE_MINDTRICK;
        case EFFECT_STATE_DROID_SCRAMBLE: return EFFECT_TYPE_DROIDSCRAMBLE;
#endif
        }
        return 0;
    }
    if (m_subType == CSWSEffectListHandler::EFFECT_POISON)
        return EFFECT_TYPE_POISON;
    if (m_subType == CSWSEffectListHandler::EFFECT_DISEASE)
        return EFFECT_TYPE_DISEASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_CURSE)
        return EFFECT_TYPE_CURSE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SILENCE)
        return EFFECT_TYPE_SILENCE;
    if (m_subType == CSWSEffectListHandler::EFFECT_HASTE)
        return EFFECT_TYPE_HASTE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SLOW)
        return EFFECT_TYPE_SLOW;
    if (m_subType == CSWSEffectListHandler::EFFECT_ABILITY_INCREASE)
        return EFFECT_TYPE_ABILITY_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_ABILITY_DECREASE)
        return EFFECT_TYPE_ABILITY_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE)
        return EFFECT_TYPE_ATTACK_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_ATTACK_DECREASE)
        return EFFECT_TYPE_ATTACK_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
        return EFFECT_TYPE_DAMAGE_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
        return EFFECT_TYPE_DAMAGE_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE)
        return EFFECT_TYPE_DAMAGE_IMMUNITY_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE)
        return EFFECT_TYPE_DAMAGE_IMMUNITY_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
        return EFFECT_TYPE_AC_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_AC_DECREASE)
        return EFFECT_TYPE_AC_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_INCREASE)
        return EFFECT_TYPE_MOVEMENT_SPEED_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE)
        return EFFECT_TYPE_MOVEMENT_SPEED_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE)
        return EFFECT_TYPE_SAVING_THROW_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE)
        return EFFECT_TYPE_SAVING_THROW_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE)
        return EFFECT_TYPE_FORCE_RESISTANCE_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_DECREASE)
        return EFFECT_TYPE_FORCE_RESISTANCE_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SKILL_INCREASE)
        return EFFECT_TYPE_SKILL_INCREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SKILL_DECREASE)
        return EFFECT_TYPE_SKILL_DECREASE;
    if (m_subType == CSWSEffectListHandler::EFFECT_INVISIBILITY)
        return EFFECT_TYPE_INVISIBILITY;
    if (m_subType == CSWSEffectListHandler::EFFECT_DARKNESS)
        return EFFECT_TYPE_DARKNESS;
    if (m_subType == CSWSEffectListHandler::EFFECT_DISPEL_ALL_MAGIC)
        return EFFECT_TYPE_DISPELMAGICALL;
    if (m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD)
        return EFFECT_TYPE_ELEMENTALSHIELD;
    if (m_subType == CSWSEffectListHandler::EFFECT_NEGATIVE_LEVEL)
        return EFFECT_TYPE_NEGATIVELEVEL;
    if (m_subType == CSWSEffectListHandler::EFFECT_DISGUISE)
        return EFFECT_TYPE_DISGUISE;
    if (m_subType == CSWSEffectListHandler::EFFECT_SANCTUARY)
        return EFFECT_TYPE_SANCTUARY;
    if (m_subType == CSWSEffectListHandler::EFFECT_TRUESEEING)
        return EFFECT_TYPE_TRUESEEING;
    if (m_subType == CSWSEffectListHandler::EFFECT_SEEINVISIBLE)
        return EFFECT_TYPE_SEEINVISIBLE;
    if (m_subType == CSWSEffectListHandler::EFFECT_TIMESTOP)
        return EFFECT_TYPE_TIMESTOP;
    if (m_subType == CSWSEffectListHandler::EFFECT_BLINDNESS)
        return EFFECT_TYPE_BLINDNESS;
    if (m_subType == CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION)
        return EFFECT_TYPE_SPELLLEVELABSORPTION;
    if (m_subType == CSWSEffectListHandler::EFFECT_DISPEL_BEST_MAGIC)
        return EFFECT_TYPE_DISPELMAGICBEST;
    if (m_subType == CSWSEffectListHandler::EFFECT_ULTRAVISION)
        return EFFECT_TYPE_ULTRAVISION;
    if (m_subType == CSWSEffectListHandler::EFFECT_MISS_CHANCE)
        return EFFECT_TYPE_MISS_CHANCE;
    if (m_subType == CSWSEffectListHandler::EFFECT_CONCEALMENT)
        return EFFECT_TYPE_CONCEALMENT;
    if (m_subType == CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY)
        return EFFECT_TYPE_SPELL_IMMUNITY;
    if (m_subType == CSWSEffectListHandler::EFFECT_ASSUREDHIT)
        return EFFECT_TYPE_ASSUREDHIT;
    if (m_subType == CSWSEffectListHandler::EFFECT_VISUALEFFECT)
        return EFFECT_TYPE_VISUAL;
    if (m_subType == CSWSEffectListHandler::EFFECT_LIGHTSABERTHROW)
        return EFFECT_TYPE_LIGHTSABERTHROW;
    if (m_subType == CSWSEffectListHandler::EFFECT_FORCEJUMP)
        return EFFECT_TYPE_FORCEJUMP;
    if (m_subType == CSWSEffectListHandler::EFFECT_ASSUREDDEFLECTION)
        return EFFECT_TYPE_ASSUREDDEFLECTION;
    return 0;
}
