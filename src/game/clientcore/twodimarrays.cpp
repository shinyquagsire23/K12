#include "k2stdint.h"
#include "twodimarrays.h"
#include <stddef.h>

#if KOTOR1
int CTwoDimArrays::COL2DA_REGENERATION_HEALTHREGEN = -1;
int CTwoDimArrays::COL2DA_REGENERATION_FORCEREGEN = -1;
#endif
int CTwoDimArrays::COL2DA_APPEARANCETABLE_ABORTONPARRY = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_BACKUPHEAD = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_CAMERAHEIGHTOFFSET = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_CAMERASPACE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_CREPERSPACE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_DISABLEINJUREDANIM = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEACCl = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEMAXSPEED = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEANIMWALK = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_DRIVEANIMRUN = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_ENVMAP = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_FOOTSTEPTYPE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_GROUNDTILT = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_H = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_V = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HEADBONE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HEADTRACK = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HEIGHT = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HITDIST = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_HITRADIUS = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_NORMALHEAD = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_RACE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_RUNDIST = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_SOUNDAPPTYPE = -1;
int CTwoDimArrays::COL2DA_APPEARANCETABLE_WALKDIST = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_FALLDIRT = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_FALLMETAL = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_FALLHARD = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_FALLWATER = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_WEAPON = -1;
int CTwoDimArrays::COL2DA_APPEARANCESOUNDS_ARMORTYPE = -1;
int CTwoDimArrays::COL2DA_GENDER_GENDER = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_FIREFORGET = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_OVERLAY = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_LOOPING = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_STATIONARY = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_PAUSE = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_WALKING = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_RUNNING = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_DAMAGE = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_PARRY = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_DIALOG = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_NAME = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_DODGE = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_ATTACK = -1;
int CTwoDimArrays::COL2DA_ANIMATIONS_HIDEEQUIPPEDITEMS = -1;
int CTwoDimArrays::COL2DA_FORCEADJUST_GOODCOST = -1;
int CTwoDimArrays::COL2DA_FORCEADJUST_EVILCOST = -1;
int CTwoDimArrays::COL2DA_DIALOGANIMATIONS_FIREFORGET = -1;
int CTwoDimArrays::COL2DA_DIALOGANIMATIONS_DIALOG = -1;
int CTwoDimArrays::COL2DA_DIALOGANIMATIONS_LOOPING = -1;
int CTwoDimArrays::COL2DA_DIALOGANIMATIONS_OVERLAY = -1;
int CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_ROLLING = -1;
int CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_PITCHOFFSET = -1;
int CTwoDimArrays::COL2DA_WEAPONSOUNDS_PITCHOFFSET = -1;
int CTwoDimArrays::COL2DA_SURFACEMATERIAL_NAME = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_HEADCON_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_IMPACT_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_M_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_S_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_L_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_H_NODE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_LOWQUALITY = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_ORIENTWITHGROUND = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKETYPE = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDURATION = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDELAY = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_SOUNDIMPACT = -1;
int CTwoDimArrays::COL2DA_VISUALEFFECT_SOUNDDURATION = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_DURATIONVFX = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_ORIENTWITHGROUND = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_SHAPE = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_RADIUS = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_WIDTH = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_LENGTH = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODELMIN01 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODEL01 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODELMIN02 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODEL02 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODELMIN03 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_MODEL03 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_NUMACT01 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_NUMACT02 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_NUMACT03 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_DURATION01 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_DURATION02 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_DURATION03 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT01 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT02 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT03 = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOT = -1;
int CTwoDimArrays::COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOTPERCENTAGE = -1;
int CTwoDimArrays::COL2DA_CREATURESPEED_RUNRATE = -1;
int CTwoDimArrays::COL2DA_CREATURESPEED_WALKRATE = -1;
int CTwoDimArrays::COL2DA_DOORTYPES_SOUNDAPPTYPE = -1;
int CTwoDimArrays::COL2DA_GENERICDOORS_SOUNDAPPTYPE = -1;
int CTwoDimArrays::COL2DA_PLACEABLES_SOUNDAPPTYPE = -1;
int CTwoDimArrays::COL2DA_PLACEABLES_STRREF = -1;
int CTwoDimArrays::COL2DA_PLACEABLES_IGNORESTATICHITCHECK = -1;
int CTwoDimArrays::COL2DA_PLACEABLESOUNDS_ARMORTYPE = -1;
int CTwoDimArrays::COL2DA_PLACEABLESOUNDS_LOCKED = -1;
int CTwoDimArrays::COL2DA_PLACEABLESOUNDS_OPENED = -1;
int CTwoDimArrays::COL2DA_PLACEABLESOUNDS_CLOSED = -1;
int CTwoDimArrays::COL2DA_PLACEABLESOUNDS_DESTROYED = -1;

#include "port/libcstring.h"
extern unsigned char g_nClientServerMode;

CTwoDimArrays::CTwoDimArrays()
{
#if KOTOR1
    // K1 zeroes the slots individually and leaves m_pBindableKey, m_pForceAdjust, m_pGameEffects
    // and m_pNPC unset.
    m_paIPRPCostTables = 0;
    m_paIPRPParamTables = 0;
    m_nIPRPCostTableCount = 0;
    m_nIPRPParamTableCount = 0;
    m_pAppearance = 0;
    m_pGender = 0;
    m_pSurfaceMaterial = 0;
    m_pAreaTransition = 0;
    m_pPersistentVisualEffect = 0;
    m_pCreatureSpeed = 0;
    m_pDoorTypes = 0;
    m_pGenericDoors = 0;
    m_pPlaceables = 0;
    m_pIPRPMelee = 0;
    m_pItemPropDef = 0;
    m_pPortrait = 0;
    m_pAnimations = 0;
    m_pDialogAnimations = 0;
    m_pHeads = 0;
    m_pLightColor = 0;
    m_pCursor = 0;
    m_pAmbientMusic = 0;
    m_pAmbientSound = 0;
    m_pFootstepSounds = 0;
    m_pAppearanceSounds = 0;
    m_pWeaponSounds = 0;
    m_pAmmunitionTypes = 0;
    m_pKeymap = 0;
    m_pPlaceableSounds = 0;
    m_pCameraStyle = 0;
    m_pCombatAnimations = 0;
    m_pDifficultyOptions = 0;
    m_pGamma = 0;
    m_pStateScripts = 0;
    m_pVisualEffect = 0;
    m_pTraps = 0;
    m_pPoison = 0;
    m_pDisease = 0;
    m_pRepAdjustments = 0;
    m_pFractionalCR = 0;
    m_pExcitedDuration = 0;
    m_pRegeneration = 0;
    m_pEncounterDifficulty = 0;
    m_pIPRPMonsterDamage = 0;
    m_pIPRPDamage = 0;
    m_pIPRPBonusCost = 0;
    m_pIPRPSRCost = 0;
    m_pIPRPNeg5Cost = 0;
    m_pXpBase = 0;
    m_pRanges = 0;
    m_pOnHit = 0;
    m_pOnHitDuration = 0;
    m_pDamageHitVisual = 0;
    m_pBodyBag = 0;
    m_pRepute = 0;
    m_pWeaponDischarge = 0;
    m_pDroidDischarge = 0;
    m_pPlotXP = 0;
    m_pSpells = 0;
    m_pFeat = 0;
    m_pFormations = 0;
    m_pGrenadeSound = 0;
    m_pPlanetary = 0;
    m_pFeedBackText = 0;
    m_pForceShields = 0;
    m_pVideoEffects = 0;
    m_pItemValues = 0;
    m_pTutorial = 0;
    m_pMovies = 0;
    m_pEffectIcon = 0;
    m_pRemoveFXOnDeath = 0;
    m_pTexPacks = 0;
#else
    m_paIPRPCostTables = 0;
    m_nIPRPCostTableCount = 0;
    m_paIPRPParamTables = 0;
    m_nIPRPParamTableCount = 0;
    // Zeroes every table slot from m_pAppearance to the end of the class.
    memset(&this->m_pAppearance, 0, sizeof(CTwoDimArrays) - offsetof(CTwoDimArrays, m_pAppearance));
    __asm__ __volatile__("" ::: "memory");  // idiom-exempt: CTwoDimArrays::CTwoDimArrays MATCH (no barrier: `b memset`, -12 B)
#endif
}

