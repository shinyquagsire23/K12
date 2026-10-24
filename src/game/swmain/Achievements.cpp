#include "k2stdint.h"
#include "Achievements.h"
#include <stdarg.h>

// bionic spells this strcasecmp.
#include "port/crtapi.h"
#include "strref.h"

// Formats the cloud tag the unlock call takes. The two unused ints are part of the signature the
// unlock call expects, and pBuf must hold 100 bytes.
static void FormatAchievementTag(char* pBuf, int nUnused1, int nUnused2, ...)
{
    K2_UNUSED(nUnused1);
    va_list ap;
    va_start(ap, nUnused2);
    __builtin___vsprintf_chk(pBuf, 0, 100, "achievement_%d", ap);
    va_end(ap);
}

// The five per-dialog latches. ResetOnLoad is the only writer in this unit.
bool Achievements::s_dontGetCockyFailed = false;
bool Achievements::s_dontGetCockyStarted = false;
bool Achievements::s_buildLightsaberDialog = false;
bool Achievements::s_itsATrapDialog = false;
bool Achievements::s_findersKeeperDialog = false;

void Achievements::ResetOnLoad()
{
    s_dontGetCockyFailed = false;
    s_dontGetCockyStarted = false;
    s_buildLightsaberDialog = false;
    s_itsATrapDialog = false;
    s_findersKeeperDialog = false;
}

void Achievements::OnDialogOptionChosen(int nOption)
{
    if (!s_findersKeeperDialog)
        return;
    s_findersKeeperDialog = false;
    if (nOption != 1)
        return;
    char szTag[100];
    FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_NIHILUS_MASK);
    UnlockAchievement(szTag);
}

bool Achievements::UnlockAchievement(AchievementId id)
{
    char szTag[100];
    FormatAchievementTag(szTag, 0, 0, id);
    return UnlockAchievement(szTag);
}

void Achievements::OnEndDialog()
{
    char szTag[100];
    if (s_buildLightsaberDialog) {
        s_buildLightsaberDialog = false;
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_FIRST_LIGHTSABER);
        UnlockAchievement(szTag);
    }
    if (s_itsATrapDialog) {
        s_itsATrapDialog = false;
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_TRAPPED_QUEST);
        UnlockAchievement(szTag);
    }
}

void Achievements::ResetAchievements()
{
}

