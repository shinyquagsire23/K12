#pragma once

#include "nwsobject.h"
#include "strref.h"

class CResGFF;
struct CResStruct;
class CSWCObject;

// One entry's camera block, the payload of the DLG GFF list element LoadDialogCamera reads.
struct CSWSDialogCamera {
    // m_nCameraAnimation when the node names no camera animation.
    enum {
        CAMERA_ANIMATION_NONE = 0x7fff,
    };

    CExoString     m_sListener;
    unsigned int   m_nCameraAngle = 0;     // a value of 6 drops CameraID
    int            m_nCameraID = 0;
    float          m_fHeightOffset = 0.0f;
    float          m_fTargetHeightOffset = 0.0f;
    unsigned short m_nCameraAnimation = CAMERA_ANIMATION_NONE;
    float          m_fFieldOfView = -1.0f;
    int            m_nCamVidEffect = -1;
    // Alien VO variation for this node; the array-new loop leaves it untouched.
#if !KOTOR1
    int            m_nAlienVOVariation;
#endif
};

// One participant of the conversation, held in CSWSDialog::m_lstParticipants.
struct CSWSDialogPlayer {
    unsigned int       m_nPlayerId;
    unsigned long long m_objectId;
    unsigned char      m_nGender;
};

// One {object-id, name} record of CSWSDialog::m_paSpeakers.
struct CSWSDialogSpeaker {
    unsigned long long m_objectId;   // OBJECT_INVALID until resolved
    CExoString         m_sName;
    CSWSDialogSpeaker() : m_objectId(OBJECT_INVALID) {}
};

// One node of CSWSDialog::m_paStuntModels, the DLG's per-model list.
struct CSWSDialogStuntModel {
    CExoString m_sObject;   // "PLAYER", "OWNER", empty, or an object name
    CResRef    m_rModel;
};

// One link of the DLG's top-level StartingList / ReplyList chains.
struct CSWSDialogLinkEntry {
    CResRef m_rActive;      // the first condition script
    unsigned int m_nIndex = 0;  // index into CSWSDialog::m_paEntries
#if !KOTOR1  // K1 links carry no script parameters or second condition
    int     m_nParam1;
    int     m_nParam2;
    int     m_nParam3;
    int     m_nParam4;
    int     m_nParam5;
    CResRef m_rActive2;     // the second condition script
    int     m_nParam1b;
    int     m_nParam2b;
    int     m_nParam3b;
    int     m_nParam4b;
    int     m_nParam5b;
    unsigned char m_bNot;   // inverts the first script's result
    unsigned char m_bNot2;  // inverts the second script's result
    char    _pad4e[0x50 - 0x4e];
    int     m_nLogic;       // 0 combines the two results with AND, any other value with OR
    CExoString m_sParamStrA;  // the first script's string parameter
    CExoString m_sParamStrB;  // the second script's
#endif
};

// One element of CSWSDialogBase::m_paAnimList.
struct CSWSDialogAnim {
    CExoString     m_sParticipant;
    unsigned short m_nAnimation;
};

// The prefix every DLG EntryList and ReplyList element shares, filled by LoadDialogBase.
//
// The members with defaults are the ones the implicit default constructor seeds; the rest are left
// indeterminate and LoadDialogBase arms them later.
struct CSWSDialogBase {
    // m_nWaitFlags: the DLG's WaitFlags, plus the bit LoadDialogBase sets when the node gives its
    // own Delay (then no delay is estimated from the text).
    enum : unsigned int {
        WAIT_FLAG_FIXED_DELAY = 0x10,
    };

    // m_nSoundExists when the node has no SoundExists field.
    enum {
        SOUND_EXISTS_UNSET = 0x80,
    };

