#pragma once

// Wire ids of the client/server message layer, shared by the sending and the receiving side.
// Every message opens with three header bytes: the direction, the major (the message family) and
// the minor (the message within the family). The minor families below are listed per major; an id
// that only travels one way says so.

// Header byte 0.
enum NWMessageDirection {
    MSG_SERVER_TO_PLAYER = 'P',
    MSG_PLAYER_TO_SERVER = 'p',
};

// The player id a message is built for before its destination is known; the same value as
// CNetLayer::PLAYERID_ALL_CLIENTS, which is an out-of-line object and so cannot stand in for it.
enum NWMessagePlayerId : unsigned int {
    MSG_PLAYERID_ALL_CLIENTS = ~0u,
};

// Header byte 1.
enum NWMessageMajor {
    MSG_MAJOR_SERVER_STATUS              = 1,
    MSG_MAJOR_LOGIN                      = 2,
    MSG_MAJOR_MODULE                     = 3,
    MSG_MAJOR_AREA                       = 4,
    MSG_MAJOR_GAME_OBJECT_UPDATE         = 5,
    MSG_MAJOR_INPUT                      = 6,
    MSG_MAJOR_STORE                      = 7,
    MSG_MAJOR_GOLD                       = 8,
    MSG_MAJOR_CHAT                       = 9,
    MSG_MAJOR_PLAYER_LIST                = 10,
    MSG_MAJOR_INVENTORY                  = 12,
    MSG_MAJOR_GUI_INVENTORY              = 13,
    MSG_MAJOR_PARTY                      = 14,
    MSG_MAJOR_CHEAT                      = 15,
    MSG_MAJOR_CAMERA                     = 16,
    MSG_MAJOR_CHAR_LIST                  = 17,
    MSG_MAJOR_CLIENT_SIDE_MESSAGE        = 18,
    MSG_MAJOR_COMBAT_ROUND               = 19,
    MSG_MAJOR_DIALOG                     = 20,
    MSG_MAJOR_GUI_CHARACTER_SHEET        = 21,
    MSG_MAJOR_QUICK_CHAT                 = 22,
    MSG_MAJOR_SOUND                      = 23,
    MSG_MAJOR_ITEM_PROPERTY              = 24,
    MSG_MAJOR_GUI_CONTAINER              = 25,
    MSG_MAJOR_VOICE_CHAT                 = 26,
    MSG_MAJOR_GUI_INFO_POPUP             = 27,
    MSG_MAJOR_JOURNAL                    = 28,
    MSG_MAJOR_LEVEL_UP                   = 29,
    MSG_MAJOR_GUI_QUICKBAR               = 30,
    MSG_MAJOR_MAP_PIN                    = 32,
    MSG_MAJOR_DEBUG_INFO                 = 33,
    MSG_MAJOR_SAFE_PROJECTILE            = 34,
    MSG_MAJOR_POPUP_GUI_PANEL            = 36,
    MSG_MAJOR_DEATH                      = 37,
    MSG_MAJOR_AMBIENT                    = 40,
    MSG_MAJOR_CHARACTER_DOWNLOAD         = 43,
    MSG_MAJOR_LOAD_BAR                   = 44,
    MSG_MAJOR_SAVE_LOAD                  = 45,
    MSG_MAJOR_SHUTDOWN_SERVER            = 47,
    MSG_MAJOR_GUI_TIMING_EVENT           = 48,
    MSG_MAJOR_PLAY_MODULE_CHARACTER_LIST = 49,
    MSG_MAJOR_CUSTOM_TOKEN               = 50,
};

// Header byte 2, one family per major.

// The server's replies name its mode: no module, a module loading, a module running.
enum NWMessageServerStatus {
    MSG_SERVER_STATUS_REQUEST          = 0,    // player to server
    MSG_SERVER_STATUS_MODULE_LOADING   = 1,    // server to player
    MSG_SERVER_STATUS_IDLE             = 2,    // server to player
    MSG_SERVER_STATUS_MODULE_RUNNING   = 3,    // server to player
};

enum NWMessageLogin {
    MSG_LOGIN_LOCAL_CHARACTER          = 1,    // player to server: a character file's bytes
    MSG_LOGIN_SERVER_CHARACTER         = 2,    // player to server: a character resref
    MSG_LOGIN_CONFIRM                  = 5,    // server to player
    MSG_LOGIN_CHARACTER_QUERY          = 10,   // server to player
    MSG_LOGIN_CHARACTER_RESPONSE       = 11,   // player to server
    MSG_LOGIN_SERVER_CHARACTER_JOIN    = 14,   // player to server
    MSG_LOGIN_IFO_CHARACTER            = 15,   // player to server
    MSG_LOGIN_NEED_CHARACTER           = 16,   // server to player
    MSG_LOGIN_SERVER_CHARACTER_CREATE  = 17,   // player to server
    MSG_LOGIN_FAIL                     = 18,   // server to player
    MSG_LOGIN_LOCAL_CHARACTER_JOIN     = 19,   // player to server
};