CTwoDimArrays::~CTwoDimArrays()
{
#if KOTOR1
    // K1 deletes the tables and both pointer arrays here; K2 leaves them to UnLoad2DArrays.
    delete m_pRemoveFXOnDeath;
    m_pRemoveFXOnDeath = 0;
    delete m_pEffectIcon;
    m_pEffectIcon = 0;
    delete m_pAppearance;
    m_pAppearance = 0;
    delete m_pGender;
    m_pGender = 0;
    delete m_pSurfaceMaterial;
    m_pSurfaceMaterial = 0;
    delete m_pStateScripts;
    m_pStateScripts = 0;
    delete m_pVisualEffect;
    m_pVisualEffect = 0;
    delete m_pAreaTransition;
    m_pAreaTransition = 0;
    delete m_pTraps;
    m_pTraps = 0;
    delete m_pPersistentVisualEffect;
    m_pPersistentVisualEffect = 0;
    delete m_pCreatureSpeed;
    m_pCreatureSpeed = 0;
    delete m_pDoorTypes;
    m_pDoorTypes = 0;
    delete m_pPlaceables;
    m_pPlaceables = 0;
    delete m_pGenericDoors;
    m_pGenericDoors = 0;
    delete m_pPoison;
    m_pPoison = 0;
    delete m_pDisease;
    m_pDisease = 0;
    delete m_pRepAdjustments;
    m_pRepAdjustments = 0;
    delete m_pFractionalCR;
    m_pFractionalCR = 0;
    delete m_pEncounterDifficulty;
    m_pEncounterDifficulty = 0;
    delete m_pExcitedDuration;
    m_pExcitedDuration = 0;
    delete m_pRegeneration;
    m_pRegeneration = 0;
    delete m_pLightColor;
    m_pLightColor = 0;
    delete m_pIPRPMelee;
    m_pIPRPMelee = 0;
    delete m_pXpBase;
    m_pXpBase = 0;
    delete m_pCursor;
    m_pCursor = 0;
    delete m_pIPRPMonsterDamage;
    m_pIPRPMonsterDamage = 0;
    delete m_pIPRPDamage;
    m_pIPRPDamage = 0;
    delete m_pIPRPBonusCost;
    m_pIPRPBonusCost = 0;
    delete m_pIPRPSRCost;
    m_pIPRPSRCost = 0;
    delete m_pIPRPNeg5Cost;
    m_pIPRPNeg5Cost = 0;
    delete m_pRanges;
    m_pRanges = 0;
    delete m_pItemPropDef;
    m_pItemPropDef = 0;
    delete[] m_paIPRPCostTables;
    m_paIPRPCostTables = 0;
    m_nIPRPCostTableCount = 0;
    delete[] m_paIPRPParamTables;
    m_paIPRPParamTables = 0;
    m_nIPRPParamTableCount = 0;
    delete m_pAmbientMusic;
    m_pAmbientMusic = 0;
    delete m_pAmbientSound;
    m_pAmbientSound = 0;
    delete m_pFootstepSounds;
    m_pFootstepSounds = 0;
    delete m_pAppearanceSounds;
    m_pAppearanceSounds = 0;
    delete m_pWeaponSounds;
    m_pWeaponSounds = 0;
    delete m_pAmmunitionTypes;
    m_pAmmunitionTypes = 0;
    delete m_pKeymap;
    m_pKeymap = 0;
    delete m_pBindableKey;
    m_pBindableKey = 0;
    delete m_pOnHit;
    m_pOnHit = 0;
    delete m_pOnHitDuration;
    m_pOnHitDuration = 0;
    delete m_pDamageHitVisual;
    m_pDamageHitVisual = 0;
    delete m_pPlaceableSounds;
    m_pPlaceableSounds = 0;
    delete m_pBodyBag;
    m_pBodyBag = 0;
    delete m_pPortrait;
    m_pPortrait = 0;
    delete m_pForceAdjust;
    m_pForceAdjust = 0;
    delete m_pGameEffects;
    m_pGameEffects = 0;
    delete m_pCameraStyle;
    m_pCameraStyle = 0;
    delete m_pRepute;
    m_pRepute = 0;
    delete m_pWeaponDischarge;
    m_pWeaponDischarge = 0;
    delete m_pDroidDischarge;
    m_pDroidDischarge = 0;
    delete m_pPlotXP;
    m_pPlotXP = 0;
    delete m_pAnimations;
    m_pAnimations = 0;
    delete m_pCombatAnimations;
    m_pCombatAnimations = 0;
    delete m_pDialogAnimations;
    m_pDialogAnimations = 0;
    delete m_pNPC;
    m_pNPC = 0;
    delete m_pSpells;
    m_pSpells = 0;
    delete m_pFeat;
    m_pFeat = 0;
    delete m_pFormations;
    m_pFormations = 0;
    delete m_pGrenadeSound;
    m_pGrenadeSound = 0;
    delete m_pPlanetary;
    m_pPlanetary = 0;
    delete m_pFeedBackText;
    m_pFeedBackText = 0;
    delete m_pForceShields;
    m_pForceShields = 0;
    delete m_pHeads;
    m_pHeads = 0;
    delete m_pItemValues;
    m_pItemValues = 0;
    delete m_pVideoEffects;
    m_pVideoEffects = 0;
    delete m_pTutorial;
    m_pTutorial = 0;
    delete m_pDifficultyOptions;
    m_pDifficultyOptions = 0;
    delete m_pGamma;
    m_pGamma = 0;
    delete m_pMovies;
    m_pMovies = 0;
    delete m_pTexPacks;
    m_pTexPacks = 0;
#else
    UnLoad2DArrays();
#endif
}

