// baseitems.2da row accessors.

#include "k2stdint.h"
#include "nwitem.h"
#include "twodimarray.h"
#include "exolocstringinternal.h"
#include <stdio.h>
#include "nwsrules.h"
#include "port/libcstring.h"
#include "nwscriptconst.h"
#include "exostringinline.h"

// m_nMaxDexBonus for an item that does not cap the dexterity bonus, and GetRequiredFeat's answer
// past the end of the list.
enum {
    MAX_DEX_BONUS_NONE = 0xff,
    FEAT_NONE          = 0xffff,
};

// Resets every field to its default row value; the name strref and the armour max-Dex cap are the
// two nonzero defaults.
CSWBaseItem::CSWBaseItem()
{
    m_nModelType = 0;
    memset(m_resRefStem, 0, sizeof(m_resRefStem));
    m_bHasEnvMap = 0;
    m_nBodyVar = 1;
    m_nInvSoundType = 0;
    m_pRequiredFeats = 0;
    m_nRequiredFeats = 0;
    m_bRotateOnGround = 0;
    m_nWeaponSpecializationFeat = 0;
    m_nWeaponFocusFeat = 0;
    m_nDenySubrace = 0;
    m_nNameStrRef = -1;
    m_nEquipableSlots = 0;
    m_nMaxDexBonus = MAX_DEX_BONUS_NONE;
}

unsigned short CSWBaseItem::GetRequiredFeat(unsigned char nIndex)
{
    if (m_nRequiredFeats <= nIndex)
        return FEAT_NONE;
    return m_pRequiredFeats[nIndex];
}

void CSWBaseItem::SetRequiredFeat(unsigned char nIndex, unsigned short value)
{
    if (m_nRequiredFeats <= nIndex)
        return;
    m_pRequiredFeats[nIndex] = value;
}

// Reallocates the required-feat array to `count` entries (freeing any prior one).
void CSWBaseItem::SetRequiredFeatCount(unsigned char count)
{
    if (m_pRequiredFeats) {
        delete[] m_pRequiredFeats;
        m_pRequiredFeats = 0;
    }
    if (count != 0)
        m_pRequiredFeats = new unsigned short[count];
    m_nRequiredFeats = count;
}

int CSWBaseItem::GetIsOneHanded()
{
    if (m_weaponWieldType == WEAPON_WIELD_STUN_BATON || m_weaponWieldType == WEAPON_WIELD_ONE_HANDED)
        return 1;
    return m_weaponWieldType == WEAPON_WIELD_PISTOL;
}

int CSWBaseItem::GetIsTwoHanded()
{
    if (m_weaponWieldType == WEAPON_WIELD_TWO_HANDED || m_weaponWieldType == WEAPON_WIELD_RIFLE)
        return 1;
    return m_weaponWieldType == WEAPON_WIELD_HEAVY;
}

CSWBaseItemArray::CSWBaseItemArray()
{
    m_count = 0;
    m_pItems = 0;
}

CSWBaseItem* CSWBaseItemArray::GetBaseItem(int index) const
{
    if (index < 0)
        return 0;
    if (m_pItems == 0)
        return 0;
    if (m_count <= index)
        return 0;
    return &m_pItems[index];
}

CSWBaseItem::~CSWBaseItem()
{
    delete[] m_pRequiredFeats;
}

CSWBaseItemArray::~CSWBaseItemArray()
{
    if (m_pItems) {
        delete[] m_pItems;
        m_pItems = 0;
        m_count = 0;
    }
}

inline void CSWBaseItem::SetItemClassResRefChunk(char* pszClass)
{
    strncpy(m_resRefStem, pszClass, sizeof(m_resRefStem) - 1);
}