enum NWMessageModule {
    MSG_MODULE_INFO                    = 1,    // server to player
    MSG_MODULE_LOADED                  = 2,    // player to server
    MSG_MODULE_TIME                    = 3,    // server to player
    MSG_MODULE_SAVE_GAME               = 4,    // player to server
    MSG_MODULE_LOAD_GAME               = 5,    // player to server
    MSG_MODULE_START_START_NEW_MODULE  = 6,    // server to player
    MSG_MODULE_RUN_MODULE              = 7,    // player to server
    MSG_MODULE_LOADING                 = 8,    // server to player
    MSG_MODULE_EXPORT_REPLY            = 10,   // server to player
    MSG_MODULE_DUMP_PLAYER             = 11,   // server to player
    MSG_MODULE_END_START_NEW_MODULE    = 12,   // server to player
    MSG_MODULE_SAVE_GAME_STATUS        = 13,   // server to player
    MSG_MODULE_END_GAME                = 14,   // server to player
};

enum NWMessageArea {
    MSG_AREA_CLIENT_AREA               = 1,    // server to player
    MSG_AREA_VISUAL_EFFECT             = 2,    // server to player
    MSG_AREA_LOADED                    = 3,    // player to server
    MSG_AREA_WEATHER                   = 4,    // server to player
};

enum NWMessageGameObjectUpdate {
    MSG_GAME_OBJECT_UPDATE_UPDATE                 = 1,
    MSG_GAME_OBJECT_UPDATE_OBJECT_CONTROL         = 2,   // server to player
    MSG_GAME_OBJECT_UPDATE_VISUAL_EFFECT          = 3,   // server to player
    MSG_GAME_OBJECT_UPDATE_STRIP_EQUIPPED_ITEMS   = 6,   // server to player
    MSG_GAME_OBJECT_UPDATE_DESTROY_ITEM           = 7,   // server to player
    MSG_GAME_OBJECT_UPDATE_ITEM_SCRIPT_ADD        = 8,   // server to player
};

// Player to server.
enum NWMessageInput {
    MSG_INPUT_WALK_TO_WAYPOINT         = 1,
    MSG_INPUT_ATTACK                   = 2,
    MSG_INPUT_CHANGE_DOOR_STATE        = 3,
    MSG_INPUT_PLAY_ANIMATION           = 4,
    MSG_INPUT_EXAMINE                  = 5,
    MSG_INPUT_USE_FEAT                 = 6,
    MSG_INPUT_USE_SKILL                = 7,
    MSG_INPUT_DIALOG                   = 8,
    MSG_INPUT_USE_ITEM                 = 9,
    MSG_INPUT_TOGGLE_MODE              = 10,
    MSG_INPUT_USE_OBJECT               = 11,
    MSG_INPUT_UNLOCK_OBJECT            = 12,
    MSG_INPUT_REST                     = 13,
    MSG_INPUT_LOCK_OBJECT              = 14,
    MSG_INPUT_STOP_DRAG_MODE           = 15,
    MSG_INPUT_SPELL_MEMORIZE           = 16,
    MSG_INPUT_SPELL_UNMEMORIZE         = 17,
    MSG_INPUT_CAST_SPELL               = 18,
    MSG_INPUT_CANCEL_ACTION            = 20,
    MSG_INPUT_GET_OBJECT_DEBUG_INFO    = 21,
    MSG_INPUT_MERGE_ITEM               = 22,
    MSG_INPUT_SPLIT_ITEM               = 23,
    MSG_INPUT_TOGGLE_PAUSE             = 24,
    MSG_INPUT_SET_PAUSE_REQUEST        = 25,
    MSG_INPUT_ALWAYS_RUN               = 26,
    MSG_INPUT_ASSOCIATE_COMMAND        = 27,
    MSG_INPUT_TURN_ON_SPOT             = 28,
    MSG_INPUT_DRIVE_CONTROL            = 29,
    MSG_INPUT_CANCEL_DISGUISE          = 30,
    MSG_INPUT_EXPORT_REQUEST           = 31,
    MSG_INPUT_ABORT_DRIVE_CONTROL      = 33,
    MSG_INPUT_CANCEL_GUI_TIMING_EVENT  = 34,
    MSG_INPUT_CAST_SPELL_LIKE_ABILITY  = 35,
    MSG_INPUT_GIVE_ITEM                = 36,
    MSG_INPUT_DOOR_SABER               = 37,
    MSG_INPUT_SWITCH_WEAPONS           = 38,
    MSG_INPUT_STANCE                   = 39,
};