bool CTwoDimArrays::UnLoad2DArrays()
{
    if (m_paIPRPCostTables != 0) {
        C2DA** paTables = m_paIPRPCostTables;
        unsigned char count = m_nIPRPCostTableCount;
        if (count == 0) {
            operator delete[](paTables);
        } else {
            uint64_t i = 0;
            for (;;) {
                C2DA* pTable = paTables[i];
                if (pTable != 0) {
                    pTable->Unload2DArray();
                    paTables = m_paIPRPCostTables;
                    C2DA* pElem = paTables[i];
                    if (pElem != 0) {
                        delete pElem;
                        paTables = m_paIPRPCostTables;
                    }
                    paTables[i] = 0;
                    paTables = m_paIPRPCostTables;
                    count = m_nIPRPCostTableCount;
                }
                i++;
                if (count <= i) break;
            }
            if (paTables != 0) {
                operator delete[](paTables);
            }
        }
        m_paIPRPCostTables = 0;
    }

    if (m_paIPRPParamTables != 0) {
        C2DA** paTables = m_paIPRPParamTables;
        unsigned char count = m_nIPRPParamTableCount;
        if (count == 0) {
            operator delete[](paTables);
        } else {
            uint64_t i = 0;
            for (;;) {
                C2DA* pTable = paTables[i];
                if (pTable != 0) {
                    pTable->Unload2DArray();
                    paTables = m_paIPRPParamTables;
                    C2DA* pElem = paTables[i];
                    if (pElem != 0) {
                        delete pElem;
                        paTables = m_paIPRPParamTables;
                    }
                    paTables[i] = 0;
                    paTables = m_paIPRPParamTables;
                    count = m_nIPRPParamTableCount;
                }
                i++;
                if (count <= i) break;
            }
            if (paTables != 0) {
                operator delete[](paTables);
            }
        }
        m_paIPRPParamTables = 0;
    }

#if KOTOR1
    if (m_pRemoveFXOnDeath != 0) {
        m_pRemoveFXOnDeath->Unload2DArray();
    }
    if (m_pEffectIcon != 0) {
        m_pEffectIcon->Unload2DArray();
    }
    if (m_pAppearance != 0) {
        m_pAppearance->Unload2DArray();
    }
    if (m_pGender != 0) {
        m_pGender->Unload2DArray();
    }
    if (m_pSurfaceMaterial != 0) {
        m_pSurfaceMaterial->Unload2DArray();
    }
    if (m_pStateScripts != 0) {
        m_pStateScripts->Unload2DArray();
    }
    if (m_pVisualEffect != 0) {
        m_pVisualEffect->Unload2DArray();
    }
    if (m_pAreaTransition != 0) {
        m_pAreaTransition->Unload2DArray();
    }
    if (m_pTraps != 0) {
        m_pTraps->Unload2DArray();
    }
    if (m_pPersistentVisualEffect != 0) {
        m_pPersistentVisualEffect->Unload2DArray();
    }
    if (m_pCreatureSpeed != 0) {
        m_pCreatureSpeed->Unload2DArray();
    }
    if (m_pDoorTypes != 0) {
        m_pDoorTypes->Unload2DArray();
    }
    if (m_pPlaceables != 0) {
        m_pPlaceables->Unload2DArray();
    }
    if (m_pGenericDoors != 0) {
        m_pGenericDoors->Unload2DArray();
    }
    if (m_pPoison != 0) {
        m_pPoison->Unload2DArray();
    }
    if (m_pDisease != 0) {
        m_pDisease->Unload2DArray();
    }
    if (m_pRepAdjustments != 0) {
        m_pRepAdjustments->Unload2DArray();
    }
    if (m_pFractionalCR != 0) {
        m_pFractionalCR->Unload2DArray();
    }
    if (m_pEncounterDifficulty != 0) {
        m_pEncounterDifficulty->Unload2DArray();
    }
    if (m_pExcitedDuration != 0) {
        m_pExcitedDuration->Unload2DArray();
    }
    if (m_pRegeneration != 0) {
        m_pRegeneration->Unload2DArray();
    }
    if (m_pLightColor != 0) {
        m_pLightColor->Unload2DArray();
    }
    if (m_pIPRPMelee != 0) {
        m_pIPRPMelee->Unload2DArray();
    }
    if (m_pItemPropDef != 0) {
        m_pItemPropDef->Unload2DArray();
    }
    if (m_pXpBase != 0) {
        m_pXpBase->Unload2DArray();
    }
    if (m_pCursor != 0) {
        m_pCursor->Unload2DArray();
    }
    if (m_pIPRPMonsterDamage != 0) {
        m_pIPRPMonsterDamage->Unload2DArray();
    }
    if (m_pIPRPDamage != 0) {
        m_pIPRPDamage->Unload2DArray();
    }
    if (m_pIPRPBonusCost != 0) {
        m_pIPRPBonusCost->Unload2DArray();
    }
    if (m_pIPRPSRCost != 0) {
        m_pIPRPSRCost->Unload2DArray();
    }
    if (m_pIPRPNeg5Cost != 0) {
        m_pIPRPNeg5Cost->Unload2DArray();
    }
    if (m_pAmbientMusic != 0) {
        m_pAmbientMusic->Unload2DArray();
    }
    if (m_pAmbientSound != 0) {
        m_pAmbientMusic->Unload2DArray();
    }
    if (m_pFootstepSounds != 0) {
        m_pFootstepSounds->Unload2DArray();
    }
    if (m_pAppearanceSounds != 0) {
        m_pAppearanceSounds->Unload2DArray();
    }
    if (m_pWeaponSounds != 0) {
        m_pWeaponSounds->Unload2DArray();
    }
    if (m_pAmmunitionTypes != 0) {
        m_pAmmunitionTypes->Unload2DArray();
    }
    if (m_pKeymap != 0) {
        m_pKeymap->Unload2DArray();
    }
    if (m_pBindableKey != 0) {
        m_pBindableKey->Unload2DArray();
    }
    if (m_pOnHit != 0) {
        m_pOnHit->Unload2DArray();
    }
    if (m_pOnHitDuration != 0) {
        m_pOnHitDuration->Unload2DArray();
    }
    if (m_pDamageHitVisual != 0) {
        m_pDamageHitVisual->Unload2DArray();
    }
    if (m_pPlaceableSounds != 0) {
        m_pPlaceableSounds->Unload2DArray();
    }
    if (m_pBodyBag != 0) {
        m_pBodyBag->Unload2DArray();
    }
    if (m_pPortrait != 0) {
        m_pPortrait->Unload2DArray();
    }
    if (m_pCameraStyle != 0) {
        m_pCameraStyle->Unload2DArray();
    }
    if (m_pRepute != 0) {
        m_pRepute->Unload2DArray();
    }
    if (m_pPlotXP != 0) {
        m_pPlotXP->Unload2DArray();
    }
    if (m_pNPC != 0) {
        m_pNPC->Unload2DArray();
    }
    if (m_pSpells != 0) {
        m_pSpells->Unload2DArray();
    }
    if (m_pFeat != 0) {
        m_pFeat->Unload2DArray();
    }
    if (m_pFormations != 0) {
        m_pFormations->Unload2DArray();
    }
    if (m_pGrenadeSound != 0) {
        m_pGrenadeSound->Unload2DArray();
    }
    if (m_pPlanetary != 0) {
        m_pPlanetary->Unload2DArray();
    }
    if (m_pFeedBackText != 0) {
        m_pFeedBackText->Unload2DArray();
    }
    if (m_pItemValues != 0) {
        m_pItemValues->Unload2DArray();
    }
    if (m_pVideoEffects != 0) {
        m_pVideoEffects->Unload2DArray();
    }
    if (m_pTutorial != 0) {
        m_pTutorial->Unload2DArray();
    }
    if (m_pDifficultyOptions != 0) {
        m_pDifficultyOptions->Unload2DArray();
    }
    if (m_pGamma != 0) {
        m_pGamma->Unload2DArray();
    }
    if (m_pTexPacks != 0) {
        m_pTexPacks->Unload2DArray();
    }
#else
    if (m_pAppearance != 0) {
        m_pAppearance->Unload2DArray();
        delete m_pAppearance;
        m_pAppearance = 0;
    }
    if (m_pGender != 0) {
        m_pGender->Unload2DArray();
        delete m_pGender;
        m_pGender = 0;
    }
    if (m_pSurfaceMaterial != 0) {
        m_pSurfaceMaterial->Unload2DArray();
        delete m_pSurfaceMaterial;
        m_pSurfaceMaterial = 0;
    }
    if (m_pVisualEffect != 0) {
        m_pVisualEffect->Unload2DArray();
        delete m_pVisualEffect;
        m_pVisualEffect = 0;
    }
    if (m_pPersistentVisualEffect != 0) {
        m_pPersistentVisualEffect->Unload2DArray();
        delete m_pPersistentVisualEffect;
        m_pPersistentVisualEffect = 0;
    }
    if (m_pCreatureSpeed != 0) {
        m_pCreatureSpeed->Unload2DArray();
        delete m_pCreatureSpeed;
        m_pCreatureSpeed = 0;
    }
    if (m_pDoorTypes != 0) {
        m_pDoorTypes->Unload2DArray();
        delete m_pDoorTypes;
        m_pDoorTypes = 0;
    }
    if (m_pGenericDoors != 0) {
        m_pGenericDoors->Unload2DArray();
        delete m_pGenericDoors;
        m_pGenericDoors = 0;
    }
    if (m_pPlaceables != 0) {
        m_pPlaceables->Unload2DArray();
        delete m_pPlaceables;
        m_pPlaceables = 0;
    }
    if (m_pAreaTransition != 0) {
        m_pAreaTransition->Unload2DArray();
        delete m_pAreaTransition;
        m_pAreaTransition = 0;
    }
    if (m_pIPRPMelee != 0) {
        m_pIPRPMelee->Unload2DArray();
        delete m_pIPRPMelee;
        m_pIPRPMelee = 0;
    }
    if (m_pItemPropDef != 0) {
        m_pItemPropDef->Unload2DArray();
        delete m_pItemPropDef;
        m_pItemPropDef = 0;
    }
    if (m_pPortrait != 0) {
        m_pPortrait->Unload2DArray();
        delete m_pPortrait;
        m_pPortrait = 0;
    }
    if (m_pIPRPDamage != 0) {
        m_pIPRPDamage->Unload2DArray();
        delete m_pIPRPDamage;
        m_pIPRPDamage = 0;
    }
    if (m_pTraps != 0) {
        m_pTraps->Unload2DArray();
        delete m_pTraps;
        m_pTraps = 0;
    }
    if (m_pLightColor != 0) {
        m_pLightColor->Unload2DArray();
        delete m_pLightColor;
        m_pLightColor = 0;
    }
    if (m_pCursor != 0) {
        m_pCursor->Unload2DArray();
        delete m_pCursor;
        m_pCursor = 0;
    }
    if (m_pAmbientMusic != 0) {
        m_pAmbientMusic->Unload2DArray();
        delete m_pAmbientMusic;
        m_pAmbientMusic = 0;
    }
    if (m_pAmbientSound != 0) {
        m_pAmbientSound->Unload2DArray();
        delete m_pAmbientSound;
        m_pAmbientSound = 0;
    }
    if (m_pFootstepSounds != 0) {
        m_pFootstepSounds->Unload2DArray();
        delete m_pFootstepSounds;
        m_pFootstepSounds = 0;
    }
    if (m_pAppearanceSounds != 0) {
        m_pAppearanceSounds->Unload2DArray();
        delete m_pAppearanceSounds;
        m_pAppearanceSounds = 0;
    }
    if (m_pWeaponSounds != 0) {
        m_pWeaponSounds->Unload2DArray();
        delete m_pWeaponSounds;
        m_pWeaponSounds = 0;
    }
    if (m_pAmmunitionTypes != 0) {
        m_pAmmunitionTypes->Unload2DArray();
        delete m_pAmmunitionTypes;
        m_pAmmunitionTypes = 0;
    }
    if (m_pKeymap != 0) {
        m_pKeymap->Unload2DArray();
        delete m_pKeymap;
        m_pKeymap = 0;
    }
    if (m_pBindableKey != 0) {
        m_pBindableKey->Unload2DArray();
        delete m_pBindableKey;
        m_pBindableKey = 0;
    }
    if (m_pPlaceableSounds != 0) {
        m_pPlaceableSounds->Unload2DArray();
        delete m_pPlaceableSounds;
        m_pPlaceableSounds = 0;
    }
    if (m_pCameraStyle != 0) {
        m_pCameraStyle->Unload2DArray();
        delete m_pCameraStyle;
        m_pCameraStyle = 0;
    }
    if (m_pTutorial != 0) {
        m_pTutorial->Unload2DArray();
        delete m_pTutorial;
        m_pTutorial = 0;
    }
    if (m_pDifficultyOptions != 0) {
        m_pDifficultyOptions->Unload2DArray();
        delete m_pDifficultyOptions;
        m_pDifficultyOptions = 0;
    }
    if (m_pGamma != 0) {
        m_pGamma->Unload2DArray();
        delete m_pGamma;
        m_pGamma = 0;
    }
    if (m_pStateScripts != 0) {
        m_pStateScripts->Unload2DArray();
        delete m_pStateScripts;
        m_pStateScripts = 0;
    }
    if (m_pPoison != 0) {
        m_pPoison->Unload2DArray();
        delete m_pPoison;
        m_pPoison = 0;
    }
    if (m_pDisease != 0) {
        m_pDisease->Unload2DArray();
        delete m_pDisease;
        m_pDisease = 0;
    }
    if (m_pRepAdjustments != 0) {
        m_pRepAdjustments->Unload2DArray();
        delete m_pRepAdjustments;
        m_pRepAdjustments = 0;
    }
    if (m_pFractionalCR != 0) {
        m_pFractionalCR->Unload2DArray();
        delete m_pFractionalCR;
        m_pFractionalCR = 0;
    }
    if (m_pEncounterDifficulty != 0) {
        m_pEncounterDifficulty->Unload2DArray();
        delete m_pEncounterDifficulty;
        m_pEncounterDifficulty = 0;
    }
    if (m_pExcitedDuration != 0) {
        m_pExcitedDuration->Unload2DArray();
        delete m_pExcitedDuration;
        m_pExcitedDuration = 0;
    }
    if (m_pRegeneration != 0) {
        m_pRegeneration->Unload2DArray();
        delete m_pRegeneration;
        m_pRegeneration = 0;
    }
    if (m_pXpBase != 0) {
        m_pXpBase->Unload2DArray();
        delete m_pXpBase;
        m_pXpBase = 0;
    }
    if (m_pIPRPMonsterDamage != 0) {
        m_pIPRPMonsterDamage->Unload2DArray();
        delete m_pIPRPMonsterDamage;
        m_pIPRPMonsterDamage = 0;
    }
    if (m_pIPRPBonusCost != 0) {
        m_pIPRPBonusCost->Unload2DArray();
        delete m_pIPRPBonusCost;
        m_pIPRPBonusCost = 0;
    }
    if (m_pIPRPSRCost != 0) {
        m_pIPRPSRCost->Unload2DArray();
        delete m_pIPRPSRCost;
        m_pIPRPSRCost = 0;
    }
    if (m_pIPRPNeg5Cost != 0) {
        m_pIPRPNeg5Cost->Unload2DArray();
        delete m_pIPRPNeg5Cost;
        m_pIPRPNeg5Cost = 0;
    }
    if (m_pRanges != 0) {
        m_pRanges->Unload2DArray();
        delete m_pRanges;
        m_pRanges = 0;
    }
    if (m_pOnHit != 0) {
        m_pOnHit->Unload2DArray();
        delete m_pOnHit;
        m_pOnHit = 0;
    }
    if (m_pOnHitDuration != 0) {
        m_pOnHitDuration->Unload2DArray();
        delete m_pOnHitDuration;
        m_pOnHitDuration = 0;
    }
    if (m_pDamageHitVisual != 0) {
        m_pDamageHitVisual->Unload2DArray();
        delete m_pDamageHitVisual;
        m_pDamageHitVisual = 0;
    }
    if (m_pAnimations != 0) {
        m_pAnimations->Unload2DArray();
        delete m_pAnimations;
        m_pAnimations = 0;
    }
    if (m_pBodyBag != 0) {
        m_pBodyBag->Unload2DArray();
        delete m_pBodyBag;
        m_pBodyBag = 0;
    }
    if (m_pForceAdjust != 0) {
        m_pForceAdjust->Unload2DArray();
        delete m_pForceAdjust;
        m_pForceAdjust = 0;
    }
    if (m_pGameEffects != 0) {
        m_pGameEffects->Unload2DArray();
        delete m_pGameEffects;
        m_pGameEffects = 0;
    }
    if (m_pRepute != 0) {
        m_pRepute->Unload2DArray();
        delete m_pRepute;
        m_pRepute = 0;
    }
    if (m_pWeaponDischarge != 0) {
        m_pWeaponDischarge->Unload2DArray();
        delete m_pWeaponDischarge;
        m_pWeaponDischarge = 0;
    }
    if (m_pDroidDischarge != 0) {
        m_pDroidDischarge->Unload2DArray();
        delete m_pDroidDischarge;
        m_pDroidDischarge = 0;
    }
    if (m_pPlotXP != 0) {
        m_pPlotXP->Unload2DArray();
        delete m_pPlotXP;
        m_pPlotXP = 0;
    }
    if (m_pCombatAnimations != 0) {
        m_pCombatAnimations->Unload2DArray();
        delete m_pCombatAnimations;
        m_pCombatAnimations = 0;
    }
    if (m_pDialogAnimations != 0) {
        m_pDialogAnimations->Unload2DArray();
        delete m_pDialogAnimations;
        m_pDialogAnimations = 0;
    }
    if (m_pNPC != 0) {
        m_pNPC->Unload2DArray();
        delete m_pNPC;
        m_pNPC = 0;
    }
    if (m_pSpells != 0) {
        m_pSpells->Unload2DArray();
        delete m_pSpells;
        m_pSpells = 0;
    }
    if (m_pFeat != 0) {
        m_pFeat->Unload2DArray();
        delete m_pFeat;
        m_pFeat = 0;
    }
    if (m_pFormations != 0) {
        m_pFormations->Unload2DArray();
        delete m_pFormations;
        m_pFormations = 0;
    }
    if (m_pGrenadeSound != 0) {
        m_pGrenadeSound->Unload2DArray();
        delete m_pGrenadeSound;
        m_pGrenadeSound = 0;
    }
    if (m_pPlanetary != 0) {
        m_pPlanetary->Unload2DArray();
        delete m_pPlanetary;
        m_pPlanetary = 0;
    }
    if (m_pFeedBackText != 0) {
        m_pFeedBackText->Unload2DArray();
        delete m_pFeedBackText;
        m_pFeedBackText = 0;
    }
    if (m_pForceShields != 0) {
        m_pForceShields->Unload2DArray();
        delete m_pForceShields;
        m_pForceShields = 0;
    }
    if (m_pHeads != 0) {
        m_pHeads->Unload2DArray();
        delete m_pHeads;
        m_pHeads = 0;
    }
    if (m_pItemValues != 0) {
        m_pItemValues->Unload2DArray();
        delete m_pItemValues;
        m_pItemValues = 0;
    }
    if (m_pVideoEffects != 0) {
        m_pVideoEffects->Unload2DArray();
        delete m_pVideoEffects;
        m_pVideoEffects = 0;
    }
    if (m_pMovies != 0) {
        m_pMovies->Unload2DArray();
        delete m_pMovies;
        m_pMovies = 0;
    }
    if (m_pEffectIcon != 0) {
        m_pEffectIcon->Unload2DArray();
        delete m_pEffectIcon;
        m_pEffectIcon = 0;
    }
    if (m_pRemoveFXOnDeath != 0) {
        m_pRemoveFXOnDeath->Unload2DArray();
        delete m_pRemoveFXOnDeath;
        m_pRemoveFXOnDeath = 0;
    }
    if (m_pTexPacks != 0) {
        m_pTexPacks->Unload2DArray();
        delete m_pTexPacks;
        m_pTexPacks = 0;
    }
    if (m_pSubrace != 0) {
        m_pSubrace->Unload2DArray();
        delete m_pSubrace;
        m_pSubrace = 0;
    }
    if (m_pIPRPPC != 0) {
        m_pIPRPPC->Unload2DArray();
        delete m_pIPRPPC;
        m_pIPRPPC = 0;
    }
    if (m_pInfluence != 0) {
        m_pInfluence->Unload2DArray();
        delete m_pInfluence;
        m_pInfluence = 0;
    }
    if (m_pItemCreate != 0) {
        m_pItemCreate->Unload2DArray();
        delete m_pItemCreate;
        m_pItemCreate = 0;
    }
    if (m_pChemicalCreate != 0) {
        m_pChemicalCreate->Unload2DArray();
        delete m_pChemicalCreate;
        m_pChemicalCreate = 0;
    }
    if (m_pSwoopUpgrade != 0) {
        m_pSwoopUpgrade->Unload2DArray();
        delete m_pSwoopUpgrade;
        m_pSwoopUpgrade = 0;
    }
    if (m_pMusic != 0) {
        m_pMusic->Unload2DArray();
        delete m_pMusic;
        m_pMusic = 0;
    }
    if (m_pAlienVO != 0) {
        m_pAlienVO->Unload2DArray();
        delete m_pAlienVO;
        m_pAlienVO = 0;
    }
    if (m_pEmotion != 0) {
        m_pEmotion->Unload2DArray();
        delete m_pEmotion;
        m_pEmotion = 0;
    }
    if (m_pAutoBalance != 0) {
        m_pAutoBalance->Unload2DArray();
        delete m_pAutoBalance;
        m_pAutoBalance = 0;
    }
    if (m_pAIScripts != 0) {
        m_pAIScripts->Unload2DArray();
        delete m_pAIScripts;
        m_pAIScripts = 0;
    }
#endif

    return true;
}


