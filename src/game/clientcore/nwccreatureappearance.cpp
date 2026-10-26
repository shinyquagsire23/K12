#include "k2stdint.h"
#include "nwccreatureappearance.h"
#include "AURORA/aurmodel.h"
#include "nwccreature.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "exostring.h"
#include "exoresman.h"
#include "nwscreature.h"
#include "nwcanimbase.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwcobject.h"
#include "nwcitem.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "nwmessagetypes.h"
#include "restype.h"
#include "handmaidenappearance.h"
#include "nwscriptconst.h"
#include "exostringinline.h"

// The shared formatter every appearance-suffix path uses.
static void FormatAppearanceSuffix(char* pBuf, uint64_t nSize, const char* pFmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, pFmt);
    __vsprintf_chk(pBuf, 0, nSize, pFmt, ap);
    __builtin_va_end(ap);
}

CSWCCreatureAppearance::CSWCCreatureAppearance(CSWCCreature* pCreature)
{
    m_cachedBTypeVariant = 0;
    m_cachedBTypeNumber = 1;
    for (int i = 0; i < 6; i++)
        m_aCachedBodyParts[i] = 0;
    m_oidHeadItem = CGameObjectArray::INVALIDOBJID;
    m_bArmorEquipped = 0;
    m_nAppearanceId = 0xffff;
    m_nHeadNumber = 0xff;
    m_nGender = 0xff;
    m_nPhenotype = 0xff;
    m_nSkinColor = 0;
    m_nHairColor = 0;
    m_nTattooColor1 = 0;
    m_nTattooColor2 = 0;
    m_cachedTailVariant = 0;
    m_cachedWingVariant = 0;
    m_fPersonalSpace = 0.6f;
    m_fHeight = 0.5f;
    m_cachedAlignment = 50;
    m_bUseBackupHead = 0;
    m_race = 0xff;
    m_bGroundTilt = 0;
    m_pCreature = pCreature;
    m_bRebuildHead = 0;
    m_bRebuildTail = 0;
    m_bRebuildWings = 0;
    m_bAlignmentDirty = 0;
    m_bCreatedWhilePaused = 0;
}

CSWCCreatureAppearance::~CSWCCreatureAppearance()
{
}

void CSWCCreatureAppearance::GetPartVariations(unsigned char* paOut, unsigned char n)
{
    K2_UNUSED(paOut); K2_UNUSED(n);
}

int CSWCCreatureAppearance::GetArmourClass()
{
    return 0;
}

int CSWCCreatureAppearance::RequestTailAppearance(CCAppearanceInfo* pInfo)
{
    K2_UNUSED(pInfo);
    return 1;
}

int CSWCCreatureAppearance::CreateTail(unsigned char a, CCAppearanceInfo* pInfo)
{
    K2_UNUSED(a); K2_UNUSED(pInfo);
    return 1;
}

int CSWCCreatureAppearance::CreateWing(unsigned char a, CCAppearanceInfo* pInfo)
{
    K2_UNUSED(a); K2_UNUSED(pInfo);
    return 1;
}

int CSWCCreatureAppearance::RequestWingAppearance(CCAppearanceInfo* pInfo)
{
    K2_UNUSED(pInfo);
    return 1;
}

// The camera-hook part of the creature's model, or the model itself when it has none.
void* CSWCCreatureAppearance::GetCameraFollowPart()
{
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
    Part* pPart = pModel->GetPart(const_cast<char*>("CAMERAHOOK"));
    if (pPart != nullptr)
        return pPart->GetAurObject();
    return pModel;
}

// Whether the creature's model carries a camera-hook part.
int CSWCCreatureAppearance::GetFollowingBody()
{
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
    return pModel->GetPart(const_cast<char*>("CAMERAHOOK")) != nullptr;
}

// Height of the head hook above the creature's feet, falling back to a stored offset when the model
// reports no usable position.
float CSWCCreatureAppearance::GetHeadHeightOffset()
{
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
    if (pModel != nullptr)
    {
        Part* pPart = pModel->GetPart(const_cast<char*>("CAMERAHOOK"));
        if (pPart != nullptr)
        {
            Vector vPos = {0.0f, 0.0f, 0.0f};
            Quaternion qOrient(1.0f, 0.0f, 0.0f, 0.0f);
            pModel->GetPartLocalPosition(const_cast<char*>("CAMERAHOOK"), &vPos, &qOrient);
            return vPos.z;
        }
        Vector vPos = {0.0f, 0.0f, 0.0f};
        Quaternion qOrient(1.0f, 0.0f, 0.0f, 0.0f);
        pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        if (pModel != nullptr)
        {
            float fHeadHeight;
            pModel->GetPartPosition(const_cast<char*>("HEAD_G"), &vPos, &qOrient);
            if (vPos.z == 0.0f || (fHeadHeight = vPos.z - m_pCreature->m_position.z) < 0.0f)
                fHeadHeight = m_fHeight;
            return fHeadHeight;
        }
    }
    return 0.0f;
}

// The camera height offset for this appearance, from the appearance table.
float CSWCCreatureAppearance::GetCameraOffset()
{
    float fValue = 0.0f;
    int bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_CAMERAHEIGHTOFFSET, &fValue);
    return bFound ? fValue : 0.0f;
}

// The run distance for this appearance, scaled to an animation rate.
float CSWCCreatureAppearance::GetRunLooksAwfulRate()
{
    float fValue;
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_RUNDIST, &fValue);
    return fValue * 1000.0f * 0.37f;
}