// Player to server.
enum NWMessageStore {
    MSG_STORE_REQUEST_BUY              = 1,
    MSG_STORE_REQUEST_SELL             = 2,
    MSG_STORE_CLOSE                    = 5,
};

// Player to server.
enum NWMessageGold {
    MSG_GOLD_TRANSFER                  = 1,
};

enum NWMessageChat {
    MSG_CHAT_TALK                      = 1,
    MSG_CHAT_SHOUT                     = 2,
    MSG_CHAT_WHISPER                   = 3,
    MSG_CHAT_TELL                      = 4,
    MSG_CHAT_SERVER_TELL               = 5,    // server to player
    MSG_CHAT_PARTY                     = 6,
    MSG_CHAT_AI_ACTION_PLAY_SOUND      = 7,    // server to player
    MSG_CHAT_TALK_STRREF               = 8,    // server to player
    MSG_CHAT_SHOUT_STRREF              = 9,    // server to player
    MSG_CHAT_WHISPER_STRREF            = 10,   // server to player
    MSG_CHAT_MULTI_LANGUAGE_TALK       = 11,   // server to player
    MSG_CHAT_MULTI_LANGUAGE_SHOUT      = 12,   // server to player
    MSG_CHAT_SILENT_TALK               = 13,   // not shown; heard by listeners in talk range
    MSG_CHAT_SILENT_SHOUT              = 14,   // not shown; heard by listeners area-wide
    MSG_CHAT_SERVER_TELL_STRREF        = 15,   // server to player
};

// Server to player.
enum NWMessagePlayerList {
    MSG_PLAYER_LIST_ALL                = 1,
    MSG_PLAYER_LIST_ADD                = 2,
    MSG_PLAYER_LIST_DELETE             = 3,
};

enum NWMessageInventory {
    MSG_INVENTORY_EQUIP                = 1,
    MSG_INVENTORY_EQUIP_CANCEL         = 2,    // server to player
    MSG_INVENTORY_DROP                 = 3,
    MSG_INVENTORY_DROP_CANCEL          = 4,    // server to player
    MSG_INVENTORY_PICKUP               = 5,
    MSG_INVENTORY_PICKUP_CANCEL        = 6,    // server to player
    MSG_INVENTORY_UNEQUIP              = 7,
    MSG_INVENTORY_UNEQUIP_CANCEL       = 8,    // server to player
    MSG_INVENTORY_EQUIP_TOGGLE         = 11,   // player to server
    MSG_INVENTORY_USE                  = 12,   // player to server
    MSG_INVENTORY_LEARN_SCROLL         = 13,   // player to server
    MSG_INVENTORY_LEARNED_SCROLL       = 14,   // server to player: the reply to LEARN_SCROLL
    MSG_INVENTORY_CONFIRM_DROP         = 15,   // server to player
};

enum NWMessageGuiInventory {
    MSG_GUI_INVENTORY_STATUS           = 1,
};

enum NWMessageParty {
    MSG_PARTY_LIST                     = 1,    // server to player: the whole party
    MSG_PARTY_LIST_REQUEST             = 2,    // player to server
    MSG_PARTY_LIST_ADD                 = 3,    // server to player
    MSG_PARTY_LIST_REMOVE              = 4,    // server to player
    MSG_PARTY_LEAVE                    = 6,    // player to server
    MSG_PARTY_KICK                     = 7,    // player to server
    MSG_PARTY_TRANSFER_LEADERSHIP      = 8,    // player to server
    MSG_PARTY_INVITE                   = 9,    // both ways: the invitee hears of it from the server
    MSG_PARTY_IGNORE_INVITATION        = 10,   // player to server
    MSG_PARTY_ACCEPT_INVITATION        = 11,   // player to server
    MSG_PARTY_DECLINE_INVITATION       = 12,   // player to server
};

enum NWMessageCheat {
    MSG_CHEAT_MOVE_TO_AREA                     = 1,
    MSG_CHEAT_RESURRECT                        = 2,
    MSG_CHEAT_CAST_SPELL                       = 3,
    MSG_CHEAT_INVULNERABILITY                  = 4,
    MSG_CHEAT_KILL                             = 7,
    MSG_CHEAT_RUN_SCRIPT                       = 8,
    MSG_CHEAT_TOGGLE_COMBAT_DEBUGGING          = 9,
    MSG_CHEAT_PLAY_VISUAL_EFFECT               = 10,
    MSG_CHEAT_NASTY                            = 11,
    MSG_CHEAT_PONY_RIDE                        = 12,
    MSG_CHEAT_RAIN_OF_COWS                     = 13,
    MSG_CHEAT_SUMMON_NASTY                     = 14,
    MSG_CHEAT_TOGGLE_FLOATY_EFFECTS            = 15,
    MSG_CHEAT_SHOW_SERVER_MEM                  = 17,
    MSG_CHEAT_LEVEL_UP                         = 18,
    MSG_CHEAT_DEBUG_MODE                       = 21,
    MSG_CHEAT_TOGGLE_MOVEMENT_SPEED_DEBUGGING  = 22,
    MSG_CHEAT_PLAYER_PATHFIND_RULE             = 25,
    MSG_CHEAT_COMPUTE_SAFE_LOCATION            = 32,
};