// The per-strref dispatcher the talk table calls before it resolves anything. Every case value is a
// string ref; the body either sets one of the five "dialog already seen" latches, which later turn
// into an unlock, or formats and unlocks an achievement directly.
//
// Adjacent case labels share a body rather than being spelled as ranges. All the latch cases set
// their flag and return; the guarded case is the only one that tests a second latch.
void Achievements::EvaluateAchievement(uint64_t strRef)
{
    char szTag[100];

    switch (strRef) {
    case STRREF_CHAMP_AGREES_TO_LOSE:
    case STRREF_CHAMP_LOSES_THIS_TIME:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_CHAMP_THROWS_GAME);
        UnlockAchievement(szTag);
        break;
    case STRREF_EBON_HAWK_DOOR_SLICED:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_COMPLETE_PROLOGUE);
        UnlockAchievement(szTag);
        break;
    case STRREF_TEACH_ME_TO_BE_A_JEDI:
    case STRREF_TEACH_ME_TO_WIELD_THE_FORCE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_TEACH_HANDMAIDEN);
        UnlockAchievement(szTag);
        break;
    case STRREF_BRING_ME_THE_MASK:
        s_findersKeeperDialog = true;
        break;
    case STRREF_FIRST_STEPS_TO_DARK_SIDE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_FIRST_DARK_SIDE_POINTS);
        UnlockAchievement(szTag);
        break;
    case STRREF_HANDMAIDEN_JOINS_PARTY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_RECRUIT_HANDMAIDEN);
        UnlockAchievement(szTag);
        break;
    case STRREF_GUIDE_YOUR_DESTINY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_PRESTIGE_CLASS);
        UnlockAchievement(szTag);
        break;
    case STRREF_YOU_HAVE_WON_TALIA:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_ONDERON_SIDE_WITH_QUEEN);
        UnlockAchievement(szTag);
        break;
    case STRREF_HK47_ELECTRONIC_BUTCHERY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_HK47_UNIQUE_MODULE);
        UnlockAchievement(szTag);
        break;
    case STRREF_DANTOOINE_WILL_REBUILD:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_KHOONDA_SIDE_WITH_SETTLERS);
        UnlockAchievement(szTag);
        break;
    case STRREF_CRYSTAL_RESPONDS:
    case STRREF_CRYSTAL_OFFERED:
    case STRREF_CRYSTAL_RESPONDS_2:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_NAMED_LIGHTSABER_CRYSTAL);
        UnlockAchievement(szTag);
        break;
    case STRREF_CARRY_YOUR_OWN_WEIGHT:
    case STRREF_WELCOME_YOUR_COMPANY:
    case STRREF_HE_SEEMS_CAPABLE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_RECRUIT_DISCIPLE);
        UnlockAchievement(szTag);
        break;
    case STRREF_TAKE_THIS_GOLDEN_CARD:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_BEAT_PAZAAK_CHAMP);
        UnlockAchievement(szTag);
        break;
    case STRREF_SHE_IS_HERE:
    case STRREF_HER_APPRENTICE_IS_HERE:
        s_itsATrapDialog = true;
        break;
    case STRREF_GIVE_THE_TURRET_A_WORKOUT:
        s_dontGetCockyStarted = true;
        break;
    case STRREF_HANHARR_BROKEN:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_BREAK_HANHARR);
        UnlockAchievement(szTag);
        break;
    case STRREF_NO_MORE_UNKNOWNS:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_KILL_JEDI_MASTERS);
        UnlockAchievement(szTag);
        break;
    case STRREF_DISTURBED_THE_SPIRITS:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_LOOT_KORRIBAN_BODY);
        UnlockAchievement(szTag);
        break;
    case STRREF_KORRIBAN_RUNS_THROUGH_HIM:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_FLEE_KORRIBAN);
        UnlockAchievement(szTag);
        break;
    case STRREF_IMPOSSIBLE_ORDER:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_GOTO_IDENTITY);
        UnlockAchievement(szTag);
        break;
    case STRREF_MEN_WILL_LEAVE_YOU_BE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_KHOONDA_ABANDON_SETTLERS);
        UnlockAchievement(szTag);
        break;
    case STRREF_VISAS_WHAT_ARE_YOU_DOING:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DEFEAT_NIHILUS);
        UnlockAchievement(szTag);
        break;
    case STRREF_MOVING_MEDITATION_LEARNED:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_LEARN_MOVING_MEDITATION);
        UnlockAchievement(szTag);
        break;
    case STRREF_FORCE_SIGHT_LEARNED:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_LEARN_FORCE_SIGHT);
        UnlockAchievement(szTag);
        break;
    case STRREF_TRADE_WITH_TELOSIANS:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_VOGGA_FUEL);
        UnlockAchievement(szTag);
        break;
    case STRREF_MIRA_JOINS_PARTY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_RECRUIT_MIRA);
        UnlockAchievement(szTag);
        break;
    case STRREF_PERMIT_ME_TO_KILL_JEDI:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_HK47_KILL_JEDI);
        UnlockAchievement(szTag);
        break;
    case STRREF_BATTLE_CIRCLE_CHAMPION:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_MANDALORIAN_BATTLE_CIRCLE);
        UnlockAchievement(szTag);
        break;
    case STRREF_HER_SACRIFICE_WAS_NECESSARY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_ONDERON_SIDE_WITH_VAKLU);
        UnlockAchievement(szTag);
        break;
    case STRREF_DROID_POWER_RESTORED:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_REPAIR_EBON_HAWK_DROID);
        UnlockAchievement(szTag);
        break;
    case STRREF_WHAT_LIGHTSABER_TO_CREATE:
        s_buildLightsaberDialog = true;
        break;
    case STRREF_DEFINITION_OF_LOVE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_HK47_LOVE);
        UnlockAchievement(szTag);
        break;
    case STRREF_GLAD_TO_LEAVE_THIS_PLACE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DEFEAT_SION);
        UnlockAchievement(szTag);
        break;
    case STRREF_FIRST_STEPS_TO_LIGHT_SIDE:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_FIRST_LIGHT_SIDE_POINTS);
        UnlockAchievement(szTag);
        break;
    case STRREF_HANHARR_JOINS_PARTY:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_RECRUIT_HANHARR);
        UnlockAchievement(szTag);
        break;
    case STRREF_LETS_GET_OUT_OF_HERE:
        if (s_dontGetCockyStarted && !s_dontGetCockyFailed) {
            FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DONT_GET_COCKY);
            UnlockAchievement(szTag);
        }