static inline CResRef MakeResRef(const char* szName) { CResRef r(szName); return r; }

int CTwoDimArrays::Load2DArrays()
{
    LoadIPRPCostTables();
    LoadIPRPParamTables();

    if (!Load2DArrays_Appearance()) return false;
    if (!Load2DArrays_Gender()) return false;
    if (!Load2DArrays_SurfaceMaterial()) return false;
    if (!Load2DArrays_VisualEffect()) return false;
    if (!Load2DArrays_PersistentVisualEffect()) return false;
    if (!Load2DArrays_CreatureSpeed()) return false;
    if (!Load2DArrays_DoorTypes()) return false;
    if (!Load2DArrays_GenericDoors()) return false;
    if (!Load2DArrays_Placeables()) return false;

    m_pAreaTransition = new C2DA(MakeResRef("Loadscreens"), 0);
    if (!m_pAreaTransition->Load2DArray()) return false;
    m_pIPRPMelee = new C2DA(MakeResRef("IPRP_MeleeCost"), 0);
    if (!m_pIPRPMelee->Load2DArray()) return false;
    m_pItemPropDef = new C2DA(MakeResRef("ItemPropDef"), 0);
    if (!m_pItemPropDef->Load2DArray()) return false;
    m_pPortrait = new C2DA(MakeResRef("Portraits"), 0);
    if (!m_pPortrait->Load2DArray()) return false;
    m_pIPRPDamage = new C2DA(MakeResRef("IPRP_DAMAGECOST"), 0);
    if (!m_pIPRPDamage->Load2DArray()) return false;
    m_pTraps = new C2DA(MakeResRef("Traps"), 0);
    if (!m_pTraps->Load2DArray()) return false;
    m_pLightColor = new C2DA(MakeResRef("LightColor"), 0);
    if (!m_pLightColor->Load2DArray()) return false;

    bool bSkipStateScripts = false;
    if (g_nClientServerMode != 2) {
        m_pCursor = new C2DA(MakeResRef("cursors"), 0);
        if (!m_pCursor->Load2DArray()) return false;
        m_pAmbientMusic = new C2DA(MakeResRef("AmbientMusic"), 0);
        if (!m_pAmbientMusic->Load2DArray()) return false;
        m_pAmbientSound = new C2DA(MakeResRef("AmbientSound"), 0);
        if (!m_pAmbientSound->Load2DArray()) return false;
        if (!Load2DArrays_FootstepSounds()) return false;
        if (!Load2DArrays_AppearanceSounds()) return false;
        if (!Load2DArrays_WeaponSounds()) return false;
        m_pAmmunitionTypes = new C2DA(MakeResRef("AmmunitionTypes"), 0);
        if (!m_pAmmunitionTypes->Load2DArray()) return false;
        m_pKeymap = new C2DA(MakeResRef("keymap"), 0);
        if (!m_pKeymap->Load2DArray()) return false;
        m_pBindableKey = new C2DA(MakeResRef("BindableKeys"), 0);
        if (!m_pBindableKey->Load2DArray()) return false;
        if (!Load2DArrays_PlaceableSounds()) return false;
        m_pCameraStyle = new C2DA(MakeResRef("CameraStyle"), 0);
        if (!m_pCameraStyle->Load2DArray()) return false;
        m_pTutorial = new C2DA(MakeResRef("tutorial"), 0);
        if (!m_pTutorial->Load2DArray()) return false;
        m_pDifficultyOptions = new C2DA(MakeResRef("difficultyopt"), 0);
        if (!m_pDifficultyOptions->Load2DArray()) return false;
        m_pGamma = new C2DA(MakeResRef("gamma"), 0);
        if (!m_pGamma->Load2DArray()) return false;
#if !KOTOR1
        m_pAIScripts = new C2DA(MakeResRef("aiscripts"), 0);
        if (!m_pAIScripts->Load2DArray()) return false;
#endif
        bSkipStateScripts = (g_nClientServerMode == 1);
    }

    if (!bSkipStateScripts) {
        m_pStateScripts = new C2DA(MakeResRef("StateScripts"), 0);
        if (!m_pStateScripts->Load2DArray()) return false;
        m_pPoison = new C2DA(MakeResRef("poison"), 0);
        if (!m_pPoison->Load2DArray()) return false;
        m_pDisease = new C2DA(MakeResRef("disease"), 0);
        if (!m_pDisease->Load2DArray()) return false;
        m_pRepAdjustments = new C2DA(MakeResRef("RepAdjust"), 0);
        if (!m_pRepAdjustments->Load2DArray()) return false;
        m_pFractionalCR = new C2DA(MakeResRef("FractionalCR"), 0);
        if (!m_pFractionalCR->Load2DArray()) return false;
        m_pEncounterDifficulty = new C2DA(MakeResRef("EncDifficulty"), 0);
        if (!m_pEncounterDifficulty->Load2DArray()) return false;
        m_pExcitedDuration = new C2DA(MakeResRef("ExcitedDuration"), 0);
        if (!m_pExcitedDuration->Load2DArray()) return false;
#if KOTOR1
        if (!Load2DArrays_Regeneration()) return false;
#else
        m_pRegeneration = new C2DA(MakeResRef("Regeneration"), 0);
        if (!m_pRegeneration->Load2DArray()) return false;
#endif
        m_pXpBase = new C2DA(MakeResRef("XPTable"), 0);
        if (!m_pXpBase->Load2DArray()) return false;
        m_pIPRPMonsterDamage = new C2DA(MakeResRef("IPRP_MONSTCOST"), 0);
        if (!m_pIPRPMonsterDamage->Load2DArray()) return false;
        m_pIPRPBonusCost = new C2DA(MakeResRef("IPRP_BONUSCOST"), 0);
        if (!m_pIPRPBonusCost->Load2DArray()) return false;
        m_pIPRPSRCost = new C2DA(MakeResRef("IPRP_SRCOST"), 0);
        if (!m_pIPRPSRCost->Load2DArray()) return false;
        m_pIPRPNeg5Cost = new C2DA(MakeResRef("IPRP_NEG5COST"), 0);
        if (!m_pIPRPNeg5Cost->Load2DArray()) return false;
        m_pRanges = new C2DA(MakeResRef("Ranges"), 0);
        if (!m_pRanges->Load2DArray()) return false;
        m_pOnHit = new C2DA(MakeResRef("IPRP_ONHIT"), 0);
        if (!m_pOnHit->Load2DArray()) return false;
        m_pOnHitDuration = new C2DA(MakeResRef("IPRP_ONHITDUR"), 0);
        if (!m_pOnHitDuration->Load2DArray()) return false;
        m_pDamageHitVisual = new C2DA(MakeResRef("DamageHitVisual"), 0);
        if (!m_pDamageHitVisual->Load2DArray()) return false;
        if (!Load2DArrays_Animations()) return false;
        m_pBodyBag = new C2DA(MakeResRef("bodybag"), 0);
        if (!m_pBodyBag->Load2DArray()) return false;
        if (!Load2DArrays_ForceAdjust()) return false;
        m_pGameEffects = new C2DA(MakeResRef("GameEffects"), 0);
        if (!m_pGameEffects->Load2DArray()) return false;
#if KOTOR1
        m_pGameEffects = new C2DA(MakeResRef("GameEffects"), 0);
        if (!m_pGameEffects->Load2DArray()) return false;
#endif
        m_pRepute = new C2DA(MakeResRef("Repute"), 0);
        if (!m_pRepute->Load2DArray()) return false;
        m_pWeaponDischarge = new C2DA(MakeResRef("WeaponDischarge"), 0);
        if (!m_pWeaponDischarge->Load2DArray()) return false;
        m_pDroidDischarge = new C2DA(MakeResRef("DroidDischarge"), 0);
        if (!m_pDroidDischarge->Load2DArray()) return false;
        m_pPlotXP = new C2DA(MakeResRef("Plot"), 0);
        if (!m_pPlotXP->Load2DArray()) return false;
        m_pCombatAnimations = new C2DA(MakeResRef("CombatAnimations"), 0);
        if (!m_pCombatAnimations->Load2DArray()) return false;
        if (!Load2DArrays_DialogAnimations()) return false;
        m_pNPC = new C2DA(MakeResRef("NPC"), 0);
        if (!m_pNPC->Load2DArray()) return false;
        m_pSpells = new C2DA(MakeResRef("Spells"), 0);
        if (!m_pSpells->Load2DArray()) return false;
        m_pFeat = new C2DA(MakeResRef("Feat"), 0);
        if (!m_pFeat->Load2DArray()) return false;
        m_pFormations = new C2DA(MakeResRef("Formations"), 0);
        if (!m_pFormations->Load2DArray()) return false;
        m_pGrenadeSound = new C2DA(MakeResRef("GrenadeSND"), 0);
        if (!m_pGrenadeSound->Load2DArray()) return false;
        m_pPlanetary = new C2DA(MakeResRef("Planetary"), 0);
        if (!m_pPlanetary->Load2DArray()) return false;
        m_pFeedBackText = new C2DA(MakeResRef("FeedBackText"), 0);
        if (!m_pFeedBackText->Load2DArray()) return false;
        m_pForceShields = new C2DA(MakeResRef("ForceShields"), 0);
        if (!m_pForceShields->Load2DArray()) return false;
        m_pHeads = new C2DA(MakeResRef("Heads"), 0);
        if (!m_pHeads->Load2DArray()) return false;
        m_pItemValues = new C2DA(MakeResRef("ItemValue"), 0);
        if (!m_pItemValues->Load2DArray()) return false;
        m_pVideoEffects = new C2DA(MakeResRef("VideoEffects"), 0);
        if (!m_pVideoEffects->Load2DArray()) return false;
        m_pMovies = new C2DA(MakeResRef("Movies"), 0);
        if (!m_pMovies->Load2DArray()) return false;
        if (!Load2DArrays_EffectIcon()) return false;
        if (!Load2DArrays_RemoveFXOnDeath()) return false;
        if (!Load2DArrays_TexPacks()) return false;
#if !KOTOR1
        m_pSubrace = new C2DA(MakeResRef("Subrace"), 0);
        if (!m_pSubrace->Load2DArray()) return false;
        m_pIPRPPC = new C2DA(MakeResRef("IPRP_PC"), 0);
        if (!m_pIPRPPC->Load2DArray()) return false;
        m_pInfluence = new C2DA(MakeResRef("Influence"), 0);
        if (!m_pInfluence->Load2DArray()) return false;
        m_pItemCreate = new C2DA(MakeResRef("ItemCreate"), 0);
        if (!m_pItemCreate->Load2DArray()) return false;
        m_pChemicalCreate = new C2DA(MakeResRef("ChemicalCreate"), 0);
        if (!m_pChemicalCreate->Load2DArray()) return false;
        m_pSwoopUpgrade = new C2DA(MakeResRef("SwoopUpgrade"), 0);
        if (!m_pSwoopUpgrade->Load2DArray()) return false;
        m_pMusic = new C2DA(MakeResRef("MusicTable"), 0);
        if (!m_pMusic->Load2DArray()) return false;
        m_pAlienVO = new C2DA(MakeResRef("AlienVO"), 0);
        if (!m_pAlienVO->Load2DArray()) return false;
        m_pEmotion = new C2DA(MakeResRef("Emotion"), 0);
        if (!m_pEmotion->Load2DArray()) return false;
        m_pAutoBalance = new C2DA(MakeResRef("AutoBalance"), 0);
        if (!m_pAutoBalance->Load2DArray()) return false;
#endif
    }

    return true;
}