// Loads the whole baseitems.2da table: one C2DA round trip, one array of rows, then a flat per-row
// run of column reads. The column order below is the table's own read order, not the struct's
// offset order.
void CSWBaseItemArray::Load()
{
    C2DA* pTable = new C2DA(CResRef("BASEITEMS"), 0);
    pTable->Load2DArray();

    m_count = pTable->m_nRows;
    m_pItems = new CSWBaseItem[m_count];

    CExoString sValue;
    for (int nRow = 0; nRow < m_count; nRow++) {
        int nValue;
        int nFound;
        float fValue;

        pTable->GetINTEntry(nRow, CExoString("Name"), &nValue);
        m_pItems[nRow].m_nNameStrRef = nValue;

        pTable->GetINTEntry(nRow, CExoString("EquipableSlots"), &nValue);
        m_pItems[nRow].m_nEquipableSlots = nValue;

        pTable->GetINTEntry(nRow, CExoString("CanRotateIcon"), &nValue);
        m_pItems[nRow].m_bCanRotateIcon = nValue;

        pTable->GetINTEntry(nRow, CExoString("ModelType"), &nValue);
        m_pItems[nRow].m_nModelType = static_cast<unsigned char>(nValue);

        pTable->GetCExoStringEntry(nRow, CExoString("ItemClass"), &sValue);
        m_pItems[nRow].SetItemClassResRefChunk(sValue.CStr());

        pTable->GetINTEntry(nRow, CExoString("GenderSpecific"), &nValue);
        m_pItems[nRow].m_bHasTextureVariation = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("PartEnvMap"), &nValue);
        m_pItems[nRow].m_bHasEnvMap = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("DefaultModel"), &sValue);
        m_pItems[nRow].SetDefaultModelResRef(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("DefaultIcon"), &sValue);
        m_pItems[nRow].SetDefaultIconResRef(CResRef(sValue));

        pTable->GetINTEntry(nRow, CExoString("Container"), &nValue);
        m_pItems[nRow].m_bContainer = nValue;

        pTable->GetINTEntry(nRow, CExoString("WeaponWield"), &nValue);
        m_pItems[nRow].m_weaponWieldType = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("WeaponType"), &nValue);
        m_pItems[nRow].m_nWeaponType = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("DamageFlags"), &nValue);
        m_pItems[nRow].m_nDamageFlags = nValue;

        pTable->GetINTEntry(nRow, CExoString("WeaponSize"), &nValue);
        m_pItems[nRow].m_nWeaponSize = static_cast<unsigned char>(nValue);

        nFound = pTable->GetINTEntry(nRow, CExoString("RangedWeapon"), &nValue);
        m_pItems[nRow].m_bRangedWeapon = nFound ? static_cast<unsigned char>(nValue) : static_cast<unsigned char>(0);

        pTable->GetFLOATEntry(nRow, CExoString("MaxAttackRange"), &fValue);
        m_pItems[nRow].m_fMaxRange = fValue;

        pTable->GetFLOATEntry(nRow, CExoString("PrefAttackDist"), &fValue);
        m_pItems[nRow].m_fPrefAttackDist = fValue;

        pTable->GetINTEntry(nRow, CExoString("MinRange"), &nValue);
        m_pItems[nRow].m_nMinRange = nValue;

        pTable->GetINTEntry(nRow, CExoString("MaxRange"), &nValue);
        m_pItems[nRow].m_nMaxRange = nValue;

        pTable->GetINTEntry(nRow, CExoString("NumDice"), &nValue);
        m_pItems[nRow].m_nNumDice = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("DieToRoll"), &nValue);
        m_pItems[nRow].m_nDieToRoll = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("CritThreat"), &nValue);
        m_pItems[nRow].m_nCritThreat = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("CritHitMult"), &nValue);
        m_pItems[nRow].m_nCritMultiplier = static_cast<unsigned char>(nValue);

        pTable->GetFLOATEntry(nRow, CExoString("BaseCost"), &fValue);
        m_pItems[nRow].m_fBaseCost = fValue;

        pTable->GetINTEntry(nRow, CExoString("Stacking"), &nValue);
        m_pItems[nRow].m_nStacking = static_cast<unsigned short>(nValue);
        m_pItems[nRow].m_costMultiplier = 1.0f;

        pTable->GetINTEntry(nRow, CExoString("Description"), &nValue);
        m_pItems[nRow].m_nDescription = nValue;

        if (pTable->GetINTEntry(nRow, CExoString("InvSoundType"), &nValue))
            m_pItems[nRow].m_nInvSoundType = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("MaxProps"), &nValue);
        m_pItems[nRow].m_nMaxProps = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("MinProps"), &nValue);
        m_pItems[nRow].m_nMinProps = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("PropColumn"), &nValue);
        m_pItems[nRow].m_nPropColumn = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("ArmorCheckPen"), &nValue);
        m_pItems[nRow].m_nArmorCheckPen = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("ChargesStarting"), &nValue);
        m_pItems[nRow].m_nChargesStarting = static_cast<unsigned char>(nValue);

        if (!pTable->GetINTEntry(nRow, CExoString("BaseItemStatRef"), &nValue))
            nValue = -1;
        m_pItems[nRow].m_nBaseItemStatRef = nValue;

        pTable->GetINTEntry(nRow, CExoString("StorePanel"), &nValue);
        m_pItems[nRow].m_nStorePanel = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("StorePanelSort"), &nValue);
        m_pItems[nRow].m_nStorePanelSort = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("RotateOnGround"), &nValue);
        m_pItems[nRow].m_bRotateOnGround = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("TenthLBS"), &nValue);
        m_pItems[nRow].m_nTenthLbs = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("BaseAC"), &nValue);
        m_pItems[nRow].m_nBaseAC = static_cast<unsigned char>(nValue);

        // The absent-column value here is -1, not 0.
        nFound = pTable->GetINTEntry(nRow, CExoString("AC_Enchant"), &nValue);
        m_pItems[nRow].m_nACEnchant = nFound ? static_cast<unsigned char>(nValue) : static_cast<unsigned char>(-1);

        pTable->GetINTEntry(nRow, CExoString("WeaponMatType"), &nValue);
        m_pItems[nRow].m_bIsPC = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("AmmunitionType"), &nValue);
        m_pItems[nRow].m_nAmmunitionType = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("PoweredItem"), &nValue);
        m_pItems[nRow].m_bPoweredItem = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("PoweredSnd"), &sValue);
        m_pItems[nRow].m_sPoweredSnd = sValue;

        pTable->GetCExoStringEntry(nRow, CExoString("PowerDownSnd"), &sValue);
        m_pItems[nRow].m_sPowerDownSnd = sValue;

        pTable->GetCExoStringEntry(nRow, CExoString("PowerUpSnd"), &sValue);
        m_pItems[nRow].m_sPowerUpSnd = sValue;

        pTable->GetINTEntry(nRow, CExoString("ItemType"), &nValue);
        m_pItems[nRow].m_nItemType = static_cast<unsigned char>(nValue);

        if (!pTable->GetINTEntry(nRow, CExoString("DroidOrHuman"), &nValue))
            nValue = 0;
        m_pItems[nRow].m_nRaceRestriction = static_cast<unsigned char>(nValue);

        pTable->GetCExoStringEntry(nRow, CExoString("BodyVar"), &sValue);
        m_pItems[nRow].m_nBodyVar = 1;
        // The returned copy is discarded.
        sValue.UpperCase();
        if (sValue.GetLength() == 1
            && sValue[0] >= 'A' && sValue[0] <= 'Z')
            m_pItems[nRow].m_nBodyVar = sValue[0] - '@';

        unsigned char nReqFeatCount = 0;
        CExoString sReqFeat;
        for (int i = 0; i < 5; i++) {
            sReqFeat.Format(const_cast<char*>("ReqFeat%d"), i);
            if (pTable->GetINTEntry(nRow, CExoString(sReqFeat.CStr()), &nValue))
                nReqFeatCount++;
        }
        if (nReqFeatCount != 0) {
            m_pItems[nRow].SetRequiredFeatCount(nReqFeatCount);
            unsigned char nFeat = 0;
            for (int i = 0; i < 5; i++) {
                sReqFeat.Format(const_cast<char*>("ReqFeat%d"), i);
                if (pTable->GetINTEntry(nRow, CExoString(sReqFeat.CStr()), &nValue)) {
                    m_pItems[nRow].SetRequiredFeat(nFeat, static_cast<unsigned short>(nValue));
                    nFeat++;
                }
            }
        }

        if (pTable->GetINTEntry(nRow, CExoString("SpecFeat"), &nValue))
            m_pItems[nRow].m_nWeaponSpecializationFeat = static_cast<unsigned short>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("FocFeat"), &nValue))
            m_pItems[nRow].m_nWeaponFocusFeat = static_cast<unsigned short>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("DenySubrace"), &nValue))
            m_pItems[nRow].m_nDenySubrace = nValue;

        if (pTable->GetCExoStringEntry(nRow, CExoString("ArmorType"), &sValue))
            m_pItems[nRow].m_sArmorType = sValue;

        if (pTable->GetINTEntry(nRow, CExoString("DEXBONUS"), &nValue))
            m_pItems[nRow].m_nMaxDexBonus = static_cast<unsigned char>(nValue);
    }
    delete pTable;
}

