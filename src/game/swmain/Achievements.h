#pragma once
#include "k2stdint.h"

// The port's achievement-unlock dispatcher, shared app glue reached from both the party subsystem
// and the creature-stats subsystem.
class Achievements {
public:
    // The cloud achievement number ("achievement_%d"), with its title in the Steam schema.
    enum AchievementId {
        ACHIEVEMENT_COMPLETE_PROLOGUE = 0,               // Training Wheels
        ACHIEVEMENT_DESTROY_PERAGUS_ASTEROIDS = 1,       // It Was Like That When I Got Here!
        ACHIEVEMENT_SITH_DESTROY_PERAGUS_ASTEROIDS = 2,  // I Didn't Do It!
        ACHIEVEMENT_FIRST_LIGHT_SIDE_POINTS = 3,         // Larger World
        ACHIEVEMENT_FIRST_DARK_SIDE_POINTS = 4,          // Starting Down the Dark Path
        ACHIEVEMENT_FIRST_LIGHTSABER = 5,                // An Elegant Weapon
        ACHIEVEMENT_DEFEAT_NIHILUS = 6,                  // Hunger Strike
        ACHIEVEMENT_DEFEAT_SION = 7,                     // Pain Relief
        ACHIEVEMENT_LIGHT_SIDE_ENDING = 8,               // Seeker
        ACHIEVEMENT_DARK_SIDE_ENDING = 9,                // Destiny, Dominated
        ACHIEVEMENT_HK47_UNIQUE_MODULE = 10,             // Short Circuit
        ACHIEVEMENT_ONDERON_SIDE_WITH_QUEEN = 11,        // Royal Protocol
        ACHIEVEMENT_ONDERON_SIDE_WITH_VAKLU = 12,        // Martial Law
        ACHIEVEMENT_HANDMAIDEN_BATTLE_CIRCLE = 13,       // The First Rule...
        ACHIEVEMENT_MANDALORIAN_BATTLE_CIRCLE = 14,      // The Second Rule...
        ACHIEVEMENT_REPAIR_EBON_HAWK_DROID = 15,         // Unadulterated Violence
        ACHIEVEMENT_LEARN_MOVING_MEDITATION = 16,        // Lost in Your Work
        ACHIEVEMENT_LEARN_FORCE_SIGHT = 17,              // Your Eyes Can Deceive You
        ACHIEVEMENT_RECRUIT_HANHARR = 18,                // The Walking Carpet
        ACHIEVEMENT_RECRUIT_MIRA = 19,                   // Lost Girl
        ACHIEVEMENT_LOOT_KORRIBAN_BODY = 20,             // Grave Robber
        ACHIEVEMENT_FLEE_KORRIBAN = 21,                  // Fight Another Day
        ACHIEVEMENT_BREAK_HANHARR = 22,                  // Laugh It Up, Fuzzball
        ACHIEVEMENT_BEAT_PAZAAK_CHAMP = 23,              // Pure Pazaak
        ACHIEVEMENT_NAMED_LIGHTSABER_CRYSTAL = 24,       // Pet Rock
        ACHIEVEMENT_KHOONDA_SIDE_WITH_SETTLERS = 25,     // Last Stand
        ACHIEVEMENT_KHOONDA_ABANDON_SETTLERS = 26,       // Nothing Personal
        ACHIEVEMENT_RECRUIT_HANDMAIDEN = 27,             // Orphan White
        ACHIEVEMENT_RECRUIT_DISCIPLE = 28,               // Ancient History
        ACHIEVEMENT_PRESTIGE_CLASS = 29,                 // A Certain Set of Skills
        ACHIEVEMENT_FULL_LIGHT_SIDE = 30,                // Luminous Beings
        ACHIEVEMENT_FULL_DARK_SIDE = 31,                 // If You Only Knew...
        ACHIEVEMENT_PAZAAK_NINE_CARDS = 32,              // Never Tell Me the Odds
        ACHIEVEMENT_TEN_THOUSAND_CREDITS = 33,           // In It For the Money
        ACHIEVEMENT_PERSUADE_30 = 34,                    // Talk Them to Death
        ACHIEVEMENT_DONT_GET_COCKY = 35,                 // Don't Get Cocky
        ACHIEVEMENT_TEN_PARTY_MEMBERS = 36,              // The Gang's All Here
        ACHIEVEMENT_INFLUENCE_ZERO = 37,                 // I hate everything about you
        ACHIEVEMENT_ALL_DARK_SIDE_POWERS = 38,           // Unlimited Power
        ACHIEVEMENT_ALL_LIGHT_SIDE_POWERS = 39,          // I Am A Jedi
        ACHIEVEMENT_DANCE_FOR_VOGGA = 40,                // Dancing Queen
        ACHIEVEMENT_KILL_JEDI_MASTERS = 41,              // No Jedi can stop us
        ACHIEVEMENT_LEVEL_30 = 42,                       // Over Achiever
        ACHIEVEMENT_STEALTH_30 = 43,                     // Silent, But Deadly
        ACHIEVEMENT_DEMOLITIONS_30 = 44,                 // Let's Blow This Place!
        ACHIEVEMENT_REPAIR_30 = 45,                      // I've Seen How You Use a Hydrospanner
        ACHIEVEMENT_COMPUTER_USE_30 = 46,                // We Have to Hack Into the Mainframe!
        ACHIEVEMENT_TREAT_INJURY_30 = 47,                // Trust Me, I'm a Doctor
        ACHIEVEMENT_TRAPPED_QUEST = 48,                  // It's a trap!
        ACHIEVEMENT_VOGGA_FUEL = 49,                     // Hutt Oil
        ACHIEVEMENT_CHAMP_THROWS_GAME = 50,              // Cheater
        ACHIEVEMENT_NIHILUS_MASK = 51,                   // Finders Keepers
        ACHIEVEMENT_SITH_LORD_MAIN_MENU = 52,            // The Sith Lord
        ACHIEVEMENT_TEACH_HANDMAIDEN = 53,               // Breaking the Oath
        ACHIEVEMENT_GOTO_IDENTITY = 54,                  // You ARE the droid I'm looking for
        ACHIEVEMENT_HK47_KILL_JEDI = 55,                 // Assassination Protocol: Active
        ACHIEVEMENT_HK47_LOVE = 56,                      // Cupid's Rifle
    };
    static bool UnlockAchievement(AchievementId id);
    // The real unlocker, keyed by the cloud tag the id overload formats.
    static bool UnlockAchievement(char const* pTag);
    static void EvaluateAchievementMovie(const char* movie);
    // Per-strref dispatcher, called with the strref before it is resolved.
    static void EvaluateAchievement(uint64_t strRef);
    // Fired when a conversation ends.
    static void OnEndDialog();
    // Fired when the player picks a dialog reply.
    static void OnDialogOptionChosen(int nOption);
    // Fired on a save load to clear the five "dialog already seen" latches below.
    static void ResetOnLoad();
    // Clears the whole achievement backlog; an empty body in this build.
    static void ResetAchievements();

    // The five per-dialog latches the two functions above use.
private:
    static bool s_dontGetCockyFailed;
    static bool s_dontGetCockyStarted;
    static bool s_buildLightsaberDialog;
    static bool s_itsATrapDialog;
    static bool s_findersKeeperDialog;
};
