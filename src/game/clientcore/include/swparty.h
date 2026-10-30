#pragma once
#include "k2stdint.h"
// idiom-exempt: canonical

// Client-side party roster, a leader plus up to two companions, reached through
// CClientExoApp::GetSWParty(). It tracks the live in-module party -- current members, formation and
// follow positions, per-member pathfind bookkeeping -- as opposed to CSWPartyTable, the save-file
// roster.

#include "Vector.h"
#include "Quaternion.h"
#include "strref.h"
#include "gameobjectarray.h"

class CSWCCreature;

// One formation's per-companion target offsets, three Vectors. DefineFormation writes the second
// and third of an entry and indexes the table as 2 - formationId.
struct FormationDef {
    Vector         vCompanion1Offset;
    Vector         vCompanion2Offset;
    Vector         vReserved;          // neither read nor written
};

// One follow-path sample, a world position plus a scalar value. The position is three bare floats
// rather than a Vector so the sample array can be bulk-zeroed without an element-construction loop;
// the conversions below keep the plain 12-byte struct copy.
struct CSWPartyFollowPos {
    float x, y, z;

    operator const Vector&() const { return *reinterpret_cast<const Vector*>(this); }  // idiom-exempt: layout twin
    CSWPartyFollowPos& operator=(const Vector& v)                     // idiom-exempt: layout twin
    {
        *reinterpret_cast<Vector*>(this) = v;
        return *this;
    }
};

struct CSWPartyFollowData {
    CSWPartyFollowPos m_pos;
    // The leader's world yaw in degrees (not radians) when this sample was recorded.
    float         m_fLeaderYaw;
    // The companions' world formation positions for this sample; ProcessPoint fills both.
    CSWPartyFollowPos m_aFormationPos[2];
};

// Per-member follow-point solver, embedded in the party rather than held by pointer: its methods
// take the party address plus this offset as their receiver.
struct CSWParty;

class CSWPartyFollowDataArray {
public:
    int  GetPointBackFromStart(float distance, Vector& outPos, int& outFlag, int direction);
    // Feeds the leader's just-applied move into the follow sampler: current yaw, the line-of-sight
    // hit fraction and the applied target.
    void ProcessPoint(const Vector& vPos, float fYaw, float fUnused);
    // Re-fits the recorded ring so the companions' follow points stop cutting corners.
    void SmoothFollowPath();
    void SetPathData(int index, const Vector& pos, float value);
    // Walks the recorded path forward by `distance` from the given point and sample index, returning
    // the distance left over after the advance.
    float AdvancePointOnPath(float distance, int direction, const Vector& pos, int sampleIndex,
                             const Vector& followPos, int followIndex,
                             Vector& outPos, int& outIndex);
    // Bounds-checked index into the sample array; the bound is 100.
    CSWPartyFollowData* GetPathData(int index);
    // Distance from the party leader to member `nMember`'s current path point; a nonzero `bSquared`
    // returns the squared distance instead of taking the square root.
    float PointOnLineDistanceFromLeader(int nMember, int bSquared);
    // Four companion pickers, nearest to or furthest back along the recorded follow path,
    // returning 1 or 2, never 0. The _Current pair reads each member's current sample index, the
    // _Target pair its follow-point index; an empty slot scores -1 and always loses.
    int FindClosestPartyMemeber_Current();
    int FindFurthestPartyMemeber_Current();
    int FindClosestPartyMemeber_Target();
    int FindFurthestPartyMemeber_Target();
    // Stamps every sample with the same position and value and rewinds both cursors.
    void Reset(const Vector& pos, float value);
    CSWPartyFollowDataArray();

    // Recorded path ring of exactly 100 samples.
    CSWPartyFollowData m_entries[100];
    // Back-pointer to the owning party.
    CSWParty*          m_pParty;
    int                m_nWriteIndex;  // ring-buffer write cursor, clamped at 99
    // Samples recorded so far, as opposed to the wrapping write cursor.
    int                m_nSampleCount;
};