CExoString CSWBaseItem::GetNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

// The icon resref for one appearance variation, used only for a nonzero texture variation on a row
// that carries variation art.
CResRef CSWBaseItem::GetIconResRef(short nModelVariation, char nTextureVariation)
{
    char szName[17];
    if (nTextureVariation != 0 && m_bHasTextureVariation != 0)
        sprintf(szName, "i%s_%c_%03d", m_resRefStem, nTextureVariation, nModelVariation);
    else
        sprintf(szName, "i%s_%03d", m_resRefStem, nModelVariation);
    return CResRef(szName);
}

// The same pair for the model resref, with no icon prefix.
CResRef CSWBaseItem::GetModelResRef(short nModelVariation, char nTextureVariation)
{
    char szName[17];
    if (nTextureVariation != 0 && m_bHasTextureVariation != 0)
        sprintf(szName, "%s_%c_%03d", m_resRefStem, nTextureVariation, nModelVariation);
    else
        sprintf(szName, "%s_%03d", m_resRefStem, nModelVariation);
    return CResRef(szName);
}

// The shared client/server item root.

CSWItem::CSWItem()
    : m_baseItem(BASE_ITEM_SONIC_GRENADE)
{
    for (int i = 0; i < 6; i++)
        m_aModelParts[i] = 0;
}

CSWItem::~CSWItem()
{
}


CSWBaseItem* CSWItem::GetBaseItem()
{
    // m_baseItem is signed and is loaded before the global chain.
    if (m_baseItem < 0)
        return 0;
    CSWBaseItemArray* pBaseItems = g_pRules->m_pBaseItemArray;
    if (pBaseItems->m_pItems == 0)
        return 0;
    if (pBaseItems->m_count <= m_baseItem)
        return 0;
    return &pBaseItems->m_pItems[m_baseItem];
}