// Animation family selected by the first model-name letter.
int CSWCCreatureAppearance::GetAnimType(const CExoString& s)
{
    if (s.Find('S', 0) != -1)
        return CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS;
    if (s.Find(CExoString("B"), 0) != -1)
        return CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD;
    if (s.Find('F', 0) != -1)
        return CSWCAnimBase::ANIM_BASE_TYPE_WIELD;
    return (s.Find('L', 0) != -1) ? CSWCAnimBase::ANIM_BASE_TYPE_WIELD : CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS;
}

int CSWCCreatureAppearance::CreateTailAppearance(CCAppearanceInfo* pInfo)
{
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE, &sModelType);
    GetAnimType(sModelType);
    if (sModelType.Find('T', 0) == -1)
    {
        m_cachedTailVariant = pInfo->core.m_tailVariant;
        m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_TAIL);
    }
    return 1;
}

int CSWCCreatureAppearance::CreateWingAppearance(CCAppearanceInfo* pInfo)
{
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE, &sModelType);
    GetAnimType(sModelType);
    if (sModelType.Find('W', 0) == -1)
    {
        m_cachedWingVariant = pInfo->core.m_wingVariant;
        m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_WINGS);
    }
    return 1;
}

// Dispatches body appearance work on the appearance's model type.
int CSWCCreatureAppearance::RequestBodyAppearance(CCAppearanceInfo* pInfo)
{
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("MODELTYPE"), &sModelType);
    unsigned char nType = static_cast<unsigned char>(GetAnimType(sModelType));
    switch (nType)
    {
    case CSWCAnimBase::ANIM_BASE_TYPE_WIELD:
    case CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS:
        RequestSimple(pInfo);
        break;
    case CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD:
        RequestBType(pInfo);
        break;
    default:
        return 0;
    }
    return 1;
}

int CSWCCreatureAppearance::RequestHeadAppearance(CCAppearanceInfo* pInfo)
{
    CExoString sModelResRef;
    CExoString sTextureResRef;
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("MODELTYPE"), &sModelType);
    unsigned char nType = static_cast<unsigned char>(GetAnimType(sModelType));
    if (nType == 3 && pInfo->core.m_nHeadNumber != 0 &&
        pInfo->core.m_oidHeadItem == CGameObjectArray::INVALIDOBJID)
    {
        sModelResRef = "P";
        g_pRules->m_p2DArrays->m_pGender->GetCExoStringEntry(
            pInfo->core.m_nGender == GENDER_FEMALE, CExoString("GENDER"), &sModelType);
        sModelResRef = sModelResRef + sModelType;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString("RACE"), &sModelType);
        sModelResRef = sModelResRef + sModelType;
        sTextureResRef = sModelResRef;
        sModelResRef = sModelResRef + CExoString(static_cast<unsigned int>(pInfo->core.m_nPhenotype));
        sModelResRef = sModelResRef + "_HEAD";
        sTextureResRef = sTextureResRef + "0_HEAD";
        char acSuffix[4];
        FormatAppearanceSuffix(acSuffix, sizeof(acSuffix), "%03u", pInfo->core.m_nHeadNumber);
        sModelResRef = sModelResRef + acSuffix;
        sTextureResRef = sTextureResRef + acSuffix;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString("MODELTYPE"), &sModelType);
        int nAnimType = GetAnimType(sModelType);
        m_pCreature->RequestModel(CResRef(sModelResRef), CResRef(sTextureResRef), static_cast<unsigned char>(nAnimType));
    }
    return 1;
}

// The model and texture resrefs for a B-type body, from the appearance table's per-variant columns.
void CSWCCreatureAppearance::GetBTypeModelAndTextureResRef(CCAppearanceInfo* pInfo, CResRef* pModelOut, CResRef* pTextureOut)
{
    char szModelCol[7] = "ModelA";
    char szTexCol[5] = "TEXA";
#if KOTOR1  // K1's appearance.2da has model/texture columns A..J only
    unsigned char nVariant = pInfo->core.m_bTypeVariant;
    int nClamped = (nVariant < 10) ? nVariant : 10;
    unsigned int nNumber = pInfo->core.m_bTypeNumber;
    if (nNumber == 0)
        nNumber = 1;
    if (pInfo->core.m_bArmorEquipped != 0)
    {
        char cLetter = static_cast<char>(((nClamped > 1) ? nClamped : 1) | 0x40);
        szModelCol[5] = cLetter;
        szTexCol[3] = cLetter;
    }
#else
    unsigned int nNumber = pInfo->core.m_bTypeNumber;
    if (nNumber == 0)
        nNumber = 1;
    if (pInfo->core.m_bArmorEquipped != 0)
    {
        unsigned char nVariant = pInfo->core.m_bTypeVariant;
        unsigned char nClamped = (nVariant < 14) ? nVariant : 14;
        char cLetter = (nVariant == 0) ? 'A' : static_cast<char>(nClamped | 0x40);
        szModelCol[5] = cLetter;
        szTexCol[3] = cLetter;
    }
#endif
    CExoString sValue;
    int bFound;
    if (pModelOut != nullptr)
    {
        bFound = g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString(szModelCol), &sValue);
        if (bFound == 0)
            sValue = "";
        *pModelOut = sValue;
    }
    if (pTextureOut != nullptr)
    {
        bFound = g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString(szTexCol), &sValue);
        if (bFound == 0)
        {
            *pTextureOut = const_cast<char*>("");
        }
        else
        {
            CResRef rTexResRef;
            CExoString sTexResRef;
            sTexResRef.Format(const_cast<char*>("%s%02d"), sValue.CStr(), nNumber);
            rTexResRef = sTexResRef;
            if (!g_pExoResMan->Exists(rTexResRef, RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) &&
                !g_pExoResMan->Exists(rTexResRef, RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
            {
                sTexResRef.Format(const_cast<char*>("%s%02d"), sValue.CStr(), 1);
                rTexResRef = sTexResRef;
            }
            *pTextureOut = rTexResRef;
        }
    }
}