#ifndef K2_FIX_ENGINE_BUGS
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DANCE_FOR_VOGGA);
        UnlockAchievement(szTag);
#endif
        break;
    case STRREF_WEVE_GOT_COMPANY:
        s_dontGetCockyFailed = true;
        break;
    case STRREF_CONVEY_HIS_COMPLIMENTS:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DANCE_FOR_VOGGA);
        UnlockAchievement(szTag);
        break;
    case STRREF_NO_MORE_MATCHES:
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_HANDMAIDEN_BATTLE_CIRCLE);
        UnlockAchievement(szTag);
        break;
    }
}

// Maps four movie names to a cloud achievement id and ignores anything else. It has the same shape
// as the dialog-end handler: format the tag into a 100-byte local, then hand it to the unlock call.
void Achievements::EvaluateAchievementMovie(const char* movie)
{
    char szTag[100];

    if (!_stricmp(movie, "malmov12"))
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_LIGHT_SIDE_ENDING);
    else if (!_stricmp(movie, "malmov14"))
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DARK_SIDE_ENDING);
    else if (!_stricmp(movie, "permov07"))
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_DESTROY_PERAGUS_ASTEROIDS);
    else if (!_stricmp(movie, "permov06"))
        FormatAchievementTag(szTag, 0, 0, ACHIEVEMENT_SITH_DESTROY_PERAGUS_ASTEROIDS);
    else
        return;

    UnlockAchievement(szTag);
}

#if KOTOR1
// K1 has no Achievements class: a free per-strref dispatcher hands an achievement name straight to
// the Java side (no "achievement_%d" ids, no dialog latches).
#include <jni.h>
extern "C" JNIEnv* Android_JNI_GetEnv();

void UnlockAchievement(const char* pszName);