bool CTwoDimArrays::LoadIPRPCostTables()
{
    C2DA* pMaster = new C2DA(CResRef("IPRP_COSTTABLE"), 0);
    if (!pMaster->Load2DArray())
        return false;

    m_nIPRPCostTableCount = static_cast<unsigned char>(pMaster->m_nRows);
    CExoString sResRef;
    unsigned char count = m_nIPRPCostTableCount;
    m_paIPRPCostTables = new C2DA*[count];
    if (count != 0)
    {
        uint64_t i = 0;
        do
        {
            m_paIPRPCostTables[i] = 0;
            int nClientLoad = 1;
            bool bLoadTable = true;
            if (g_nClientServerMode == 1)
            {
                pMaster->GetINTEntry(static_cast<int>(i), CExoString("ClientLoad"), &nClientLoad);
                bLoadTable = (nClientLoad != 0);
            }
            if (bLoadTable)
            {
                pMaster->GetCExoStringEntry(static_cast<int>(i), CExoString("Name"), &sResRef);
                m_paIPRPCostTables[i] = new C2DA(CResRef(static_cast<char*>(sResRef.CStr())), 0);
                if (!m_paIPRPCostTables[i]->Load2DArray())
                {
                    delete pMaster;
                    return false;
                }
            }
            i++;
        } while (i < m_nIPRPCostTableCount);
    }
    delete pMaster;
    return true;
}

bool CTwoDimArrays::LoadIPRPParamTables()
{
    C2DA* pMaster = new C2DA(CResRef("IPRP_PARAMTABLE"), 0);
    if (!pMaster->Load2DArray())
        return false;

    m_nIPRPParamTableCount = static_cast<unsigned char>(pMaster->m_nRows);
    CExoString sResRef;
    unsigned char count = m_nIPRPParamTableCount;
    m_paIPRPParamTables = new C2DA*[count];
    if (count == 0) goto delete_master_success;

    {
        uint64_t i = 0;
        while (true) {
            m_paIPRPParamTables[i] = 0;
            pMaster->GetCExoStringEntry(static_cast<int>(i), CExoString("TableResRef"), &sResRef);
            C2DA* pTable = new C2DA(CResRef(static_cast<char*>(sResRef.CStr())), 0);
            m_paIPRPParamTables[i] = pTable;
            if (!pTable->Load2DArray())
                break;
            i++;
            if (m_nIPRPParamTableCount <= i)
                goto delete_master_success;
        }
    }
    delete pMaster;
    return false;

delete_master_success:
    delete pMaster;
    return true;
}