// Requests a B-type body model from the creature.
int CSWCCreatureAppearance::RequestBType(CCAppearanceInfo* pInfo)
{
    CResRef sModelResRef;
    GetBTypeModelAndTextureResRef(pInfo, &sModelResRef, nullptr);
    m_pCreature->RequestModel(sModelResRef, CResRef(""), 3);
    return 1;
}

// Reads the movement tuning and body type for this appearance and builds the body.
int CSWCCreatureAppearance::CreateBodyAppearance(CCAppearanceInfo* pInfo)
{
    unsigned char nType;
    int nResult;
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEACCl, &m_driveAccel);
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEMAXSPEED, &m_fDriveMaxSpeed);
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEANIMWALK, &m_walkSpeed);
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEANIMRUN, &m_runSpeed);
    if (m_driveAccel == 0.0f)
        m_driveAccel = 25.0f;
    if (m_fDriveMaxSpeed == 0.0f)
        m_fDriveMaxSpeed = 6.0f;
    if (m_walkSpeed == 0.0f)
        m_walkSpeed = 2.0f;
    if (m_runSpeed == 0.0f)
        m_runSpeed = 4.0f;

    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature != nullptr)
    {
        float fRateFactor = pServerCreature->GetMovementRateFactor(1);
        m_fDriveMaxSpeed = fRateFactor * m_fDriveMaxSpeed;
    }

    float fValue;
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE, &fValue);
    m_fPersonalSpace = fValue;

    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE, &sModelType);
    nType = static_cast<unsigned char>(GetAnimType(sModelType));
    switch (nType)
    {
    case CSWCAnimBase::ANIM_BASE_TYPE_WIELD:
    case CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS:
        nResult = CreateSimple(nType, pInfo);
        break;
    case CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD:
        nResult = CreateBTypeBody(CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD, pInfo);
        break;
    default:
        nResult = 0;
        break;
    }

    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HEIGHT, &fValue);
    m_fHeight = fValue;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_GROUNDTILT, reinterpret_cast<int*>(&m_bGroundTilt));

    if (nResult != 0)
    {
        if ((nType & 0xfe) == 2 && sModelType.Find(CExoString("L"), 0) == -1)
        {
            if (m_pCreature->GetAnimationBase() != nullptr)
                m_pCreature->GetAnimationBase()->SetWieldLimited(0);
            m_pCreature->SetCreatureType(CSWCAnimBase::CREATURE_TYPE_FULL);
        }
        else
        {
            if (m_pCreature->GetAnimationBase() != nullptr)
                m_pCreature->GetAnimationBase()->SetWieldLimited(1);
            m_pCreature->SetCreatureType(CSWCAnimBase::CREATURE_TYPE_LIMITED);
        }
        m_pCreature->UpdateWeaponClass();
    }
    return nResult;
}

void SwitchWeaponEvent(CAurObject* pObject, char* pEvent, void* pData);

int CSWCCreatureAppearance::CreateBTypeBody(unsigned char a, CCAppearanceInfo* pInfo)
{
    CResRef sModelResRef;
    CResRef sTextureResRef;
    CResRef sEnvMapResRef;
    CExoString sRace;
    CExoString sEnvMap;
    CExoString sUnused;

    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("RACE"), &sRace);
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("ENVMAP"), &sEnvMap);

    bool bHasEnvMap;
#if KOTOR1
    if (sEnvMap.GetLength() < 1)
#else
    if (sEnvMap.m_sString == nullptr || static_cast<int>(strlen(sEnvMap.m_sString)) < 1)