// One party member's slot inside CSWParty::m_members.
struct CSWPartyMemberData {
        // A member's follow-the-leader state, one per
        // CSWSCreature::WalkUpdateLocation_QuickWalk_FollowLeader_* step. NEW seats the safe
        // position and starts the path search.
        enum FollowState {
            FOLLOW_STATE_NEW                 = -1,
            FOLLOW_STATE_FIND_PATH           = 0,
            FOLLOW_STATE_OFFSET              = 1,
            FOLLOW_STATE_MOVING_TO_PATH      = 2,
            FOLLOW_STATE_FOLLOWING_PATH      = 3,
            FOLLOW_STATE_MOVING_TO_OFFSET    = 4,
            FOLLOW_STATE_WAITING_FOR_PATH    = 5,
            FOLLOW_STATE_OFFSET_WAIT_DEST    = 6,
            FOLLOW_STATE_PATH_WAIT_DEST      = 7,
            FOLLOW_STATE_TO_OFFSET_WAIT_DEST = 8,
            FOLLOW_STATE_TO_PATH_WAIT_DEST   = 9,
            FOLLOW_STATE_TELEPORT_FORWARD    = 10,
        };

        CSWPartyMemberData();
        unsigned long long id;              // OBJECT_INVALID when the slot is empty
        int                m_nFollowState;  // a FollowState; FollowStateChanged turns each
                                            // moving state into its wait-for-destination state
        // How far back along the recorded follow path this member's waiting point sits.
        float              m_followPathDistance;
        int                m_nReserved1;
        int                m_nReserved2;
        int                m_nReserved3;
        Vector              m_vPathPoint;         // second position, written from the path sample by
                                                // TeleportForward
        int                 m_nPathIndex;         // ring index into the follow-path samples
        Vector              m_followPoint;    // written by RecaulateFollowPoint for companions only
        int                 m_bFollowPointValid; // RecaulateFollowPoint's out-value; a follow-sample
                                                  // index despite the name
        // The member's formation-slot offset; InvertFormation negates only the side component, a
        // left/right mirror.
        Vector              m_formationOffset;
        int                m_nStateSteps;           // zeroed together with the follow distance
        float               m_followDistance;   // distance to the follow target; picks the walk or
                                                 // run animation
        Vector              m_followOrientation;
        float              m_fPathSpanRatio;
        Vector              m_safePosition;     // the snapped world position the member was last
                                                 // moved to
        int                m_bReseatSafePosition;
        // Frames spent stuck: cleared on the one path where the member actually advances, and the
        // give-up-and-repath tail fires once it passes 3000.
        unsigned int       m_nStuckTimeMs;
        // Holds the member in its waiting state until the leader is more than seven units away.
        int                m_bWaitForLeaderDistance;
        // The leader's world position when this member was last handed a fresh path.
        Vector             m_vLeaderPathOrigin;
        unsigned long long m_oidLastTarget;        // an object id, seeded to OBJECT_INVALID
        // Pending combat-feedback strref; the sentinel value means there is nothing to say.
        uint64_t      m_nCombatMessageStrRef;

    // Posts a strref to this member's combat feedback line, the same channel the main interface
    // drives client-side.
    void SetCombatMessage(uint64_t nStrRef);

    // Restarts the follow-the-leader state machine (the slot's id and formation offset stay).
    void Reset()
    {
        m_nFollowState = FOLLOW_STATE_NEW;
        m_nStateSteps = 0;
        m_followDistance = 0.0f;
        m_bReseatSafePosition = 0;
        m_followOrientation.x = 0.0f;
        m_nReserved1 = 0;
        m_nReserved2 = 0;
        m_nReserved3 = 0;
        m_vPathPoint.x = 0.0f;
        m_vPathPoint.y = 0.0f;
        m_vPathPoint.z = 0.0f;
        m_followPoint.x = 0.0f;
        m_followPoint.y = 0.0f;
        m_followPoint.z = 0.0f;
        m_bFollowPointValid = 0;
        m_followOrientation.y = 1.0f;
        m_followOrientation.z = 0.0f;
        m_fPathSpanRatio = 1.0f;
        m_oidLastTarget = OBJECT_INVALID;
        m_nCombatMessageStrRef = STRREF_COMBAT_MODE_ENGAGED;
    }
};