    CExoString    m_sSpeaker;
    CExoLocString m_sText;
    CResRef       m_rScript;
#if !KOTOR1  // K1 nodes have no action-script parameters, Script2 or NodeUnskippable
    int           m_nActionParam1;
    int           m_nActionParam2;
    int           m_nActionParam3;
    int           m_nActionParam4;
    int           m_nActionParam5;
    CResRef       m_rScript2;
    int           m_nActionParam1b;
    int           m_nActionParam2b;
    int           m_nActionParam3b;
    int           m_nActionParam4b;
    int           m_nActionParam5b;
    CExoString    m_sActionParamStrA;
    CExoString    m_sActionParamStrB;
    int           m_nNodeUnskippable;
#endif
    unsigned int  m_nWaitFlags = 0;
    unsigned int  m_nDelay = 0;
    int           m_nEstimateDelay;             // armed to -1 unconditionally by LoadDialogBase
    int           m_bVOResRefUnused = 0;         // armed to 1 on the branch that rejects the VO_ResRef lookup
    int           m_nReserved1 = 0;         // never written
    CResRef       m_rVO_ResRef;
    CExoString    m_sReserved2;            // needs a destructor; never written
    CExoString    m_sQuest;
    unsigned int  m_nQuestEntry = 0;
    int           m_nPlotIndex;
    float         m_fPlotXPPercentage;
    unsigned int  m_nAnimCount = 0;
    CSWSDialogAnim* m_paAnimList = 0;
    unsigned char m_nFadeType;
    char          _padE9[0xec - 0xe9];
    Vector        m_vFadeColor;
    float         m_fFadeLength;
    float         m_fFadeDelay;
    unsigned char m_nSoundExists = 0;
#if !KOTOR1
    char          _pad101[0x104 - 0x101];
    int           m_nAlienRaceNode = 0;
    int           m_nEmotion = 4;
    int           m_nFacialAnim = 0;
#endif
};

// One element of the per-entry reply array at CSWSDialogEntry::m_paReplies.
struct CSWSDialogReplyLink : CSWSDialogLinkEntry {
    int m_nIsChild = 0;
};

// cl.exe has no warning-free spelling of the attribute before C++20 (C4848), so under cl.exe K1's
// reply count follows the camera instead of sitting in its tail padding.
#if defined(_MSC_VER) && !defined(__clang__)
#define K2_NO_UNIQUE_ADDRESS
#else
#define K2_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

// One element of the DLG's EntryList array, holding the CSWSDialogBase prefix, a camera block, then
// the entry's own reply array.
struct CSWSDialogEntry : CSWSDialogBase {
#if KOTOR1  // the reply count sits in the camera's tail padding
    K2_NO_UNIQUE_ADDRESS CSWSDialogCamera m_camera;
#else
    CSWSDialogCamera m_camera;
#endif
    unsigned int     m_nReplyCount = 0;
    CSWSDialogReplyLink* m_paReplies = 0;
};

// One element of the DLG's ReplyList array; the layout matches CSWSDialogEntry's apart from the
// trailing field.
struct CSWSDialogReply : CSWSDialogBase {
#if KOTOR1
    K2_NO_UNIQUE_ADDRESS CSWSDialogCamera m_camera;
#else
    CSWSDialogCamera m_camera;
#endif
    unsigned int     m_nEntryCount = 0;
    CSWSDialogLinkEntry* m_paEntries = 0;
    unsigned int     m_nLinkIndex = 0xffffffff;   // reply index last handled, -1 when unanswered
};