#endif
    {
        bHasEnvMap = false;
    }
    else
    {
        sEnvMapResRef = sEnvMap;
        bHasEnvMap = true;
    }

    GetBTypeModelAndTextureResRef(pInfo, &sModelResRef, &sTextureResRef);

    unsigned short nAppearanceId = pInfo->core.m_nAppearanceId;
    bool bSameAppearance = (m_nAppearanceId == nAppearanceId)
        && (m_bArmorEquipped == pInfo->core.m_bArmorEquipped)
        && (m_cachedBTypeVariant == pInfo->core.m_bTypeVariant)
        && (m_cachedBTypeNumber == pInfo->core.m_bTypeNumber)
        && (m_nGender == pInfo->core.m_nGender);

    int nResult;
    if (bSameAppearance)
    {
        nResult = 1;
    }
    else
    {
        m_cachedBTypeVariant = pInfo->core.m_bTypeVariant;
        unsigned char nBTypeNumber = pInfo->core.m_bTypeNumber;
        m_nAppearanceId = nAppearanceId;
        m_bArmorEquipped = pInfo->core.m_bArmorEquipped;
        m_cachedBTypeNumber = nBTypeNumber;
        m_nGender = pInfo->core.m_nGender;

        Gob* pOldModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
        if (pOldModel != nullptr)
            pOldModel->SetBehavior(nullptr);

        int bHadCallback = m_pCreature->ClearQuededAnimationCallback();
        m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_BODY);
        int bLoaded = m_pCreature->LoadModel(sModelResRef, CSWCAnimBase::ANIM_MODEL_BODY, a);
        if (bLoaded == 0)
        {
            char acErrorMsg[128];
            FormatAppearanceSuffix(acErrorMsg, sizeof(acErrorMsg),
                "CSWCCreatureAppearance::CreateBTypeBody(): Failed to load model '%s'.",
                sModelResRef.GetResRefStr());
            return 0;
        }

        Gob* pNewModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        if (pNewModel != nullptr)
            pNewModel->SetPosition(m_pCreature->m_position);
        if (bHadCallback != 0)
            m_pCreature->SetQuededAnimationCallback();

        if (bHasEnvMap)
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 1, 1, sEnvMapResRef);
        else
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 0, 1, CResRef("DEFAULT"));

        if (g_pAppManager->m_pServerExoApp == nullptr ||
            g_pAppManager->m_pClientExoApp->IsPlayerCreature(m_pCreature) != 0)
        {
            Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pModel->SetCanDownSample(0);
        }

        if (sTextureResRef != const_cast<char*>(""))
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), sTextureResRef, 1);

        m_bRebuildHead = 1;
        m_bRebuildTail = 1;
        m_nAppearanceId = pInfo->core.m_nAppearanceId;
        *reinterpret_cast<unsigned int*>(&m_bRebuildWings) = 1;
        SwitchWeaponEvent(nullptr, const_cast<char*>("AppearanceForceUpdate"), m_pCreature);
        if (pOldModel != nullptr)
        {
            Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pOldModel->AttachToObject(static_cast<CAurObject*>(pModel), const_cast<char*>("HeadHook"), 0);
        }
        *reinterpret_cast<unsigned int*>(&m_bAlignmentDirty) = 1;
        m_bCreatedWhilePaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        nResult = 1;
    }
    return nResult;
}

int CSWCCreatureAppearance::CreateSimple(unsigned char a, CCAppearanceInfo* pInfo)
{
    CResRef sModelResRef;
    CResRef sEnvMapResRef;
    CExoString sRace;
    CExoString sEnvMap;

    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("RACE"), &sRace);
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("ENVMAP"), &sEnvMap);

    bool bHasEnvMap;
#if KOTOR1
    if (sEnvMap.GetLength() < 1)
#else
    if (sEnvMap.m_sString == nullptr || static_cast<int>(strlen(sEnvMap.m_sString)) < 1)
#endif
    {
        bHasEnvMap = false;
    }
    else
    {
        sEnvMapResRef = sEnvMap;
        bHasEnvMap = true;
    }
    sModelResRef = sRace;

    int nResult;
    if (m_nAppearanceId == pInfo->core.m_nAppearanceId)
    {
        nResult = 1;
    }
    else
    {
        Gob* pPart = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY, 1);
        if (pPart != nullptr)
        {
            pPart->AttachToScene(nullptr);
            pPart->AttachToObject(nullptr, nullptr, 0);
        }
        pPart = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY, 1);
        if (pPart != nullptr)
        {
            pPart->AttachToScene(nullptr);
            pPart->AttachToObject(nullptr, nullptr, 0);
        }
        int bLoaded;
        m_pCreature->UnloadMonkVisuals();
        int bHadCallback = m_pCreature->ClearQuededAnimationCallback();
        m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_BODY);
        m_pCreature->UpdateMonkVisuals();
        bLoaded = m_pCreature->LoadModel(sModelResRef, CSWCAnimBase::ANIM_MODEL_BODY, a);
        if (bLoaded == 0)
        {
            return 0;
        }

        if (bHadCallback != 0)
            m_pCreature->SetQuededAnimationCallback();

        if (bHasEnvMap)
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 1, 1, sEnvMapResRef);
        else
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_BODY, 0, 1, CResRef("DEFAULT"));

        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == m_pCreature)
        {
            m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->SetCanDownSample(0);
        }

        *reinterpret_cast<CCAppearanceCore*>(this) = pInfo->core;  // idiom-exempt: not a base-class relationship — CSWCCreatureAppearance's own 0x00-0x3f fields independently mirror CCAppearanceCore's layout (see the EFFECTIVE note above); the orig itself compiles this as one aggregate copy (paired NEON ldp/stp), confirmed via bltally/dump, not per-field stores
        m_bRebuildHead = 1;
        m_bRebuildTail = 1;
        *reinterpret_cast<unsigned int*>(&m_bRebuildWings) = 1;

        CExoString sModelType;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString("MODELTYPE"), &sModelType);
        unsigned char nType = static_cast<unsigned char>(GetAnimType(sModelType));
        if ((nType & 0xfe) == 2 && sModelType.Find('L', 0) == -1)
        {
            if (m_pCreature->GetAnimationBase() != nullptr)
                m_pCreature->GetAnimationBase()->SetWieldLimited(0);
            m_pCreature->SetCreatureType(CSWCAnimBase::CREATURE_TYPE_FULL);
        }
        else
        {
            if (m_pCreature->GetAnimationBase() != nullptr)
                m_pCreature->GetAnimationBase()->SetWieldLimited(1);
            m_pCreature->SetCreatureType(CSWCAnimBase::CREATURE_TYPE_LIMITED);
        }

        if (nType != 3)
        {
            int bFound = g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                pInfo->core.m_nAppearanceId, CExoString("RaceTex"), &sEnvMap);
            if (bFound != 0)
            {
                if (g_pExoResMan->Exists(CResRef(sEnvMap), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) != 0 ||
                    g_pExoResMan->Exists(CResRef(sEnvMap), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)) != 0)
                {
                    m_pCreature->ReplaceTexture(CResRef(""), CResRef(sEnvMap), 1);
                }
            }
        }

        m_pCreature->UpdateWeaponClass();
        SwitchWeaponEvent(nullptr, const_cast<char*>("AppearanceForceUpdate"), m_pCreature);
        m_bCreatedWhilePaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        nResult = 1;
    }
    return nResult;
}