// Server to player.
enum NWMessageCamera {
    MSG_CAMERA_CHANGE_LOCATION         = 1,
    MSG_CAMERA_SET_MODE                = 2,
};

enum NWMessageCharList {
    MSG_CHAR_LIST_REQUEST              = 1,    // player to server
    MSG_CHAR_LIST_LIST                 = 2,    // server to player
    MSG_CHAR_LIST_REQUEST_UPDATE       = 3,    // player to server
    MSG_CHAR_LIST_UPDATE_RESPONSE      = 4,    // server to player
};

// Server to player: the combat-log and feedback records CSWSMessage::SendServerToPlayerCCMessage
// packs from a CSWCCMessageData and CSWCMessage::HandleServerToPlayerCCMessage renders.
enum NWMessageClientSideMessage {
    CC_MESSAGE_DAMAGE                  = 2,    // the damaged object only
    CC_MESSAGE_DAMAGE_FROM_ATTACKER    = 3,    // the attacker and the damaged object
    CC_MESSAGE_DEATH                   = 4,
    CC_MESSAGE_SAVING_THROW            = 7,
    CC_MESSAGE_SPELL                   = 8,
    CC_MESSAGE_SKILL                   = 9,
    CC_MESSAGE_FORCE_RESIST            = 10,
    CC_MESSAGE_FEEDBACK                = 11,
    CC_MESSAGE_FORCE_POINT_USAGE       = 12,
    CC_MESSAGE_ATTACK_ROLL             = 13,
    CC_MESSAGE_DISPEL                  = 15,
    CC_MESSAGE_STEALTH_DETECTION       = 16,
    CC_MESSAGE_FORCE_RESISTANCE_ROLL   = 17,
    CC_MESSAGE_ATTACK_SUMMARY          = 18,
    CC_MESSAGE_MINE_DETECTION          = 19,
    CC_MESSAGE_ATTACK_BREAKDOWN        = 20,
    CC_MESSAGE_THREAT_BREAKDOWN        = 21,
    CC_MESSAGE_DEFENSE_BREAKDOWN       = 22,
    CC_MESSAGE_DAMAGE_BREAKDOWN        = 23,
    CC_MESSAGE_CRITICAL_HIT_BREAKDOWN  = 24,
    CC_MESSAGE_EFFECT_BREAKDOWN        = 25,
    CC_MESSAGE_DEFLECT_BREAKDOWN       = 26,
    CC_MESSAGE_POISONED                = 27,
};

// Server to player.
enum NWMessageCombatRound {
    MSG_COMBAT_ROUND_STARTED           = 1,
};

enum NWMessageDialog {
    MSG_DIALOG_ENTRY                   = 1,    // server to player
    MSG_DIALOG_REPLIES                 = 2,    // server to player
    MSG_DIALOG_REPLY                   = 3,    // player to server
    MSG_DIALOG_REPLY_CHOSEN            = 4,    // server to player
    MSG_DIALOG_CLOSE                   = 5,    // server to player
    MSG_DIALOG_MODELS                  = 6,    // server to player
};

enum NWMessageGuiCharacterSheet {
    MSG_GUI_CHARACTER_SHEET_STATUS        = 1,   // player to server
    MSG_GUI_CHARACTER_SHEET_NOT_PERMITTED = 2,   // server to player
};

enum NWMessageQuickChat {
    MSG_QUICK_CHAT_MESSAGE             = 0,
};

// Server to player.
enum NWMessageSound {
    MSG_SOUND_OBJECT_PLAY              = 2,
    MSG_SOUND_OBJECT_STOP              = 3,
    MSG_SOUND_OBJECT_CHANGE_POSITION   = 5,
};

// Server to player.
enum NWMessageItemProperty {
    MSG_ITEM_PROPERTY_UPDATE_ACTIVE    = 1,
};

enum NWMessageGuiContainer {
    MSG_GUI_CONTAINER_OPEN             = 1,    // server to player
    MSG_GUI_CONTAINER_CLOSE            = 2,    // player to server
};

// Server to player.
enum NWMessageVoiceChat {
    MSG_VOICE_CHAT_PLAY                = 1,
};

// Server to player: the examine window's data, one minor per object kind.
enum NWMessageGuiInfoPopup {
    MSG_GUI_INFO_POPUP_ITEM            = 2,
    MSG_GUI_INFO_POPUP_CREATURE        = 3,
    MSG_GUI_INFO_POPUP_PLACEABLE       = 4,
    MSG_GUI_INFO_POPUP_MINE            = 5,
    MSG_GUI_INFO_POPUP_DOOR            = 6,
};