void EvaluateAchievement(uint64_t nStrRef)
{
    switch (nStrRef) {
    case STRREF_AFFECTED_IN_WAYS_I_DID_NOT_ANTICIPATE:
        UnlockAchievement("a_mandalorian_affair");
        break;
    case STRREF_RECOVERED_LAST_DELETED_MEMORY: UnlockAchievement("memory_backup_completed"); break;
    case STRREF_CATHAR_WORM_LAST_LAUGH: UnlockAchievement("end_of_a_slaver"); break;
    case STRREF_JUHANI_MAY_ACCOMPANY_YOU: UnlockAchievement("betrayed_by_anger"); break;
    case STRREF_YOUR_TRAINING_IS_COMPLETE: UnlockAchievement("your_training_continues"); break;
    case STRREF_WHERE_THEIR_JOURNEY_BEGAN: UnlockAchievement("student_of_the_force"); break;
    case STRREF_BACK_ON_DANTOOINE:
    case STRREF_BACK_ON_DANTOOINE_2:
        UnlockAchievement("wrath_of_the_sith");
        break;
    case STRREF_ROUGH_LANDING:
    case STRREF_ROUGH_LANDING_2:
    case STRREF_ROUGH_LANDING_3:
        UnlockAchievement("crash_landing");
        break;
    case STRREF_BASTILA_SACRIFICED_HERSELF: UnlockAchievement("captured"); break;
    case STRREF_YOU_RULE_KASHYYYK:
    case STRREF_WE_HAVE_DONE_IT:
        UnlockAchievement("roooarrgh_ur_roo_");
        break;
    case STRREF_ANCIENT_ANSWERS: UnlockAchievement("back_on_the_wagon"); break;
    case STRREF_STAR_MAP_ACTIVATION: UnlockAchievement("lost_to_the_jungle"); break;
    case STRREF_YOU_KNEW_SELENE:
    case STRREF_SELENE_CONVINCED_ME:
        UnlockAchievement("revealing_the_obvious");
        break;
    case STRREF_RETURN_WITH_THE_LIGHTSABER: UnlockAchievement("in_wolfs_clothing"); break;
    case STRREF_I_HAVE_TAKEN_YOUR_PLACE:
        UnlockAchievement("thats_not_true_thats_impossible");
        break;
    case STRREF_THANK_YOU_FOR_SAVING_ME: UnlockAchievement("objection_your_honor"); break;
    case STRREF_DID_YOU_FIND_YOUR_STAR_MAP: UnlockAchievement("sea_of_intrigue"); break;
    case STRREF_I_CANNOT_BE_BEATEN: UnlockAchievement("the_chosen_one"); break;
    case STRREF_IM_CARTH: UnlockAchievement("rude_awakening"); break;
    case STRREF_ABANDONED_APARTMENT_BASE: UnlockAchievement("allies_of_convenience"); break;
    case STRREF_T3M4_IS_YOURS: case STRREF_TAKE_THE_DROID: UnlockAchievement("skeleton_key"); break;
    case STRREF_STEAL_THE_EBON_HAWK:
    case STRREF_STEAL_THE_EBON_HAWK_2:
        UnlockAchievement("a_bigger_gun");
        break;
    case STRREF_GAMORREAN_SEWER_CAMPS: UnlockAchievement("why_dont_you_put_her_in_charge"); break;
    case STRREF_LIFEDEBT: UnlockAchievement("the_wookies_with_me"); break;
    case STRREF_HK47_ENTERS_SERVICE: UnlockAchievement("lets_find_some_meatbags"); break;
    case STRREF_I_HAVE_THE_HOLOCRON: UnlockAchievement("treasure_hunters_legacy"); break;
    case STRREF_TARISIAN_ALE_MISTAKE: UnlockAchievement("still_in_trouble"); break;
    case STRREF_I_LOVE_YOU: UnlockAchievement("cant_wait"); break;
    case STRREF_JEDI_NOT_ALLOWED_TO_LOVE:
    case STRREF_JEDI_NOT_ALLOWED_TO_LOVE_2:
        UnlockAchievement("an_extra_bond");
        break;
    case STRREF_JEDI_ON_THE_STAR_FORGE: UnlockAchievement("thats_no_moon"); break;
    case STRREF_ALL_HAIL_DARTH_REVAN:
    case STRREF_ALL_HAIL_DARTH_REVAN_2:
        UnlockAchievement("sith_lord");
        break;
    case STRREF_SAVIORS_OF_THE_REPUBLIC:
    case STRREF_SAVIORS_OF_THE_REPUBLIC_2:
        UnlockAchievement("jedi_knight");
        break;
    case STRREF_T3M4_BOOP_BEEP:
    case STRREF_T3M4_BOOP_BEEP_2:
    case STRREF_T3M4_BOOP_BEEP_3:
    case STRREF_FOUL_SMELL_IN_THIS_PLACE:
    case STRREF_HK47_POWER_FROM_STAR_MAP:
    case STRREF_STAR_MAP_STILL_HAS_POWER:
    case STRREF_STAR_MAP_COLD_SHIVER:
    case STRREF_STAR_MAP_IN_DRAGON_LAIR:
    case STRREF_WE_GOT_WHAT_WE_CAME_FOR:
    case STRREF_MORE_THAN_A_COINCIDENCE:
    case STRREF_STAR_MAP_ENSLAVED_DRAGON:
    case STRREF_STAR_MAP_ENSLAVED_DRAGON_2:
    case STRREF_STAR_MAP_ENSLAVED_DRAGON_3:
        UnlockAchievement("beneath_the_sand");
        break;
    }
}

#ifdef K2_HOST_BUILD
extern "C" void k2host_unlock_achievement(const char* pszTag);   // platforms/shim/shim_achievements.cpp
#endif

void UnlockAchievement(const char* pszName)
{
#ifdef K2_HOST_BUILD
    // The host keeps its own record and notice, since there is no Java side to hand it to.
    k2host_unlock_achievement(pszName);
#else
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "unlockAchievement", "(Ljava/lang/String;)V");
    jstring strName = env->NewStringUTF(pszName);
    env->CallStaticVoidMethod(cls, mid, strName);
    env->DeleteLocalRef(strName);
    env->DeleteLocalRef(cls);
#endif
}
#endif