float CSWCCreatureAppearance::GetHeight()
{
    Vector pos = {};
    Quaternion rot;
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
    float fHeight;
    if (pModel == nullptr) {
        fHeight = 0.0f;
    } else {
        pModel->GetPartPosition(const_cast<char*>("HEAD_G"), &pos, &rot);
        fHeight = pos.z - m_pCreature->m_position.z;
        if (pos.z == 0.0f || fHeight < 0.0f) {
            fHeight = m_fHeight;
        }
    }
    return fHeight;
}

char CSWCCreatureAppearance::GetGenderChar()
{
    CExoString sGender;
    g_pRules->m_p2DArrays->m_pGender->GetCExoStringEntry(m_nGender, CExoString("GENDER"), &sGender);
    return sGender[0];
}

int CSWCCreatureAppearance::CreateHeadAppearance(CCAppearanceInfo* pInfo)
{
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE, &sModelType);
    if (static_cast<unsigned char>(GetAnimType(sModelType)) == 3)
        return CreateHead(3, pInfo);
    m_nHeadNumber = pInfo->core.m_nHeadNumber;
    m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_HEAD);
    return 1;
}

int CSWCCreatureAppearance::CreateHead(unsigned char a, CCAppearanceInfo* pInfo)
{
    char acSuffix[4];
    int nHeadRow;
    CResRef sModelResRef;
    CResRef sTextureResRef;
    CResRef sEnvMapResRef;
    CExoString sHeadModel;
    CExoString sHeadTexture;
    CExoString sEnvMap;

    if (pInfo->core.m_nHeadNumber == 0 && pInfo->core.m_oidHeadItem == CGameObjectArray::INVALIDOBJID)
    {
        m_pCreature->UnloadMonkVisuals();
        m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_HEAD);
        m_pCreature->UpdateMonkVisuals();
        m_oidHeadItem = pInfo->core.m_oidHeadItem;
        m_nHeadNumber = pInfo->core.m_nHeadNumber;
        return 1;
    }

    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_ENVMAP, &sEnvMap);
    bool bHasEnvMap;
    if (sEnvMap.GetLength() < 1)
    {
        bHasEnvMap = false;
    }
    else
    {
        sEnvMapResRef = sEnvMap;
        bHasEnvMap = true;
    }

    bool bHasAltTexture;
    if (a == 3)
    {
        if (pInfo->core.m_race == 0xff)
        {
            if (pInfo->core.m_bUseBackupHead == 0)
                g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                    pInfo->core.m_nAppearanceId,
                    CTwoDimArrays::COL2DA_APPEARANCETABLE_NORMALHEAD, &nHeadRow);
            else
                g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                    pInfo->core.m_nAppearanceId,
                    CTwoDimArrays::COL2DA_APPEARANCETABLE_BACKUPHEAD, &nHeadRow);
        }
        else
        {
            nHeadRow = pInfo->core.m_race;
            m_bRebuildHead = 1;
        }

        g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(
            nHeadRow, CExoString("HEAD"), &sHeadModel);
#if KOTOR1
        if (sHeadModel.IsEmpty())
            return 0;
        sModelResRef = sHeadModel;
        sTextureResRef = sHeadModel;
        bHasAltTexture = false;
#else
        if (sHeadModel.m_sString == nullptr || sHeadModel.m_sString[0] == '\0')
            return 0;
        sModelResRef = sHeadModel;

        CExoString sAltTexture;
        g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(
            nHeadRow, CExoString("ALTTEXTURE"), &sAltTexture);
        if (sAltTexture.GetLength() >= 1)
        {
            sTextureResRef = sAltTexture;
            bHasAltTexture = true;
        }
        else
        {
            sTextureResRef = sHeadModel;
            bHasAltTexture = false;
        }
#endif
    }
    else
    {
        sHeadModel = "P";
        g_pRules->m_p2DArrays->m_pGender->GetCExoStringEntry(
            pInfo->core.m_nGender == GENDER_FEMALE, CTwoDimArrays::COL2DA_GENDER_GENDER, &sEnvMap);
        sHeadModel = sHeadModel + sEnvMap;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_RACE, &sEnvMap);
        sHeadModel = sHeadModel + sEnvMap;
        sHeadTexture = sHeadModel;
        sHeadModel = sHeadModel + CExoString(static_cast<unsigned int>(pInfo->core.m_nPhenotype));
        sHeadModel = sHeadModel + "_HEAD";
        sHeadTexture = sHeadTexture + "0_HEAD";
        FormatAppearanceSuffix(acSuffix, sizeof(acSuffix), "%03u", pInfo->core.m_nHeadNumber);
        sHeadModel = sHeadModel + acSuffix;
        sHeadTexture = sHeadTexture + acSuffix;
        sModelResRef = sHeadModel;
        sTextureResRef = sHeadTexture;
        bHasAltTexture = false;
    }