enum NWMessageJournal {
    MSG_JOURNAL_ADD_WORLD              = 1,    // server to player
    MSG_JOURNAL_ADD_WORLD_STRREF       = 2,    // server to player
    MSG_JOURNAL_DELETE_WORLD           = 3,    // server to player
    MSG_JOURNAL_DELETE_WORLD_STRREF    = 4,    // server to player
    MSG_JOURNAL_DELETE_WORLD_ALL       = 5,    // server to player
    MSG_JOURNAL_ADD_QUEST              = 6,    // server to player
    MSG_JOURNAL_REMOVE_QUEST           = 7,    // server to player
    MSG_JOURNAL_SET_QUEST_PICTURE      = 8,    // server to player
    MSG_JOURNAL_FULL_UPDATE            = 9,    // server to player
    MSG_JOURNAL_QUEST_SCREEN_OPEN      = 10,   // player to server
    MSG_JOURNAL_QUEST_SCREEN_CLOSED    = 11,   // player to server
    MSG_JOURNAL_UPDATED                = 12,   // server to player
    MSG_JOURNAL_REQUEST_ADD            = 13,   // player to server
    MSG_JOURNAL_REQUEST_DELETE         = 14,   // player to server
};

enum NWMessageLevelUp {
    MSG_LEVEL_UP_REQUEST               = 0,    // player to server
    MSG_LEVEL_UP_CONFIRMED             = 1,    // server to player
    MSG_LEVEL_UP_REFUSED               = 2,    // server to player
};

// Server to player: the whole bar (every button's payload in turn), or one button.
enum NWMessageGuiQuickbar {
    MSG_GUI_QUICKBAR_SET_ALL_BUTTONS   = 1,
    MSG_GUI_QUICKBAR_SET_BUTTON        = 2,
};

enum NWMessageMapPin {
    MSG_MAP_PIN_ENABLED                = 1,    // server to player
    MSG_MAP_PIN_ADD                    = 2,    // player to server
    MSG_MAP_PIN_REMOVE                 = 3,    // player to server
    MSG_MAP_PIN_ADDED                  = 4,    // server to player: the new pin's reference number
    MSG_MAP_PIN_CHANGE                 = 5,    // player to server
};

// Server to player.
enum NWMessageDebugInfo {
    MSG_DEBUG_INFO_CREATURE            = 1,
    MSG_DEBUG_INFO_ITEM                = 2,
    MSG_DEBUG_INFO_PLACEABLE           = 3,
    MSG_DEBUG_INFO_AREA                = 4,
};

// Server to player.
enum NWMessageSafeProjectile {
    MSG_SAFE_PROJECTILE_SPELL          = 1,
    MSG_SAFE_PROJECTILE_PROJECTILE     = 2,
};

// Server to player.
enum NWMessagePopUpGuiPanel {
    MSG_POPUP_GUI_PANEL_SHOW           = 1,
    MSG_POPUP_GUI_PANEL_DESTROY_DEATH  = 2,
    MSG_POPUP_GUI_PANEL_MESSAGE        = 3,
};

// Player to server.
enum NWMessageDeath {
    MSG_DEATH_RESPAWN                  = 1,
};

// Server to player.
enum NWMessageAmbient {
    MSG_AMBIENT_MUSIC_PLAY             = 1,
    MSG_AMBIENT_MUSIC_SET_DELAY        = 2,
    MSG_AMBIENT_MUSIC_CHANGE_TRACK     = 3,
    MSG_AMBIENT_BATTLE_MUSIC_PLAY      = 4,
    MSG_AMBIENT_BATTLE_MUSIC_CHANGE    = 5,
    MSG_AMBIENT_SOUND_LOOP_PLAY        = 6,
    MSG_AMBIENT_SOUND_LOOP_CHANGE      = 7,
    MSG_AMBIENT_SOUND_VOLUME           = 8,
};

enum NWMessageCharacterDownload {
    MSG_CHARACTER_DOWNLOAD_REQUEST     = 1,    // player to server
    MSG_CHARACTER_DOWNLOAD_REPLY       = 2,    // server to player
    MSG_CHARACTER_DOWNLOAD_FAIL        = 3,    // server to player
};

// Server to player.
enum NWMessageLoadBar {
    MSG_LOAD_BAR_START_STALL_EVENT     = 1,
    MSG_LOAD_BAR_UPDATE_STALL_EVENT    = 2,
    MSG_LOAD_BAR_END_STALL_EVENT       = 3,
};

// Server to player.
enum NWMessageSaveLoad {
    MSG_SAVE_LOAD_STATUS               = 1,
};

