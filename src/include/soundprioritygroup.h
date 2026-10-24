#pragma once

// The sound priority groups: rows of prioritygroups.2da, the group a sound source plays in.
enum {
    PRIORITY_GROUP_UNMASKABLE_SOUND             = 0,
    PRIORITY_GROUP_MUSIC_STINGERS               = 1,
    PRIORITY_GROUP_STREAMS_MUSIC                = 2,
    PRIORITY_GROUP_ONE_SHOT_STREAMS             = 3,
    PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS   = 4,
    PRIORITY_GROUP_LOOPING_POSITIONAL_AMBIENTS  = 5,
    PRIORITY_GROUP_LOOPING_PLAYER               = 6,
    PRIORITY_GROUP_LOOPING_NON_PLAYER           = 7,
    PRIORITY_GROUP_PLAYER_CHAT                  = 8,
    PRIORITY_GROUP_NON_PLAYER_CHAT              = 9,
    PRIORITY_GROUP_SCRIPTED_PLAY_SOUND          = 10,
    PRIORITY_GROUP_GUI                          = 11,
    PRIORITY_GROUP_AREA_SPELL_EFFECTS           = 12,
    PRIORITY_GROUP_NORMAL_SPELL_EFFECTS         = 13,
    PRIORITY_GROUP_PROJECTILE                   = 14,
    PRIORITY_GROUP_COMBAT                       = 15,
    PRIORITY_GROUP_LARGE_CREATURE_VOCALIZATIONS = 16,
    PRIORITY_GROUP_SMALL_CREATURE_VOCALIZATIONS = 17, // medium and small creatures
    PRIORITY_GROUP_LARGE_CREATURE_FOOTSTEPS     = 18,
    PRIORITY_GROUP_PLAYER_FOOTSTEPS             = 19,
    PRIORITY_GROUP_SMALL_CREATURE_FOOTSTEPS     = 20, // medium and small creatures
    PRIORITY_GROUP_SINGLE_SHOT_GLOBAL           = 21,
    PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL       = 22,
    PRIORITY_GROUP_DEFAULT                      = 23,
    PRIORITY_GROUP_MINIGAME_ENEMY               = 24,
    PRIORITY_GROUP_ROOM_SOUNDS                  = 25,
    PRIORITY_GROUP_BARK_BUBBLES                 = 26,
};