struct CSWParty {
    int                m_nCharacterCount;
    int                m_nReserved1;          // zeroed alongside the count; no reader found
    int                m_bEnabled;         // IsEnabled; seeded to 1
    Vector             m_vReserved1;
    // Set on every exit of AddCharacter that got as far as resolving the creature, including the
    // all-slots-full one, so it is a roster-touched latch rather than a record of the add succeeding.
    int                m_bRosterDirty;
    float              m_fReserved1;
    float              m_fReserved2;
    CSWPartyMemberData m_members[3];       // the leader, then the two companions
    int                m_nFormation;
    float              m_followRangeBias;        // added to or subtracted from the follow range for
                                                   // the two companions' target distance
    unsigned char      m_bFormationInverted;
    unsigned char      _pad1f9[0x1fc - 0x1f9];
    FormationDef       m_formationTable[3];
    // Follow spacing per formation id, read by the current formation id.
    float              m_afFollowSpacing[3];
    // The formation table used for negative ids, indexed by the id's complement with the same stride
    // as the table above.
    FormationDef       m_aCustomFormations[3];
    // Whether a negative formation id is usable, indexed by the complement of the id.
    unsigned char      m_abFormationValid[3];
    unsigned char      _pad2e3;
    // The leader position the follow anchor was last rebuilt from; once the leader has moved at
    // least one unit the anchor slides that distance toward the freshly computed target.
    Vector             m_vLastAnchorLeaderPos;
    // Seconds since the last companion retreat script fired, reset whenever one actually ran.
    float              m_fRetreatScriptTimer;
    // Party's recorded follow anchor, the last leader pose the companion follow points are built
    // from. A following action resolves an offset into a world point as the anchor position plus the
    // offset rotated by the anchor orientation.
    Vector             m_followAnchorPosition;
    Quaternion         m_followAnchorOrientation;
    CSWPartyFollowDataArray m_followData;         // embedded, not a pointer
    // Unmodelled tail slot that no shipped build reads or writes.
    unsigned long long m_nReserved2;

    CSWParty();
    ~CSWParty();

    int   GetFormation();
    void  SetFormation(int formation);   // a negative id of -9999 means "advance to the next one"
    int   IsEnabled();
    void  InvertFormation();
    void  DefineFormation(int formationId, const Vector& a, const Vector& b);
    CSWCCreature* GetPlayerCharacter();
    CSWCCreature* GetCharacter(int index);
    int   IsPartyMember(unsigned long long id);
    int   GetIndex(unsigned long long id);
    void  ClearParty();
    // Needs the stack-protector attribute: the body takes the address of two stack locals.
    void  RecaulateFollowPoint() __attribute__((no_stack_protector));
    void  FollowStateChanged();
    int   PartyHasRace(unsigned short race);
    float GetFollowRange();
    float GetEscapeRange();
    int   IsPartyMemberAware();
    int   IsPartyMemberStealthed();
    int   IsPartyHealthy();
    int   IsPartyInCombat();
    int   IsPartyTogether();
    CSWCCreature* RemoveCharacter(int index);
    int   RemoveCharacter(unsigned long long id);
    Vector GetWorldFormationPosition(const Vector& pos, const Vector& facing, int direction);

    int   AddCharacter(unsigned long long id);           // returns int, which SetInParty
                                                         // tail-returns
    // Overloaded index: a positive value names a slot, -2 the first companion, -3 the first member.
    void  SetLeader(int nIndex);
    void  ResetAllFollowData(const Vector& pos, float yaw, int a);
    // Rewinds the whole follow system to a single point: stamps every recorded path sample with the
    // given position and yaw and rewinds the cursors; with the orientation flag set it also rebuilds
    // the follow anchor orientation from the yaw. ResetAllFollowData is the wrapper that additionally
    // drags the live creatures to safe locations.
    void  Reset(const Vector& pos, float yaw, int bBuildOrientation);
    // Runs a follow-path sanity check when the leader doubles back: builds the heading the newest
    // sample was laid down with and dots it against the closest companion's follow orientation;
    // when the two oppose it re-solves the follow points, snaps each companion onto its own and
    // mirrors the formation. Returns 1 when it reset something, 0 when the path was still fine.
    int   TestAndResetPath();

    // Runs the client main loop's per-frame party tick over the leader's force-power effects, the
    // follow anchor and the companion proximity scripts.
    void Update(float fDelta);
};
