#pragma once

#include "CExoArrayListDtors.h"
#include "exolocstring.h"
#include "exostring.h"
#include "nwsobject.h"
#include "exoresman.h"
#include "nwscreature.h"

// Server-side snapshot of the last state sent to one player's client for one game object. The message
// layer's update-required family diffs the live object against this snapshot to build a change bitmask,
// and the store and update calls write the new values back in.
class CLastUpdateObject {
public:
    CSWSCreatureAppearanceInfo  m_appearance;
    int                m_bTouchedThisFrame;
    unsigned char      m_objectType;
    char               _pad3d[0x3e - 0x3d];
    unsigned short     m_nSoundSetFile;
    unsigned int       m_objectId;

    Vector             m_position;
    unsigned long long m_oidArea;
    Vector             m_orientation;
    int                m_nAnimation;
    float              m_fAnimationSpeed;
    // The x and y of the last path point sent, plus a z.
    struct LastPathPoint {
        float          x;
        float          y;
        LastPathPoint(float fX, float fY) : x(fX), y(fY) {}
    } m_lastPathPoint;
    float              m_fLastPathPointZ;
    unsigned char      m_nClientPathId;
    unsigned short     m_nAIState;
    unsigned char      m_nAIStateAction;
    char               _pad7d[0x7e - 0x7d];
    unsigned short     m_nActivityFlags;
    int                m_bCombatState;
    unsigned char      m_nStealthMode;
    int                m_oidAIStateTarget;
    unsigned long long m_oidLockOrientationTo;
    unsigned long long m_oidAttackTarget;
    CResRef            m_portrait;
    unsigned short     m_nPortraitId;

    CExoLocString      m_firstName;
    CExoLocString      m_lastName;
    float              m_fWalkRate;
    float              m_fRunRate;

    CExoArrayList<CLoopingVisualEffect*> m_visualEffects;
    unsigned char      m_nAssociateState;
    unsigned char      m_nDamageLevel;
    char               _padf2[0xf6-0xf2];
    unsigned short     m_nCurrentHitPoints;
    unsigned short     m_nHitPoints;
    unsigned short     m_nHitPointBonus;
    unsigned short     m_nMaxHitPoints;
    int                m_bAIStateControllable;
    int                m_bFactionLeader;
    int                m_bSingletonParty;
    int                m_bInvitedToParty;

    CExoArrayList<unsigned long long> m_lstObjectsOnObject;
    int                m_bTrapFlag;
    int                m_bMineFlagged;
    int                m_nFaction;
    int                m_bLocked;
    int                m_bPlot;
    unsigned char      m_nReputation;
    int                m_bSeen;
    int                m_bHeard;
    int                m_bInvisible;
    unsigned short     m_nRace;

    CExoString         m_sSubrace;
    unsigned char      m_nSubraceIndex;
    unsigned char      m_nGender;
    unsigned short     m_nClasses;
    unsigned short     m_nClassLevels;
    unsigned char      m_anReserved2[2];
    unsigned short     m_nAlignment;
#if !KOTOR1  // K1 has the alignment word only here: no base NPC alignment, stance or tag
    unsigned short     m_nBaseNPCAlignment;
    int                m_nStanceId;
    CExoString         m_sTag;
#endif
    int                m_bDeadSelectable;
    int                m_bPlayerCharacter;

    unsigned char      m_nAbilityBase[6];
    unsigned char      m_nAbilityStat[6];
    unsigned short     m_nForcePoints;
    unsigned short     m_nBaseForcePoints;
    unsigned short     m_nTemporaryForcePoints;
    unsigned short     m_nMaxForcePoints;
    unsigned char      m_nSkillRank[8];
    unsigned char      m_nSkillRankBase[8];

    int                m_bDeleteNoFade;
    int                m_bKeepCorpse;
    unsigned int       m_nFadeOutTime;

    struct {
        unsigned long long m_objectId;
        unsigned char      m_nReserved1;
        char               _pad9[0xc-0x9];
        int                m_nReserved2;
        unsigned short     m_nReserved3;
        char               _pad12[0x18-0x12];
    } m_aReserved1[36];

    CLastUpdateObject();
    ~CLastUpdateObject();
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CLastUpdateObject) == 0x500, "K1 CLastUpdateObject");
#endif

#include "CExoLinkedList.h"

class CSWSItem;

// One entry of an inventory snapshot item list, recording what the client last believed about one
// item sitting in one of the three lists below.
class CSWSPlayerLUOInventoryItem {
public:
    unsigned long long m_objectId;
    unsigned int m_nStackCount;
    unsigned int m_nCost;
};

// idiom-exempt: canonical
// Server's per-player snapshot of everything inventory-shaped the client believes: the twenty
// equipment slots plus three item lists, the creature's own repository, whatever container is open
// and whatever store is open. The message layer diffs the live state against it to decide what to
// put on the wire.
class CSWSPlayerLUOInventory {
public:
#if KOTOR1  // K1 has 18 equipment slots
    unsigned long long m_slotItemId[18];
#else
    unsigned long long m_slotItemId[20];
#endif

#if KOTOR1
    int m_slotItemStackCount[18];
#else
    int m_slotItemStackCount[20];
#endif

    CExoLinkedList<CSWSPlayerLUOInventoryItem> m_repository;

    unsigned long long m_openContainerId;

    CExoLinkedList<CSWSPlayerLUOInventoryItem> m_container;
    CExoLinkedList<CSWSPlayerLUOInventoryItem> m_store;

    CSWSPlayerLUOInventory();
    ~CSWSPlayerLUOInventory();

    void ClearSlots();
    void ClearRepository();
    void ClearContainer();
    void ClearStore();