enum NWMessageShutDownServer {
    MSG_SHUTDOWN_SERVER_REQUEST        = 0,
};

// Server to player.
enum NWMessageGuiTimingEvent {
    MSG_GUI_TIMING_EVENT_INFO          = 1,
};

enum NWMessagePlayModuleCharacterList {
    MSG_PLAY_MODULE_CHARACTER_LIST_START    = 1,   // player to server
    MSG_PLAY_MODULE_CHARACTER_LIST_STOP     = 2,   // player to server
    MSG_PLAY_MODULE_CHARACTER_LIST_RESPONSE = 3,   // server to player
};

// Server to player.
enum NWMessageCustomToken {
    MSG_CUSTOM_TOKEN_SET               = 1,
    MSG_CUSTOM_TOKEN_SET_LIST          = 2,
};

// The field mask of an object update record ('U'): which blocks follow the object id. The low six
// bits apply to every object kind; the rest are creature-only.
enum NWMessageObjectUpdateField {
    MSG_OBJECT_UPDATE_POSITION          = 1 << 0,
    MSG_OBJECT_UPDATE_ORIENTATION       = 1 << 1,
    MSG_OBJECT_UPDATE_ANIMATION         = 1 << 2,    // a creature adds its remaining path
    MSG_OBJECT_UPDATE_VISUAL_EFFECTS    = 1 << 3,
    MSG_OBJECT_UPDATE_STATE             = 1 << 4,    // trap, door or placeable state
    MSG_OBJECT_UPDATE_PORTRAIT          = 1 << 5,
    MSG_OBJECT_UPDATE_AI_STATE          = 1 << 6,
    MSG_OBJECT_UPDATE_MOVE_RATES        = 1 << 8,
    MSG_OBJECT_UPDATE_COARSE_POSITION   = 1 << 9,
    MSG_OBJECT_UPDATE_HIT_POINTS        = 1 << 10,
    MSG_OBJECT_UPDATE_DAMAGE_LEVEL      = 1 << 11,
    MSG_OBJECT_UPDATE_IDENTITY          = 1 << 12,   // race, gender, alignment, classes
    MSG_OBJECT_UPDATE_AREA_CHANGED      = 1 << 13,
    MSG_OBJECT_UPDATE_PARTY             = 1 << 14,
    MSG_OBJECT_UPDATE_VISIBILITY        = 1 << 15,   // heard, seen, inactive
    MSG_OBJECT_UPDATE_NAME              = 1 << 16,
    MSG_OBJECT_UPDATE_ASSOCIATE_STATE   = 1 << 17,
    MSG_OBJECT_UPDATE_AREA              = 1 << 18,
    MSG_OBJECT_UPDATE_FORCE_POINTS      = 1 << 19,
    MSG_OBJECT_UPDATE_REMOVAL           = 1 << 20,   // the fade-out and keep-corpse settings
};

// The byte behind MSG_OBJECT_UPDATE_ASSOCIATE_STATE: the detect and stealth modes and one bit
// per combat mode. The client never reads the counterspell bit.
enum NWMessageAssociateState {
    MSG_ASSOCIATE_STATE_DETECT                  = 1 << 0,
    MSG_ASSOCIATE_STATE_STEALTH                 = 1 << 1,
    MSG_ASSOCIATE_STATE_PARRY                   = 1 << 2,
    MSG_ASSOCIATE_STATE_POWER_ATTACK            = 1 << 3,
    MSG_ASSOCIATE_STATE_IMPROVED_POWER_ATTACK   = 1 << 4,
    MSG_ASSOCIATE_STATE_FLURRY                  = 1 << 5,
    MSG_ASSOCIATE_STATE_RAPID_SHOT              = 1 << 6,
    MSG_ASSOCIATE_STATE_COUNTERSPELL            = 1 << 7,
};

// The field mask of a creature appearance record ('P'): which appearance fields follow.
enum NWMessageAppearanceField {
    MSG_APPEARANCE_UPDATE_APPEARANCE_ID     = 1 << 0,
    MSG_APPEARANCE_UPDATE_PHENOTYPE         = 1 << 1,
    MSG_APPEARANCE_UPDATE_GENDER            = 1 << 2,
    MSG_APPEARANCE_UPDATE_SKIN_COLOR        = 1 << 3,
    MSG_APPEARANCE_UPDATE_HAIR_COLOR        = 1 << 4,
    MSG_APPEARANCE_UPDATE_TATTOO_COLOR_1    = 1 << 5,
    MSG_APPEARANCE_UPDATE_TATTOO_COLOR_2    = 1 << 6,
    MSG_APPEARANCE_UPDATE_HEAD              = 1 << 7,
    MSG_APPEARANCE_UPDATE_EQUIPMENT         = 1 << 9,    // the head, body and weapon slots
    MSG_APPEARANCE_UPDATE_NAME              = 1 << 10,
    MSG_APPEARANCE_UPDATE_TAIL              = 1 << 11,
    MSG_APPEARANCE_UPDATE_WINGS             = 1 << 12,
    MSG_APPEARANCE_UPDATE_SOUND_SET         = 1 << 13,
    MSG_APPEARANCE_UPDATE_USE_BACKUP_HEAD   = 1 << 14,
    MSG_APPEARANCE_UPDATE_DUPLICATING_HEAD  = 1 << 15,
};