#if KOTOR1  // K1 never reads a head's ALTTEXTURE, so nothing reads the flag
    K2_UNUSED(bHasAltTexture);
#endif

    if (m_bRebuildHead == 0
        && m_nAppearanceId == pInfo->core.m_nAppearanceId
        && m_nGender == pInfo->core.m_nGender
        && m_nHeadNumber == pInfo->core.m_nHeadNumber
        && m_oidHeadItem == pInfo->core.m_oidHeadItem
        && m_race == pInfo->core.m_race
        && m_bUseBackupHead == pInfo->core.m_bUseBackupHead)
    {
#if !KOTOR1
        if (bHasAltTexture &&
            (g_pExoResMan->Exists(sTextureResRef, RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) != 0 ||
             g_pExoResMan->Exists(sTextureResRef, RESTYPE_TPC, static_cast<unsigned int*>(nullptr)) != 0))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_HEAD, CResRef(""), sTextureResRef, 1);
        }
#endif
        if (bHasEnvMap)
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_HEAD, 1, 1, sEnvMapResRef);
        else
            m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_HEAD, 0, 1, CResRef("DEFAULT"));
        return 1;
    }

    m_pCreature->UnloadMonkVisuals();
    m_pCreature->UnloadModel(CSWCAnimBase::ANIM_MODEL_HEAD);
    m_pCreature->UpdateMonkVisuals();
    m_nHeadNumber = pInfo->core.m_nHeadNumber;
    m_race = pInfo->core.m_race;
    m_bUseBackupHead = pInfo->core.m_bUseBackupHead;
    if (m_pCreature->LoadModel(sModelResRef, CSWCAnimBase::ANIM_MODEL_HEAD, a) == 0)
        return 0;

    if (a == 1 || a == 3)
    {
        if (m_bRebuildHead != 0 || m_oidHeadItem != pInfo->core.m_oidHeadItem)
        {
            CSWCItem* pOldItem =
                g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidHeadItem);
            if (pOldItem != nullptr)
            {
                Gob* pOldGob = pOldItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                if (pOldGob != nullptr)
                {
                    pOldGob->AttachToObject(nullptr, nullptr, 0);
                    pOldGob->SetObjectScale(1.0f, true);
                    pOldGob->SetCanDownSample(1);
                }
                if (pOldItem->m_pArea != nullptr)
                    pOldItem->AttachToScene(nullptr);
            }
            if (pInfo->core.m_oidHeadItem != CGameObjectArray::INVALIDOBJID)
            {
                CSWCItem* pNewItem =
                    g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(pInfo->core.m_oidHeadItem);
                if (pNewItem == nullptr)
                    return 0;
                Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
                Gob* pNewGob = pNewItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                if (pNewGob != nullptr)
                {
                    pNewGob->AttachToObject(static_cast<CAurObject*>(pModel), const_cast<char*>("GoggleHook"), 0);
                    pNewGob->PlayAnimation(const_cast<char*>("default"), 1.0f, 0, 0.0f);
                }
            }
        }
        m_pCreature->GetAnimationBase()->SetHeadItem(pInfo->core.m_oidHeadItem);
    }

    m_oidHeadItem = pInfo->core.m_oidHeadItem;
#if !KOTOR1
    if (bHasAltTexture &&
        (g_pExoResMan->Exists(sTextureResRef, RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) != 0 ||
         g_pExoResMan->Exists(sTextureResRef, RESTYPE_TPC, static_cast<unsigned int*>(nullptr)) != 0))
    {
        m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_HEAD, CResRef(""), sTextureResRef, 1);
    }
#endif
    if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == m_pCreature)
    {
        Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
        pModel->SetCanDownSample(0);
    }
    if (bHasEnvMap)
        m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_HEAD, 1, 1, sEnvMapResRef);
    else
        m_pCreature->SetEnvMap(CSWCAnimBase::ANIM_MODEL_HEAD, 0, 1, CResRef("DEFAULT"));
    *reinterpret_cast<unsigned int*>(&m_bAlignmentDirty) = 1;
    m_bRebuildHead = 0;
    m_bCreatedWhilePaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    return 1;
}

int CSWCCreatureAppearance::RequestSimple(CCAppearanceInfo* pInfo)
{
    CExoString sRace;
    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("RACE"), &sRace);
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pInfo->core.m_nAppearanceId, CExoString("MODELTYPE"), &sModelType);
    int nAnimType = GetAnimType(sModelType);
    m_pCreature->RequestModel(CResRef(sRace), CResRef(""), static_cast<unsigned char>(nAnimType));
    return 1;
}

int CSWCCreatureAppearance::HeadExists(CCAppearanceInfo* pInfo)
{
    int nRow;
    CExoString sModelResRef;
    CExoString sPart;
    int nResult;
    if (pInfo->core.m_nAppearanceId == 3) {
        if (pInfo->core.m_race == 0xff) {
            if (pInfo->core.m_bUseBackupHead == 0) {
                g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(3, CExoString("NORMALHEAD"), &nRow);
            } else {
                g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(3, CExoString("BACKUPHEAD"), &nRow);
            }
        } else {
            nRow = pInfo->core.m_race;
            m_bRebuildHead = 1;
        }
        g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEAD"), &sModelResRef);
        nResult = g_pExoResMan->Exists(CResRef(sModelResRef), RESTYPE_MDL, static_cast<unsigned int*>(nullptr));
    } else {
        sModelResRef = "P";
        g_pRules->m_p2DArrays->m_pGender->GetCExoStringEntry(
            pInfo->core.m_nGender == GENDER_FEMALE, CExoString("GENDER"), &sPart);
        sModelResRef = sModelResRef + sPart;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pInfo->core.m_nAppearanceId, CExoString("RACE"), &sPart);
        char acSuffix[4];
        sModelResRef = sModelResRef + sPart + "0_HEAD";
        FormatAppearanceSuffix(acSuffix, sizeof(acSuffix), "%03u", pInfo->core.m_nHeadNumber);
        sModelResRef = sModelResRef + acSuffix;
        nResult = g_pExoResMan->Exists(CResRef(sModelResRef), RESTYPE_MDL, static_cast<unsigned int*>(nullptr));
    }
    return nResult;
}