    // nList selects the list: 0 the open container, 1 the creature's repository, 2 the open store.
    // Anything else does nothing.
    unsigned long long ItemListGetItemObjectID(CExoLinkedListNode* pNode,
                                               unsigned char nList);
    CSWSPlayerLUOInventoryItem* ItemListGetUpdateItem(CExoLinkedListNode* pNode,
                                                      unsigned char nList);
    CSWSItem* ItemListGetItem(CExoLinkedListNode* pNode, unsigned char nList);
    void ItemListSetStart(CExoLinkedListNode*& pNode, unsigned char nList);
    void ItemListSetEnd(CExoLinkedListNode*& pNode, unsigned char nList);
    void ItemListGetNext(CExoLinkedListNode*& pNode, unsigned char nList);
    void ItemListGetPrev(CExoLinkedListNode*& pNode, unsigned char nList);
    int  ItemListGetNumber(unsigned char nList);
    void ItemListRemove(CExoLinkedListNode* pNode, unsigned char nList);
    void ItemListAddHead(unsigned long long objectId, unsigned char nList,
                         unsigned int nStackCount, unsigned int nCost);
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSPlayerLUOInventory) == 0xf8, "K1 CSWSPlayerLUOInventory");
#endif

#include "CSWVisibilityNode.h"
#include "nwscreaturestats.h"

class CSWSStats_SpellLikeAbility;

// The server-side snapshot of everything last sent to one player's client, diffed to decide what
// changed and needs re-sending: known spells, feats, visibility list, action queue, spell uses and so
// on. Only the known-spell part is modeled.
class CSWSPlayerLastUpdateObject {
public:
#if !KOTOR1  // K1 has no components or chemicals; its block opens at the gold
    int            m_nComponent;
    int            m_nChemical;
#endif
    int            m_nGold;
    unsigned short m_nArmorClass;
    short          m_nEncumbranceState;
    int            m_bCanLevelUp;
    int            m_nLevel;
    int            m_nExperience;
    unsigned short m_nGuiSkills;

    CExoArrayList<unsigned short> m_feats;
    CExoArrayList<unsigned short> m_bonusFeats;
    CExoArrayList<CSWSStats_FeatRemainingUses*> m_featRemainingUses;
    unsigned short m_nPlayerState;

    CSWActionQueueEntry m_actionQueue[NUM_ACTION_QUEUE_ENTRIES];

    CExoArrayList<CSWVisibilityNode*> m_visibilityList;

    CExoArrayList<unsigned int> m_knownSpells[2][1];
    CExoArrayList<unsigned int> m_spellDeleteList[2][1];
    CExoArrayList<unsigned int> m_spellAddList[2][1];

    unsigned char m_knownSpellUsesLeft[2];

    CExoArrayList<CSWSStats_SpellLikeAbility>* m_pSpellLikeAbilityList;

    unsigned short GetNumberKnownSpells(unsigned char nClass, unsigned char nLevel)
    {
        return static_cast<unsigned short>(m_knownSpells[nClass][nLevel].m_count);
    }

    CSWSPlayerLastUpdateObject();
    ~CSWSPlayerLastUpdateObject();

    int           GetKnownSpell(unsigned char nClass, unsigned char nLevel,
                                unsigned char nIndex);
    void          AddKnownSpell(unsigned char nClass, unsigned char nLevel,
                                unsigned int nSpell);
    void          ClearKnownSpells();
    void          ClearSpellAddDeleteLists();
    void          ClearVisibilityList();
    void          ClearFeats();
    void          ClearActionQueue();
    void          ClearKnownSpellUsesLeft();
    unsigned char GetKnownSpellUsesLeft(unsigned char nClass, unsigned char nLevel);
    void          SetKnownSpellUsesLeft(unsigned char nClass, unsigned char nLevel,
                                        unsigned char nUses);
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSPlayerLastUpdateObject) == 0x210, "K1 CSWSPlayerLastUpdateObject");
#endif

#include "exoresman.h"

// idiom-exempt: canonical
// The server-side snapshot of everything last sent to one player's client about one other party
// member: the display values the party bar and party sheet need, not a replicated world object.
class CLastUpdatePartyObject {
public:
    CResRef            m_portrait;
    unsigned short     m_nPortraitId;
    int                m_bTouchedThisFrame;
    unsigned long long m_objectId;

    unsigned short     m_nAIState;
    unsigned char      m_nAIStateAction;
    char               _pad23[0x24 - 0x23];
    unsigned short     m_nActivityFlags;
    int                m_oidAIStateTarget;

    unsigned short     m_nCurrentHitPoints;
    unsigned short     m_nHitPoints;
    unsigned short     m_nHitPointBonus;
    unsigned short     m_nMaxHitPoints;

    unsigned char      m_nAIStateReputation;
    int                m_bCombatState;
    unsigned char      m_nStealthMode;

    unsigned long long m_oidArea;
    // The writer refreshes x and y only, so z is compared against the origin rather than against the last
    // value sent.
    Vector             m_position;

    int                m_bPlayerCharacter;
    int                m_bAIStateControllable;
    int                m_bIsPartyLeader;
    int                m_nSingletonParty;
    int                m_bInvitedToParty;
    int                m_bPoisoned;
    int                m_bDiseased;

    unsigned short     m_nCurrentForcePoints;
    unsigned short     m_nBaseForcePoints;
    unsigned short     m_nTemporaryForcePoints;
    unsigned short     m_nMaxForcePoints;
};

// One entry of the distance-sorted object list the message layer builds for a player each update frame.
// The producer returns an array of these and reports the count through an out-parameter, so the struct
// holds no count of its own.
class CSWSPlayerLUOSortedObjectList {
public:
    unsigned long long m_objectId;
    float              m_fDistance;
    int                m_nCategory;
};