// Every appearance field, and every equipment slot resent from scratch.
enum NWMessageAppearanceAll : unsigned int {
    MSG_APPEARANCE_UPDATE_ALL = ~0u,
};

// The field mask of a player info record ('I'): which of the controlled creature's panels follow.
enum NWMessagePlayerUpdateField {
    MSG_PLAYER_UPDATE_CHARACTER                 = 1 << 0,    // gold, AC, experience, level-up preview
    MSG_PLAYER_UPDATE_GUI_SKILLS                = 1 << 1,
    MSG_PLAYER_UPDATE_FEATS                     = 1 << 2,
    MSG_PLAYER_UPDATE_STATUS                    = 1 << 3,    // an NWMessagePlayerStatus word
    MSG_PLAYER_UPDATE_KNOWN_SPELLS              = 1 << 4,
    MSG_PLAYER_UPDATE_MEMORIZED_SPELLS          = 1 << 5,
    MSG_PLAYER_UPDATE_KNOWN_SPELL_USES          = 1 << 6,
    MSG_PLAYER_UPDATE_MEMORIZED_SPELL_COUNTS    = 1 << 7,
    MSG_PLAYER_UPDATE_ACTION_QUEUE              = 1 << 8,
    MSG_PLAYER_UPDATE_HIT_POINTS                = 1 << 9,
    MSG_PLAYER_UPDATE_ABILITIES                 = 1 << 11,   // ability scores and saving throws
    MSG_PLAYER_UPDATE_RESET_STATS               = 1 << 12,
    MSG_PLAYER_UPDATE_VISIBILITY                = 1 << 13,
    MSG_PLAYER_UPDATE_SPELL_LIKE_ABILITIES      = 1 << 14,
    MSG_PLAYER_UPDATE_SKILL_RANKS               = 1 << 15,
    MSG_PLAYER_UPDATE_PORTRAIT                  = 1 << 16,
    MSG_PLAYER_UPDATE_FORCE_POINTS              = 1 << 17,
};

// The status word behind MSG_PLAYER_UPDATE_STATUS. The low seven bits are the associate state's.
enum NWMessagePlayerStatus {
    MSG_PLAYER_STATUS_DETECT                    = 1 << 0,
    MSG_PLAYER_STATUS_STEALTH                   = 1 << 1,
    MSG_PLAYER_STATUS_PARRY                     = 1 << 2,
    MSG_PLAYER_STATUS_POWER_ATTACK              = 1 << 3,
    MSG_PLAYER_STATUS_IMPROVED_POWER_ATTACK     = 1 << 4,
    MSG_PLAYER_STATUS_FLURRY                    = 1 << 5,
    MSG_PLAYER_STATUS_RAPID_SHOT                = 1 << 6,
    MSG_PLAYER_STATUS_MOVEMENT_SPEED_LIMITED    = 1 << 7,
    MSG_PLAYER_STATUS_SPECIAL_WALK_ANIMATION    = 1 << 8,
    MSG_PLAYER_STATUS_COUNTERSPELL              = 1 << 9,
    MSG_PLAYER_STATUS_DISGUISED                 = 1 << 10,
    MSG_PLAYER_STATUS_COMMANDABLE               = 1 << 11,
    MSG_PLAYER_STATUS_EXCITED                   = 1 << 12,
};

// Every player info panel, sent when the player's panels are first seen.
enum NWMessagePlayerUpdateAll : unsigned int {
    MSG_PLAYER_UPDATE_ALL = ~0u,
};

// A portrait id at or above MSG_PORTRAIT_ID_CUSTOM is followed on the wire by the portrait's resref.
enum NWMessagePortraitId {
    MSG_PORTRAIT_ID_CUSTOM = 65534,
};

// The field mask of a character sheet record ('G' 'S'): which panels of the sheet follow.
enum NWMessageCharacterSheetField {
    MSG_CHARACTER_SHEET_ABILITIES           = 1 << 0,
    MSG_CHARACTER_SHEET_FORTITUDE_SAVE      = 1 << 1,
    MSG_CHARACTER_SHEET_WILL_SAVE           = 1 << 2,
    MSG_CHARACTER_SHEET_REFLEX_SAVE         = 1 << 3,
    MSG_CHARACTER_SHEET_EXPERIENCE          = 1 << 4,
    MSG_CHARACTER_SHEET_BASE_ATTACK_BONUS   = 1 << 5,
    MSG_CHARACTER_SHEET_COMBAT_INFORMATION  = 1 << 6,
    MSG_CHARACTER_SHEET_SKILLS              = 1 << 7,
    MSG_CHARACTER_SHEET_FEATS               = 1 << 9,
    MSG_CHARACTER_SHEET_ARMOR_CLASS         = 1 << 10,
};