bool CTwoDimArrays::UnLoadIPRPCostTables()
{
    C2DA** paTables = m_paIPRPCostTables;
    if (paTables == 0) return true;

    unsigned char count = m_nIPRPCostTableCount;
    if (count != 0) {
        for (uint64_t i = 0; i < m_nIPRPCostTableCount; i++) {
            C2DA* pTable = m_paIPRPCostTables[i];
            if (pTable != 0) {
                pTable->Unload2DArray();
                delete m_paIPRPCostTables[i];
                m_paIPRPCostTables[i] = 0;
                paTables = m_paIPRPCostTables;
            }
        }
        if (paTables == 0) {
            m_paIPRPCostTables = 0;
            return true;
        }
    }
    operator delete[](paTables);
    m_paIPRPCostTables = 0;
    return true;
}

bool CTwoDimArrays::UnLoadIPRPParamTables()
{
    C2DA** paTables = m_paIPRPParamTables;
    if (paTables == 0) return true;

    unsigned char count = m_nIPRPParamTableCount;
    if (count != 0) {
        for (uint64_t i = 0; i < m_nIPRPParamTableCount; i++) {
            C2DA* pTable = m_paIPRPParamTables[i];
            if (pTable != 0) {
                pTable->Unload2DArray();
                delete m_paIPRPParamTables[i];
                m_paIPRPParamTables[i] = 0;
                paTables = m_paIPRPParamTables;
            }
        }
        if (paTables == 0) {
            m_paIPRPParamTables = 0;
            return true;
        }
    }
    operator delete[](paTables);
    m_paIPRPParamTables = 0;
    return true;
}

C2DA* CTwoDimArrays::GetIPRPCostTable(unsigned char idx)
{
    if (idx >= m_nIPRPCostTableCount || !m_paIPRPCostTables)
        return 0;
    return m_paIPRPCostTables[idx];
}

C2DA* CTwoDimArrays::GetIPRPParamTable(unsigned char idx)
{
    if (idx >= m_nIPRPParamTableCount || !m_paIPRPParamTables)
        return 0;
    return m_paIPRPParamTables[idx];
}