void CSWCCreatureAppearance::SetAppearanceAlignment(short a)
{
#if KOTOR1
    if (static_cast<int>(m_bAlignmentDirty) == 0 && m_cachedAlignment == static_cast<unsigned short>(a))
        return;
    m_cachedAlignment = static_cast<unsigned short>(a);
    if (m_pCreature == nullptr)
        return;
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 0);
    if (pModel == nullptr)
        return;

    CExoString sHeadTexCol, sTexA;
    C2DA* pAppearance = g_pRules->m_p2DArrays->m_pAppearance;
    if (pAppearance == nullptr)
        return;

    *reinterpret_cast<unsigned int*>(&m_bAlignmentDirty) = 0;
    unsigned int nRow;
    if (m_race != 0xff)
    {
        m_bRebuildHead = 1;
        nRow = m_race;
    }
    else if (m_bUseBackupHead == 0)
    {
        pAppearance->GetINTEntry(m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_NORMALHEAD, reinterpret_cast<int*>(&nRow));
    }
    else
    {
        pAppearance->GetINTEntry(m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_BACKUPHEAD, reinterpret_cast<int*>(&nRow));
    }

    int bHeadTexFound = 0;
    int bFoundA = 0;
    if (a <= 40)
    {
        if (a <= 10)
            bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVVVE"), &sHeadTexCol);
        else if (a <= 20)
            bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVVE"), &sHeadTexCol);
        else if (a <= 30)
            bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVE"), &sHeadTexCol);
        else
            bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXE"), &sHeadTexCol);
        if (m_bArmorEquipped == 0)
            bFoundA = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXAEVIL"), &sTexA);
    }

    if (bHeadTexFound)
    {
        if (g_pExoResMan->Exists(CResRef(sHeadTexCol), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sHeadTexCol), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_HEAD, CResRef(""), CResRef(sHeadTexCol), 1);
        }
    }
    else
    {
        m_pCreature->RestoreTexture(CSWCAnimBase::ANIM_MODEL_HEAD);
    }

    if (bFoundA)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexA.CStr());
        sTexA = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexA), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexA), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexA), 1);
        }
    }
    else
    {
        CResRef rTex;
        GetBTypeModelAndTextureResRef(reinterpret_cast<CCAppearanceInfo*>(this), nullptr, &rTex);  // idiom-exempt: see the K2 arm
        if (rTex != const_cast<char*>(""))
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), rTex, 1);

        if (g_pAppManager->m_pServerExoApp == nullptr ||
            g_pAppManager->m_pClientExoApp->IsPlayerCreature(m_pCreature) != 0)
        {
            Gob* pBodyModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pBodyModel->SetCanDownSample(0);
        }
    }