// The field mask of a module time update (MSG_MODULE_TIME): which clock fields follow.
enum NWMessageModuleTimeField {
    MSG_MODULE_TIME_TIME_OF_DAY  = 1 << 0,
    MSG_MODULE_TIME_HOUR         = 1 << 1,
    MSG_MODULE_TIME_DAY          = 1 << 2,
    MSG_MODULE_TIME_MONTH        = 1 << 3,
    MSG_MODULE_TIME_YEAR         = 1 << 4,
};

// The word that leads each quest of a journal update (MSG_JOURNAL_FULL_UPDATE): which of the quest's
// fields follow. ALL sends every field; REMOVED sends only the quest tag.
enum NWMessageJournalQuestField {
    MSG_JOURNAL_QUEST_NAME          = 1 << 0,
    MSG_JOURNAL_QUEST_TEXT          = 1 << 1,
    MSG_JOURNAL_QUEST_DATE          = 1 << 2,
    MSG_JOURNAL_QUEST_TIME          = 1 << 3,
    MSG_JOURNAL_QUEST_STATE         = 1 << 4,
    MSG_JOURNAL_QUEST_PRIORITY      = 1 << 5,
    MSG_JOURNAL_QUEST_PICTURE       = 1 << 6,
    MSG_JOURNAL_QUEST_END           = 1 << 7,
    MSG_JOURNAL_QUEST_OBSERVED      = 1 << 8,
    MSG_JOURNAL_QUEST_PLOT_INDEX    = 1 << 9,
    MSG_JOURNAL_QUEST_PLANET        = 1 << 10,
    MSG_JOURNAL_QUEST_EXPERIENCE    = 1 << 11,
    MSG_JOURNAL_QUEST_REMOVED       = 1 << 14,
    MSG_JOURNAL_QUEST_ALL           = 1 << 15,
};

// The skill-button word behind MSG_PLAYER_UPDATE_GUI_SKILLS: which skill buttons the client shows.
enum NWMessageGuiSkill {
    MSG_GUI_SKILL_DEMOLITIONS  = 1 << 1,
    MSG_GUI_SKILL_STEALTH      = 1 << 4,
    MSG_GUI_SKILL_SECURITY     = 1 << 5,
};

// Equipment slot bits, as inventory and appearance messages carry a slot: bit n is inventory slot n
// (nwscript's INVENTORY_SLOT_ n). Slots 2, 6 and 11-13 have no nwscript name and keep the names
// the engine inherited (boots, cloak, and three ammunition slots).
enum NWMessageEquipSlot {
    MSG_SLOT_HEAD               = 1 << 0,
    MSG_SLOT_BODY               = 1 << 1,
    MSG_SLOT_BOOTS              = 1 << 2,
    MSG_SLOT_HANDS              = 1 << 3,
    MSG_SLOT_RIGHT_WEAPON       = 1 << 4,
    MSG_SLOT_LEFT_WEAPON        = 1 << 5,
    MSG_SLOT_CLOAK              = 1 << 6,
    MSG_SLOT_LEFT_ARM           = 1 << 7,
    MSG_SLOT_RIGHT_ARM          = 1 << 8,
    MSG_SLOT_IMPLANT            = 1 << 9,
    MSG_SLOT_BELT               = 1 << 10,
    MSG_SLOT_ARROWS             = 1 << 11,
    MSG_SLOT_BULLETS            = 1 << 12,
    MSG_SLOT_BOLTS              = 1 << 13,
    MSG_SLOT_CREATURE_WEAPON_L  = 1 << 14,
    MSG_SLOT_CREATURE_WEAPON_R  = 1 << 15,
    MSG_SLOT_CREATURE_WEAPON_B  = 1 << 16,
    MSG_SLOT_CREATURE_ARMOR     = 1 << 17,
    MSG_SLOT_RIGHT_WEAPON_2     = 1 << 18,
    MSG_SLOT_LEFT_WEAPON_2      = 1 << 19,
};

// The type tag that leads each quickbar button of a MSG_GUI_QUICKBAR message.
enum NWMessageQuickbarButton {
    MSG_QUICKBAR_BUTTON_ITEM                = 1,
    MSG_QUICKBAR_BUTTON_SPELL               = 2,
    MSG_QUICKBAR_BUTTON_SPELL_LIKE_ABILITY  = 44,
};