int CTwoDimArrays::Load2DArrays_Appearance()
{
    m_pAppearance = new C2DA(CResRef("Appearance"), 0);
    if (m_pAppearance->Load2DArray()) {
        COL2DA_APPEARANCETABLE_ABORTONPARRY = m_pAppearance->GetColumnIndex(CExoString("ABORTONPARRY"));
        COL2DA_APPEARANCETABLE_BACKUPHEAD = m_pAppearance->GetColumnIndex(CExoString("BACKUPHEAD"));
        COL2DA_APPEARANCETABLE_CAMERAHEIGHTOFFSET = m_pAppearance->GetColumnIndex(CExoString("CameraHeightOffset"));
        COL2DA_APPEARANCETABLE_CAMERASPACE = m_pAppearance->GetColumnIndex(CExoString("CAMERASPACE"));
        COL2DA_APPEARANCETABLE_CREPERSPACE = m_pAppearance->GetColumnIndex(CExoString("CREPERSPACE"));
        COL2DA_APPEARANCETABLE_DISABLEINJUREDANIM = m_pAppearance->GetColumnIndex(CExoString("DisableInjuredAnim"));
        COL2DA_APPEARANCETABLE_DRIVEACCl = m_pAppearance->GetColumnIndex(CExoString("DRIVEACCl"));
#if KOTOR1
        COL2DA_APPEARANCETABLE_DRIVEANIMRUN = m_pAppearance->GetColumnIndex(CExoString("DriveAnimRun"));
#else
        COL2DA_APPEARANCETABLE_DRIVEANIMRUN = m_pAppearance->GetColumnIndex(CExoString("DriveAnimRun_PC"));
#endif
        COL2DA_APPEARANCETABLE_DRIVEANIMWALK = m_pAppearance->GetColumnIndex(CExoString("DriveAnimWalk"));
        COL2DA_APPEARANCETABLE_DRIVEMAXSPEED = m_pAppearance->GetColumnIndex(CExoString("DriveMaxSpeed"));
        COL2DA_APPEARANCETABLE_ENVMAP = m_pAppearance->GetColumnIndex(CExoString("ENVMAP"));
        COL2DA_APPEARANCETABLE_FOOTSTEPTYPE = m_pAppearance->GetColumnIndex(CExoString("FootstepType"));
        COL2DA_APPEARANCETABLE_HEAD_ARC_H = m_pAppearance->GetColumnIndex(CExoString("HEAD_ARC_H"));
        COL2DA_APPEARANCETABLE_HEAD_ARC_V = m_pAppearance->GetColumnIndex(CExoString("HEAD_ARC_V"));
        COL2DA_APPEARANCETABLE_HEADBONE = m_pAppearance->GetColumnIndex(CExoString("HEADBONE"));
        COL2DA_APPEARANCETABLE_HEADTRACK = m_pAppearance->GetColumnIndex(CExoString("HEADTRACK"));
        COL2DA_APPEARANCETABLE_HEIGHT = m_pAppearance->GetColumnIndex(CExoString("HEIGHT"));
        COL2DA_APPEARANCETABLE_HITRADIUS = m_pAppearance->GetColumnIndex(CExoString("HITRADIUS"));
        COL2DA_APPEARANCETABLE_HITDIST = m_pAppearance->GetColumnIndex(CExoString("hitdist"));
        COL2DA_APPEARANCETABLE_GROUNDTILT = m_pAppearance->GetColumnIndex(CExoString("GROUNDTILT"));
        COL2DA_APPEARANCETABLE_MODELTYPE = m_pAppearance->GetColumnIndex(CExoString("MODELTYPE"));
        COL2DA_APPEARANCETABLE_NORMALHEAD = m_pAppearance->GetColumnIndex(CExoString("NORMALHEAD"));
        COL2DA_APPEARANCETABLE_PERSPACE = m_pAppearance->GetColumnIndex(CExoString("PERSPACE"));
        COL2DA_APPEARANCETABLE_RACE = m_pAppearance->GetColumnIndex(CExoString("RACE"));
        COL2DA_APPEARANCETABLE_RUNDIST = m_pAppearance->GetColumnIndex(CExoString("RUNDIST"));
        COL2DA_APPEARANCETABLE_SIZECATEGORY = m_pAppearance->GetColumnIndex(CExoString("SIZECATEGORY"));
        COL2DA_APPEARANCETABLE_SOUNDAPPTYPE = m_pAppearance->GetColumnIndex(CExoString("SoundAppType"));
        COL2DA_APPEARANCETABLE_WALKDIST = m_pAppearance->GetColumnIndex(CExoString("WALKDIST"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_Gender()
{
    m_pGender = new C2DA(CResRef("GENDER"), 0);
    if (m_pGender->Load2DArray()) {
        COL2DA_GENDER_GENDER = m_pGender->GetColumnIndex(CExoString("GENDER"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_SurfaceMaterial()
{
    m_pSurfaceMaterial = new C2DA(CResRef("SURFACEMAT"), 0);
    if (m_pSurfaceMaterial->Load2DArray()) {
        COL2DA_SURFACEMATERIAL_NAME = m_pSurfaceMaterial->GetColumnIndex(CExoString("Name"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_VisualEffect()
{
    m_pVisualEffect = new C2DA(CResRef("visualeffects"), 0);
    if (m_pVisualEffect->Load2DArray()) {
        COL2DA_VISUALEFFECT_IMP_HEADCON_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_HeadCon_Node"));
        COL2DA_VISUALEFFECT_IMP_IMPACT_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_Impact_Node"));
        COL2DA_VISUALEFFECT_IMP_ROOT_H_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_Root_H_Node"));
        COL2DA_VISUALEFFECT_IMP_ROOT_L_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_Root_L_Node"));
        COL2DA_VISUALEFFECT_IMP_ROOT_M_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_Root_M_Node"));
        COL2DA_VISUALEFFECT_IMP_ROOT_S_NODE = m_pVisualEffect->GetColumnIndex(CExoString("Imp_Root_S_Node"));
        COL2DA_VISUALEFFECT_LOWQUALITY = m_pVisualEffect->GetColumnIndex(CExoString("LowQuality"));
        COL2DA_VISUALEFFECT_ORIENTWITHGROUND = m_pVisualEffect->GetColumnIndex(CExoString("OrientWithGround"));
        COL2DA_VISUALEFFECT_SHAKEDELAY = m_pVisualEffect->GetColumnIndex(CExoString("ShakeDelay"));
        COL2DA_VISUALEFFECT_SHAKEDURATION = m_pVisualEffect->GetColumnIndex(CExoString("ShakeDuration"));
        COL2DA_VISUALEFFECT_SHAKETYPE = m_pVisualEffect->GetColumnIndex(CExoString("ShakeType"));
        COL2DA_VISUALEFFECT_SOUNDDURATION = m_pVisualEffect->GetColumnIndex(CExoString("SoundDuration"));
        COL2DA_VISUALEFFECT_SOUNDIMPACT = m_pVisualEffect->GetColumnIndex(CExoString("SoundImpact"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_PersistentVisualEffect()
{
    m_pPersistentVisualEffect = new C2DA(CResRef("VFX_Persistent"), 0);
    if (m_pPersistentVisualEffect->Load2DArray()) {
        COL2DA_PERSISTENTVISUALEFFECT_DURATIONVFX = m_pPersistentVisualEffect->GetColumnIndex(CExoString("DurationVFX"));
        COL2DA_PERSISTENTVISUALEFFECT_ORIENTWITHGROUND = m_pPersistentVisualEffect->GetColumnIndex(CExoString("OrientWithGround"));
        COL2DA_PERSISTENTVISUALEFFECT_SHAPE = m_pPersistentVisualEffect->GetColumnIndex(CExoString("SHAPE"));
        COL2DA_PERSISTENTVISUALEFFECT_RADIUS = m_pPersistentVisualEffect->GetColumnIndex(CExoString("RADIUS"));
        COL2DA_PERSISTENTVISUALEFFECT_WIDTH = m_pPersistentVisualEffect->GetColumnIndex(CExoString("WIDTH"));
        COL2DA_PERSISTENTVISUALEFFECT_LENGTH = m_pPersistentVisualEffect->GetColumnIndex(CExoString("LENGTH"));
        COL2DA_PERSISTENTVISUALEFFECT_MODELMIN01 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODELMIN01"));
        COL2DA_PERSISTENTVISUALEFFECT_MODEL01 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODEL01"));
        COL2DA_PERSISTENTVISUALEFFECT_MODELMIN02 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODELMIN02"));
        COL2DA_PERSISTENTVISUALEFFECT_MODEL02 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODEL02"));
        COL2DA_PERSISTENTVISUALEFFECT_MODELMIN03 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODELMIN03"));
        COL2DA_PERSISTENTVISUALEFFECT_MODEL03 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("MODEL03"));
        COL2DA_PERSISTENTVISUALEFFECT_NUMACT01 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("NUMACT01"));
        COL2DA_PERSISTENTVISUALEFFECT_NUMACT02 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("NUMACT02"));
        COL2DA_PERSISTENTVISUALEFFECT_NUMACT03 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("NUMACT03"));
        COL2DA_PERSISTENTVISUALEFFECT_DURATION01 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("DURATION01"));
        COL2DA_PERSISTENTVISUALEFFECT_DURATION02 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("DURATION02"));
        COL2DA_PERSISTENTVISUALEFFECT_DURATION03 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("DURATION03"));
        COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT01 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("EDGEWGHT01"));
        COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT02 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("EDGEWGHT02"));
        COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT03 = m_pPersistentVisualEffect->GetColumnIndex(CExoString("EDGEWGHT03"));
        COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOT = m_pPersistentVisualEffect->GetColumnIndex(CExoString("SoundOneShot"));
        COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOTPERCENTAGE = m_pPersistentVisualEffect->GetColumnIndex(CExoString("SoundOneShotPercentage"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_CreatureSpeed()
{
    m_pCreatureSpeed = new C2DA(CResRef("CreatureSpeed"), 0);
    if (m_pCreatureSpeed->Load2DArray()) {
        COL2DA_CREATURESPEED_RUNRATE = m_pCreatureSpeed->GetColumnIndex(CExoString("RUNRATE"));
        COL2DA_CREATURESPEED_WALKRATE = m_pCreatureSpeed->GetColumnIndex(CExoString("WALKRATE"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_DoorTypes()
{
    m_pDoorTypes = new C2DA(CResRef("DoorTypes"), 0);
    if (m_pDoorTypes->Load2DArray()) {
        COL2DA_DOORTYPES_SOUNDAPPTYPE = m_pDoorTypes->GetColumnIndex(CExoString("SoundAppType"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_GenericDoors()
{
    m_pGenericDoors = new C2DA(CResRef("GenericDoors"), 0);
    if (m_pGenericDoors->Load2DArray()) {
        COL2DA_GENERICDOORS_SOUNDAPPTYPE = m_pGenericDoors->GetColumnIndex(CExoString("SoundAppType"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_FootstepSounds()
{
    m_pFootstepSounds = new C2DA(CResRef("FootstepSounds"), 0);
    if (m_pFootstepSounds->Load2DArray()) {
        COL2DA_FOOTSTEPSOUNDS_PITCHOFFSET = m_pFootstepSounds->GetColumnIndex(CExoString("PitchOffset"));
        COL2DA_FOOTSTEPSOUNDS_ROLLING = m_pFootstepSounds->GetColumnIndex(CExoString("Rolling"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_Placeables()
{
    m_pPlaceables = new C2DA(CResRef("Placeables"), 0);
    if (m_pPlaceables->Load2DArray()) {
        COL2DA_PLACEABLES_SOUNDAPPTYPE = m_pPlaceables->GetColumnIndex(CExoString("SoundAppType"));
        COL2DA_PLACEABLES_STRREF = m_pPlaceables->GetColumnIndex(CExoString("StrRef"));
        COL2DA_PLACEABLES_IGNORESTATICHITCHECK = m_pPlaceables->GetColumnIndex(CExoString("IgnoreStaticHitcheck"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_AppearanceSounds()
{
    m_pAppearanceSounds = new C2DA(CResRef("AppearanceSndSet"), 0);
    if (m_pAppearanceSounds->Load2DArray()) {
        COL2DA_APPEARANCESOUNDS_ARMORTYPE = m_pAppearanceSounds->GetColumnIndex(CExoString("ArmorType"));
        COL2DA_APPEARANCESOUNDS_FALLDIRT = m_pAppearanceSounds->GetColumnIndex(CExoString("FallDirt"));
        COL2DA_APPEARANCESOUNDS_FALLHARD = m_pAppearanceSounds->GetColumnIndex(CExoString("FallHard"));
        COL2DA_APPEARANCESOUNDS_FALLMETAL = m_pAppearanceSounds->GetColumnIndex(CExoString("FallMetal"));
        COL2DA_APPEARANCESOUNDS_FALLWATER = m_pAppearanceSounds->GetColumnIndex(CExoString("FallWater"));
        COL2DA_APPEARANCESOUNDS_WEAPON = m_pAppearanceSounds->GetColumnIndex(CExoString("Weapon"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_WeaponSounds()
{
    m_pWeaponSounds = new C2DA(CResRef("WeaponSounds"), 0);
    if (m_pWeaponSounds->Load2DArray()) {
        COL2DA_WEAPONSOUNDS_PITCHOFFSET = m_pWeaponSounds->GetColumnIndex(CExoString("PitchOffset"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_AreaTransition()
{
    C2DA* p = new C2DA(CResRef("Loadscreens"), 0);
    m_pAreaTransition = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPMelee()
{
    C2DA* p = new C2DA(CResRef("IPRP_MeleeCost"), 0);
    m_pIPRPMelee = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ItemPropDef()
{
    C2DA* p = new C2DA(CResRef("ItemPropDef"), 0);
    m_pItemPropDef = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Portrait()
{
    C2DA* p = new C2DA(CResRef("Portraits"), 0);
    m_pPortrait = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPDamage()
{
    C2DA* p = new C2DA(CResRef("IPRP_DAMAGECOST"), 0);
    m_pIPRPDamage = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Traps()
{
    C2DA* p = new C2DA(CResRef("Traps"), 0);
    m_pTraps = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_LightColor()
{
    C2DA* p = new C2DA(CResRef("LightColor"), 0);
    m_pLightColor = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Cursor()
{
    C2DA* p = new C2DA(CResRef("cursors"), 0);
    m_pCursor = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AmbientMusic()
{
    C2DA* p = new C2DA(CResRef("AmbientMusic"), 0);
    m_pAmbientMusic = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AmbientSound()
{
    C2DA* p = new C2DA(CResRef("AmbientSound"), 0);
    m_pAmbientSound = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AmmunitionTypes()
{
    C2DA* p = new C2DA(CResRef("AmmunitionTypes"), 0);
    m_pAmmunitionTypes = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Keymap()
{
    C2DA* p = new C2DA(CResRef("keymap"), 0);
    m_pKeymap = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_BindableKey()
{
    C2DA* p = new C2DA(CResRef("BindableKeys"), 0);
    m_pBindableKey = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_PlaceableSounds()
{
    m_pPlaceableSounds = new C2DA(CResRef("placeableobjsnds"), 0);
    if (m_pPlaceableSounds->Load2DArray()) {
        COL2DA_PLACEABLESOUNDS_ARMORTYPE = m_pPlaceableSounds->GetColumnIndex(CExoString("ArmorType"));
        COL2DA_PLACEABLESOUNDS_LOCKED = m_pPlaceableSounds->GetColumnIndex(CExoString("Locked"));
        COL2DA_PLACEABLESOUNDS_OPENED = m_pPlaceableSounds->GetColumnIndex(CExoString("Opened"));
        COL2DA_PLACEABLESOUNDS_CLOSED = m_pPlaceableSounds->GetColumnIndex(CExoString("Closed"));
        COL2DA_PLACEABLESOUNDS_DESTROYED = m_pPlaceableSounds->GetColumnIndex(CExoString("Destroyed"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_CameraStyle()
{
    C2DA* p = new C2DA(CResRef("CameraStyle"), 0);
    m_pCameraStyle = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Tutorial()
{
    C2DA* p = new C2DA(CResRef("tutorial"), 0);
    m_pTutorial = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_DifficultyOptions()
{
    C2DA* p = new C2DA(CResRef("difficultyopt"), 0);
    m_pDifficultyOptions = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Gamma()
{
    C2DA* p = new C2DA(CResRef("gamma"), 0);
    m_pGamma = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_StateScripts()
{
    C2DA* p = new C2DA(CResRef("StateScripts"), 0);
    m_pStateScripts = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Poison()
{
    C2DA* p = new C2DA(CResRef("poison"), 0);
    m_pPoison = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Disease()
{
    C2DA* p = new C2DA(CResRef("disease"), 0);
    m_pDisease = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_RepAdjustments()
{
    C2DA* p = new C2DA(CResRef("RepAdjust"), 0);
    m_pRepAdjustments = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_FractionalCR()
{
    C2DA* p = new C2DA(CResRef("FractionalCR"), 0);
    m_pFractionalCR = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_EncounterDifficulty()
{
    C2DA* p = new C2DA(CResRef("EncDifficulty"), 0);
    m_pEncounterDifficulty = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ExcitedDuration()
{
    C2DA* p = new C2DA(CResRef("ExcitedDuration"), 0);
    m_pExcitedDuration = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Regeneration()
{
#if KOTOR1
    m_pRegeneration = new C2DA(CResRef("Regeneration"), 0);
    if (m_pRegeneration->Load2DArray()) {
        COL2DA_REGENERATION_FORCEREGEN = m_pRegeneration->GetColumnIndex(CExoString("ForceRegen"));
        COL2DA_REGENERATION_HEALTHREGEN = m_pRegeneration->GetColumnIndex(CExoString("HealthRegen"));
        return true;
    }
    return false;
#else
    C2DA* p = new C2DA(CResRef("Regeneration"), 0);
    m_pRegeneration = p;
    return p->Load2DArray() != 0;
#endif
}

int CTwoDimArrays::Load2DArrays_XpBase()
{
    C2DA* p = new C2DA(CResRef("XPTable"), 0);
    m_pXpBase = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPMonsterDamage()
{
    C2DA* p = new C2DA(CResRef("IPRP_MONSTCOST"), 0);
    m_pIPRPMonsterDamage = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPBonusCost()
{
    C2DA* p = new C2DA(CResRef("IPRP_BONUSCOST"), 0);
    m_pIPRPBonusCost = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPSRCost()
{
    C2DA* p = new C2DA(CResRef("IPRP_SRCOST"), 0);
    m_pIPRPSRCost = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPNeg5Cost()
{
    C2DA* p = new C2DA(CResRef("IPRP_NEG5COST"), 0);
    m_pIPRPNeg5Cost = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Ranges()
{
    C2DA* p = new C2DA(CResRef("Ranges"), 0);
    m_pRanges = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_OnHit()
{
    C2DA* p = new C2DA(CResRef("IPRP_ONHIT"), 0);
    m_pOnHit = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_OnHitDuration()
{
    C2DA* p = new C2DA(CResRef("IPRP_ONHITDUR"), 0);
    m_pOnHitDuration = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_DamageHitVisual()
{
    C2DA* p = new C2DA(CResRef("DamageHitVisual"), 0);
    m_pDamageHitVisual = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_BodyBag()
{
    C2DA* p = new C2DA(CResRef("bodybag"), 0);
    m_pBodyBag = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Animations()
{
    m_pAnimations = new C2DA(CResRef("Animations"), 0);
    if (m_pAnimations->Load2DArray()) {
        COL2DA_ANIMATIONS_ATTACK = m_pAnimations->GetColumnIndex(CExoString("Attack"));
        COL2DA_ANIMATIONS_DAMAGE = m_pAnimations->GetColumnIndex(CExoString("Damage"));
        COL2DA_ANIMATIONS_DIALOG = m_pAnimations->GetColumnIndex(CExoString("Dialog"));
        COL2DA_ANIMATIONS_DODGE = m_pAnimations->GetColumnIndex(CExoString("Dodge"));
        COL2DA_ANIMATIONS_FIREFORGET = m_pAnimations->GetColumnIndex(CExoString("FireForget"));
        COL2DA_ANIMATIONS_HIDEEQUIPPEDITEMS = m_pAnimations->GetColumnIndex(CExoString("HideEquippedItems"));
        COL2DA_ANIMATIONS_LOOPING = m_pAnimations->GetColumnIndex(CExoString("Looping"));
        COL2DA_ANIMATIONS_NAME = m_pAnimations->GetColumnIndex(CExoString("Name"));
        COL2DA_ANIMATIONS_OVERLAY = m_pAnimations->GetColumnIndex(CExoString("Overlay"));
        COL2DA_ANIMATIONS_PARRY = m_pAnimations->GetColumnIndex(CExoString("Parry"));
        COL2DA_ANIMATIONS_PAUSE = m_pAnimations->GetColumnIndex(CExoString("Pause"));
        COL2DA_ANIMATIONS_RUNNING = m_pAnimations->GetColumnIndex(CExoString("Running"));
        COL2DA_ANIMATIONS_STATIONARY = m_pAnimations->GetColumnIndex(CExoString("Stationary"));
        COL2DA_ANIMATIONS_WALKING = m_pAnimations->GetColumnIndex(CExoString("Walking"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_ForceAdjust()
{
    m_pForceAdjust = new C2DA(CResRef("ForceAdjust"), 0);
    if (m_pForceAdjust->Load2DArray()) {
        COL2DA_FORCEADJUST_EVILCOST = m_pForceAdjust->GetColumnIndex(CExoString("EvilCost"));
        COL2DA_FORCEADJUST_GOODCOST = m_pForceAdjust->GetColumnIndex(CExoString("GoodCost"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_GameEffects()
{
    C2DA* p = new C2DA(CResRef("GameEffects"), 0);
    m_pGameEffects = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Repute()
{
    C2DA* p = new C2DA(CResRef("Repute"), 0);
    m_pRepute = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_WeaponDischarge()
{
    C2DA* p = new C2DA(CResRef("WeaponDischarge"), 0);
    m_pWeaponDischarge = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_DroidDischarge()
{
    C2DA* p = new C2DA(CResRef("DroidDischarge"), 0);
    m_pDroidDischarge = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_PlotXP()
{
    C2DA* p = new C2DA(CResRef("Plot"), 0);
    m_pPlotXP = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_CombatAnimations()
{
    C2DA* p = new C2DA(CResRef("CombatAnimations"), 0);
    m_pCombatAnimations = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_DialogAnimations()
{
    m_pDialogAnimations = new C2DA(CResRef("DialogAnimations"), 0);
    if (m_pDialogAnimations->Load2DArray()) {
        COL2DA_DIALOGANIMATIONS_DIALOG = m_pDialogAnimations->GetColumnIndex(CExoString("Dialog"));
        COL2DA_DIALOGANIMATIONS_FIREFORGET = m_pDialogAnimations->GetColumnIndex(CExoString("FireForget"));
        COL2DA_DIALOGANIMATIONS_LOOPING = m_pDialogAnimations->GetColumnIndex(CExoString("Looping"));
        COL2DA_DIALOGANIMATIONS_OVERLAY = m_pDialogAnimations->GetColumnIndex(CExoString("Overlay"));
        return true;
    }
    return false;
}

int CTwoDimArrays::Load2DArrays_NPC()
{
    C2DA* p = new C2DA(CResRef("NPC"), 0);
    m_pNPC = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Spells()
{
    C2DA* p = new C2DA(CResRef("Spells"), 0);
    m_pSpells = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Feat()
{
    C2DA* p = new C2DA(CResRef("Feat"), 0);
    m_pFeat = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Formations()
{
    C2DA* p = new C2DA(CResRef("Formations"), 0);
    m_pFormations = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_GrenadeSound()
{
    C2DA* p = new C2DA(CResRef("GrenadeSND"), 0);
    m_pGrenadeSound = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Planetary()
{
    C2DA* p = new C2DA(CResRef("Planetary"), 0);
    m_pPlanetary = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_FeedBackText()
{
    C2DA* p = new C2DA(CResRef("FeedBackText"), 0);
    m_pFeedBackText = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ForceShields()
{
    C2DA* p = new C2DA(CResRef("ForceShields"), 0);
    m_pForceShields = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Heads()
{
    C2DA* p = new C2DA(CResRef("Heads"), 0);
    m_pHeads = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ItemValues()
{
    C2DA* p = new C2DA(CResRef("ItemValue"), 0);
    m_pItemValues = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_VideoEffects()
{
    C2DA* p = new C2DA(CResRef("VideoEffects"), 0);
    m_pVideoEffects = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Movies()
{
    C2DA* p = new C2DA(CResRef("Movies"), 0);
    m_pMovies = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_EffectIcon()
{
#if KOTOR1
    m_pEffectIcon = new C2DA(CResRef("EffectIcon"), 0);
    if (!m_pEffectIcon->Load2DArray()) {
        delete m_pEffectIcon;
        m_pEffectIcon = 0;
        return false;
    }
    return true;
#else
    C2DA* p = new C2DA(CResRef("EffectIcon"), 0);
    m_pEffectIcon = p;
    if (p->Load2DArray() != 0)
        return true;
    if (m_pEffectIcon) {
        delete m_pEffectIcon;
    }
    m_pEffectIcon = 0;
    return false;
#endif
}

int CTwoDimArrays::Load2DArrays_RemoveFXOnDeath()
{
    C2DA* p = new C2DA(CResRef("removefxondeath"), 0);
    m_pRemoveFXOnDeath = p;
    if (p->Load2DArray() != 0)
        return true;
    if (m_pRemoveFXOnDeath) {
        delete m_pRemoveFXOnDeath;
    }
    m_pRemoveFXOnDeath = 0;
    return false;
}

int CTwoDimArrays::Load2DArrays_TexPacks()
{
    C2DA* p = new C2DA(CResRef("TexPacks"), 0);
    m_pTexPacks = p;
    if (p->Load2DArray() != 0)
        return true;
    if (m_pTexPacks) {
        delete m_pTexPacks;
    }
    m_pTexPacks = 0;
    return false;
}

#if !KOTOR1
int CTwoDimArrays::Load2DArrays_Subrace()
{
    C2DA* p = new C2DA(CResRef("Subrace"), 0);
    m_pSubrace = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_IPRPPC()
{
    C2DA* p = new C2DA(CResRef("IPRP_PC"), 0);
    m_pIPRPPC = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Influence()
{
    C2DA* p = new C2DA(CResRef("Influence"), 0);
    m_pInfluence = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ItemCreate()
{
    C2DA* p = new C2DA(CResRef("ItemCreate"), 0);
    m_pItemCreate = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_ChemicalCreate()
{
    C2DA* p = new C2DA(CResRef("ChemicalCreate"), 0);
    m_pChemicalCreate = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_SwoopUpgrade()
{
    C2DA* p = new C2DA(CResRef("SwoopUpgrade"), 0);
    m_pSwoopUpgrade = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Music()
{
    C2DA* p = new C2DA(CResRef("MusicTable"), 0);
    m_pMusic = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AlienVO()
{
    C2DA* p = new C2DA(CResRef("AlienVO"), 0);
    m_pAlienVO = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_Emotion()
{
    C2DA* p = new C2DA(CResRef("Emotion"), 0);
    m_pEmotion = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AutoBalance()
{
    C2DA* p = new C2DA(CResRef("AutoBalance"), 0);
    m_pAutoBalance = p;
    return p->Load2DArray() != 0;
}

int CTwoDimArrays::Load2DArrays_AIScripts()
{
    C2DA* p = new C2DA(CResRef("aiscripts"), 0);
    m_pAIScripts = p;
    return p->Load2DArray() != 0;
}
#endif