// The sizes above are load-bearing: LoadDialog's array strides and Cleanup's teardown sequences
// depend on them.
K2_LAYOUT_ASSERT(sizeof(CSWSDialogAnim) == 0x18, "DLG AnimList element stride");
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSDialogLinkEntry) == 0x14, "K1 DLG link stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogReplyLink) == 0x18, "K1 EntryList element's reply-array stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogBase) == 0xb0, "K1 DLG element prefix");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSDialogEntry, m_nReplyCount) == 0xdc, "K1 reply count");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogEntry) == 0xe8, "K1 EntryList element stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogReply) == 0xf0, "K1 ReplyList element stride");
#else
K2_LAYOUT_ASSERT(sizeof(CSWSDialogLinkEntry) == 0x78, "DLG link stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogReplyLink) == 0x80, "EntryList element's reply-array stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogBase) == 0x110, "DLG element prefix");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogEntry) == 0x150, "EntryList element stride");
K2_LAYOUT_ASSERT(sizeof(CSWSDialogReply) == 0x158, "ReplyList element stride");
#endif

// A server-side active conversation between objects.
class CSWSDialog {
public:
    // An entry or reply index that names no node (m_nHandledReply before the current entry has
    // been answered), and the player id that addresses every participant.
    enum : unsigned int {
        NODE_INDEX_NONE = 0xffffffff,
        PLAYER_ID_ALL   = 0xffffffff,
    };

    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 2 slots
    virtual ~CSWSDialog();  // slots 0-1 (D2+D0)
    // </VTABLE-GEN>
    // Declared and never defined: the class has by-value users.
    CSWSDialog();
    void SetDialogWaitFlags(CSWSObject* pObj, unsigned int flags);
    void SetDialogOwner(CSWSObject* pObj);
    // Writes one journal entry on a player's creature and on every member of its faction.
    void AddJournalEntry(const CExoString& sEntry, unsigned int nState, unsigned int nPlayerId);
    // Sends the conversation's stunt models to a player; always returns 1.
    int  SendDialogModels(CSWSObject* pObj, unsigned int nPlayerId);
    // Resolves one stunt model to its client-side object id; an out-of-range index yields
    // OBJECT_INVALID. nFlag is an out status.
    unsigned long long GetDialogModel(int nIndex, int& nFlag);
    // Removes one participant by object id; returns nonzero while the dialog still has players.
    int  RemovePlayer(unsigned long long objectId);
    // Delivers one player's choice from the reply list. nEntry is the entry index the client is
    // answering about; isEndDialog bypasses the one-shot reply gate and ends the listener's own
    // conversation; flag == 0 runs the reply's delay and wait-flag block.
    int  HandleReply(unsigned int listenerId, CSWSObject* pObj, unsigned int replyIndex,
                     int isEndDialog, unsigned int nEntry, int flag);
    // Pushes the current node's reply list to a player (-1 means every participant). Every link is
    // evaluated with the Not/Not2/Logic chain; with nobody left on the participant list it answers
    // the entry itself. Returns nonzero while the conversation can continue.
    int  SendDialogReplies(CSWSObject* pObj, unsigned int nPlayerId);
    // Ships one reply list of the current entry to one player. Returns 0 when the reply texts are a
    // mix of empty and non-empty, which the client cannot render.
    int  SendDialogReplyNode(unsigned int nPlayerId, unsigned long long objectId,
                             unsigned char nGender, CSWSDialogReply* pReplies,
                             unsigned int nParam5, unsigned int nParam6,
                             unsigned int nEntry, int bBark);
    // Ships one dialog entry. nStatus is an out status; 10 means deferred, not failed.
    int  SendDialogEntry(CSWSObject* pObj, unsigned int a, unsigned int nEntry,
                         unsigned int& nStatus);
    // Sends one entry node: resolves the entry's AnimList into client object ids, randomizes the
    // camera angle and alien VO variation, hides the participants' equipped items, then hands
    // everything to CGuiInGame::HandleDialogEntry. Returns 1 unconditionally; nResult carries the
    // outcome (0 sent, or 10 deferred because an object in the entry has no client yet).
    int  SendDialogEntryNode(unsigned int nEntry, unsigned long long objIdA,
                             unsigned long long objIdC, unsigned long long objIdB,
                             unsigned long long objIdD, unsigned char gender,
                             CSWSObject* pObj, CSWSDialogEntry& entry,
                             unsigned int& nResult);
    // Runs one of the conversation's two end-of-dialog scripts, then notifies the participants.
    // bBark limits the notification to the dialog owner.
    void RunEndConversationScript(CSWSObject* pObj, int bBark);
    // Clears one object's dialog ownership, but only when it still names this dialog's owner.
    void ClearDialogOwnerInObject(unsigned long long objectId);
    // Deterministic pseudo-random rotation for the dialog. The Initialize* pair hashes the resref
    // into a 19-entry index, a stride and a direction; the Assign* pair returns the current entry
    // and advances the index.
    int  AssignPseudoRandomCameraAngle();
    int  AssignPseudoRandomAlienVOVariation();
    void InitializePsuedoRandomCameraAngles(CResGFF* pGff);          // "Psuedo" misspelling is upstream
    void InitializePseudoRandomAlienVOVariation(CResGFF* pGff);
    // The localized string for one gender, read in the client app's current language.
    CExoString GetExoStringFromLocString(CExoLocString& sLoc, unsigned char nGender);
    // Awards plot XP to the whole party; nXpKind == -1 or a non-positive amount are no-ops.
    void GiveDialogPlotXP(int nXpKind, float fAmount);
    // Drops every "this player attacked me" mark the conversation accumulated.
    void ClearAllPlayerHostileActions();
    // Re-shows the equipped items and head item of every participant they were hidden from.
    void ReEquipItemsForDialog();
    void ReEquipHItemForDialog();
    // Hides one creature's equipped items (or head item) for the length of the conversation.
    void UnequipItemsForDialog(unsigned long long objectId);
    void UnequipHItemForDialog(unsigned long long objectId);
    // Runs one dialog action script against pObj and reports whether it succeeded; an empty resref
    // counts as success.
    int  CheckScript(CSWSObject* pObj, const CResRef& rScript);
    // Measures the pause before a line is spoken, applies it, and returns it.
    float SetDialogDelay(CSWSObject* pObj, CExoLocString& sLoc, unsigned int a, int b,
                         unsigned int c);
    // Parses a "DLG " GFF into this dialog. Returns nonzero on success.
    int  LoadDialog(CResGFF* pGff, int a);
    // Reads the base block the DLG's EntryList and ReplyList elements share, including the per-node
    // AnimList. nDefaultDelay is the DLG root's own delay; nReadOk only reports the Text and
    // AnimList reads.
    void LoadDialogBase(CSWSDialogBase& base, CResGFF* pGff, CResStruct& sStruct,
                        unsigned int& nDefaultDelay, int& nReadOk);
    // Reads one camera block of the DLG GFF. Neither `this` nor the trailing `int&` is used.
    void LoadDialogCamera(CSWSDialogCamera& camera, CResGFF* pGff, CResStruct& sStruct,
                          int& nUnused);
    // Reads one StartingList / ReplyList link out of the DLG GFF. A link whose Index is at or above
    // nMaxIndex clears the success flag passed in; the stored index is not clamped.
    void LoadDialogLinkedNode(CSWSDialogLinkEntry& link, CResGFF* pGff, CResStruct& sStruct,
                              int& nReadOk, unsigned int nMaxIndex);
    // Barks one entry of the conversation as a single floating line.
    void BarkOneLine(int nEntry);
    // The entry StartDialog opens on.
    unsigned int GetStartEntry(CSWSObject* pObj);
    // Resolves a speaker from a name (the object's own tag, else the nearest object of that name
    // within CSWSArea::MAX_OBJECT_CONVERSATION_DISTANCE), caches the result in m_paSpeakers, and
    // hands the object back only after claiming it as this dialog's owner. Returns null for an
    // object outside the server array, in another area, too far away unless flagged
    // ForceAlwaysUpdate, or already claimed by a live third party.
    CSWSObject* GetSpeaker(CSWSObject* pObj, const CExoString& sName);
    // Tears the whole conversation down: restores the owner's saved AI level, drops every
    // participant's dialog claim, closes the client's dialog panel and frees all five heap arrays.
    void Cleanup();
    // Resolves the first entry that passes its conditions, with its text, VO sound and action
    // scripts. Nonzero on success.
    int  GetStartEntryOneLiner(CSWSObject* pObj, CExoLocString& sText, CResRef& rSound,
                               CResRef& rScript);
    // Fires one dialog action script against pObj; the result is discarded by both callers.
    void RunScript(CSWSObject* pObj, const CResRef& rScript);

    // -- the three top-level lists, each a {count, array} pair the constructor zeroes. LoadDialog
    // loads them in the order EntryList, ReplyList, StartingList, and GetStartEntryOneLiner walks the
    // third, indexing the first with each link's m_nIndex.
    unsigned int       m_nEntryCount;
    CSWSDialogEntry*   m_paEntries;
    unsigned int       m_nReplyCount;
    CSWSDialogReply*   m_paReplies;
    unsigned int       m_nStartCount;
    CSWSDialogLinkEntry* m_paStartList;
    // The conversation's two end-of-dialog scripts. RunEndConversationScript picks between them
    // oddly: the abort script is the one a normal, non-bark conversation end runs.
    CResRef            m_rScriptEndConversation;
    CResRef            m_rScriptEndConverAbort;
    int                m_bSkippable;
    int                m_bUnequipItems;
    int                m_bUnequipHItem;
    unsigned int       m_nModelCount;
    CSWSDialogStuntModel* m_paStuntModels;
    CResRef            m_rCameraModel;
    // Set once the conversation is finished.
    int                m_bEnded;
    int                m_bNodeUnskippable;
    CExoLinkedList<CSWSDialogPlayer> m_lstParticipants;
    int                m_bRepliesPending;
    // Set when this conversation runs its own end script; raised by BarkOneLine only.
    int                m_bBark;
    unsigned int       m_nCurrentEntry;
    unsigned int       m_nHandledReply;    // -1 until the current entry has been answered
    unsigned char      m_nPlayerGender;
    char               _pad0a1[0xa8 - 0xa1];
    unsigned long long m_oidCurrentPlayer;
    unsigned long long m_ownerId;
    // Speaker cache, a heap array plus a count, with no capacity field.
    CSWSDialogSpeaker* m_paSpeakers;
    unsigned int       m_nSpeakerCount;
    int                m_bFirstPlayerOnly;
    int                m_bPrivate;          // StartDialog's third parameter, never read back
    char               _pad0cc[0xd0 - 0xcc];
    unsigned long long m_oidLastEntrySpeaker;
    unsigned long long m_oidLastReplySpeaker;
    // -- the pseudo-random camera-angle rotation state: seeded from a hash of the dialog's own
    // resref, wrapping into [0, 18] --
    int                m_nCameraAngle;
    int                m_nCameraAngleDelta;
    int                m_bCameraAngleForward;
    int                m_anCameraAngles[19];
    // -- the pseudo-random alien VO-variation rotation state, same shape as the camera-angle one --
#if !KOTOR1  // K1 has no alien VO variation
    int                m_nAlienVOVariation;
    int                m_nAlienVOVariationDelta;
    int                m_bAlienVOForward;
    int                m_anAlienVOVariations[19];
#endif
    int                m_bFirstEntryLine;
    int                m_bFirstReplyLine;
    CResRef            m_rAmbientTrack;
    // Raised the instant RunEndConversationScript runs the "EndConversation" script, so a second call
    // for the same conversation skips to the abort script.
    int                m_bRanEndScript;
    int                m_bNeverBark;
#if !KOTOR1  // K1: no computer-conversation flag, alien race owner or animation object
    int                m_bComputerConversation;
    int                m_nAlienRaceOwner;
    // Never written a non-null value; the destructor stops two "emotions" animations through it.
    CSWCObject*        m_pAnimObject;
#endif
    // -- the two lists of items this conversation temporarily hid, one heap-allocated client object
    // id per participant --
    CExoLinkedList<unsigned long long> m_lstUnequippedItems;
    CExoLinkedList<unsigned long long> m_lstUnequippedHItems;
    int                m_ownerSavedAiLevel;   // saved before it is bumped to 3 or more
    int                m_nParticipantWaitMs;
    int                m_nParticipantWaitDay;
    int                m_nParticipantWaitTod;
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSDialog) == 0x178, "K1 CSWSDialog (ctor @0x465288)");
#endif