#else
    if (static_cast<int>(m_bAlignmentDirty) == 0 && m_cachedAlignment == static_cast<unsigned short>(a))
        return;
    m_cachedAlignment = static_cast<unsigned short>(a);
    if (m_pCreature == nullptr)
        return;

    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature != nullptr &&
        pServerCreature->m_sTag.CompareNoCase("handmaiden") != 0 &&
        m_bArmorEquipped != 0)
    {
        if (m_cachedBTypeVariant == 2)
        {
            if (a <= 40)
            {
                if (m_nAppearanceId != APPEARANCE_HANDMAIDEN_CLOTHING_EVIL)
                    pServerCreature->Disguise(APPEARANCE_HANDMAIDEN_CLOTHING_EVIL, nullptr, 0, 1);
            }
            else
            {
                if (m_nAppearanceId != APPEARANCE_HANDMAIDEN)
                    pServerCreature->Disguise(APPEARANCE_HANDMAIDEN, nullptr, 0, 1);
            }
            return;
        }
        if (m_nAppearanceId != APPEARANCE_HANDMAIDEN_HEAD)
            pServerCreature->Disguise(APPEARANCE_HANDMAIDEN_HEAD, nullptr, 0, 1);
    }

    if (m_pCreature == nullptr)
        return;
    Gob* pModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 0);
    if (pModel == nullptr)
        return;

    CExoString sHeadTexCol, sTexA, sTexB, sTexI, sTexL, sTexM;
    C2DA* pAppearance = g_pRules->m_p2DArrays->m_pAppearance;
    if (pAppearance == nullptr)
        return;

    *reinterpret_cast<unsigned int*>(&m_bAlignmentDirty) = 0;
    unsigned int nRow;
    if (m_race != 0xff)
    {
        m_bRebuildHead = 1;
        nRow = m_race;
    }
    else if (m_bUseBackupHead == 0)
    {
        pAppearance->GetINTEntry(m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_NORMALHEAD, reinterpret_cast<int*>(&nRow));
    }
    else
    {
        pAppearance->GetINTEntry(m_nAppearanceId, CTwoDimArrays::COL2DA_APPEARANCETABLE_BACKUPHEAD, reinterpret_cast<int*>(&nRow));
    }

    int bHeadTexFound = 0;
    if (a <= 10)
    {
        bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVVVE"), &sHeadTexCol);
    }
    else if (a <= 20)
    {
        bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVVE"), &sHeadTexCol);
    }
    else if (a <= 30)
    {
        bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXVE"), &sHeadTexCol);
    }
    else if (a <= 40)
    {
        bHeadTexFound = g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("HEADTEXE"), &sHeadTexCol);
    }

    int bFoundA = 0, bFoundB = 0, bFoundI = 0, bFoundL = 0, bFoundM = 0;
    if (m_bArmorEquipped != 0)
    {
        unsigned char nVariant = m_cachedBTypeVariant;
        if (nVariant == 12)
        {
            if (a <= 40)
                bFoundL = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXLEVIL"), &sTexL);
        }
        else if (nVariant == 9)
        {
            if (a <= 40)
                bFoundI = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXIEVIL"), &sTexI);
        }
        else if (nVariant == 2)
        {
            if (a <= 40)
                bFoundB = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXBEVIL"), &sTexB);
        }
        else if (a <= 40 && nVariant == 14)
        {
#ifdef K2_FIX_ENGINE_BUGS
            bFoundM = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXNEVIL"), &sTexM);
#else
            bFoundM = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXMEVIL"), &sTexM);
#endif
        }
    }
    else if (a <= 40)
    {
        bFoundA = pAppearance->GetCExoStringEntry(m_nAppearanceId, CExoString("TEXAEVIL"), &sTexA);
    }

    if (bHeadTexFound)
    {
        if (g_pExoResMan->Exists(CResRef(sHeadTexCol), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sHeadTexCol), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_HEAD, CResRef(""), CResRef(sHeadTexCol), 1);
        }
    }
    else
    {
        CExoString sAltTex;
        g_pRules->m_p2DArrays->m_pHeads->GetCExoStringEntry(nRow, CExoString("ALTTEXTURE"), &sAltTex);
        if (sAltTex.GetLength() < 1)
        {
            m_pCreature->RestoreTexture(CSWCAnimBase::ANIM_MODEL_HEAD);
        }
        else
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_HEAD, CResRef(""), CResRef(sAltTex), 1);
        }
    }

    if (bFoundA)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexA.CStr());
        sTexA = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexA), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexA), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexA), 1);
        }
    }
    else if (bFoundB)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexB.CStr());
        sTexB = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexB), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexB), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexB), 1);
        }
    }
    else if (bFoundI)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexI.CStr());
        sTexI = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexI), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexI), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexI), 1);
        }
    }
    else if (bFoundL)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexL.CStr());
        sTexL = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexL), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexL), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexL), 1);
        }
    }
    else if (bFoundM)
    {
        char acBuf[32];
        FormatAppearanceSuffix(acBuf, sizeof(acBuf), "%s01", sTexM.CStr());
        sTexM = acBuf;
        if (g_pExoResMan->Exists(CResRef(sTexM), RESTYPE_TGA, static_cast<unsigned int*>(nullptr)) ||
            g_pExoResMan->Exists(CResRef(sTexM), RESTYPE_TPC, static_cast<unsigned int*>(nullptr)))
        {
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), CResRef(sTexM), 1);
        }
    }
    else
    {
        CResRef rTex;
        GetBTypeModelAndTextureResRef(reinterpret_cast<CCAppearanceInfo*>(this), nullptr, &rTex);  // idiom-exempt: not a base-class relationship — reusing this's own 0x00-0x3f cache fields as a CCAppearanceInfo (layout confirmed identical, see CreateSimple's note above)
        if (rTex != const_cast<char*>(""))
            m_pCreature->ReplaceTexture(CSWCAnimBase::ANIM_MODEL_BODY, CResRef(""), rTex, 1);

        if (g_pAppManager->m_pServerExoApp == nullptr ||
            g_pAppManager->m_pClientExoApp->IsPlayerCreature(m_pCreature) != 0)
        {
            Gob* pBodyModel = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pBodyModel->SetCanDownSample(0);
        }
    }
#endif
}

void CSWCCreatureAppearance::ForceAnimate() {
    if (!m_bCreatedWhilePaused) return;

    auto forceAnim = [](Gob* pGob) {
        if (!pGob) return;
        pGob->EnableForceAnimations();
        pGob->CAurObject::AnimateExternal(1.0f);
        pGob->DisableForceAnimations();
    };

    forceAnim(m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1));

    Gob* pHead = m_pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
    CSWCItem* pItem;
    if (pHead) {
        forceAnim(pHead);

        pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_oidHeadItem);
        if (pItem) {
            CSWCObject* pItemObj = static_cast<CSWCObject*>(pItem);
            forceAnim(pItemObj->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1));
        }
    }

    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON));
    if (pItem) {
        CSWCObject* pItemObj = static_cast<CSWCObject*>(pItem);
        forceAnim(pItemObj->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1));
    }

    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_pCreature->GetEquippedItemID(MSG_SLOT_LEFT_WEAPON));
    if (pItem) {
        CSWCObject* pItemObj = static_cast<CSWCObject*>(pItem);
        Gob* pModel = pItemObj->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        if (pModel) {
            pModel->EnableForceAnimations();
            pModel->CAurObject::AnimateExternal(1.0f);
            pModel->DisableForceAnimations();
        }
    }
}
