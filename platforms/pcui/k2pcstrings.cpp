// k2pcstrings.cpp - the PC (keyboard-and-mouse) wording of the strings the Android release rewrote
// for touch (K2_UI_PC, KOTOR II only).
//
// Aspyr's talk table carries two override sets next to dialog.tlk, chosen per fetch by
// `IsJoystickConnected()`: set 1 is `dialog.tlk_control` (55 rewordings for a pad: "press the A
// button", "use the L1 and R1 buttons", ...); set 0 is `dialog.tlk_touch`, which the Android data
// does not ship, because the base dialog.tlk already is the touch wording. The pc build has no pad,
// so it fetches set 0: this file, in the override file's own format ("<strref>,<text>", read by
// CTlkTable::OpenFile), covering the strrefs dialog.tlk_control rewrites plus the tutorial pages
// (`tutorial.2da`'s Message_PC columns), the camera-tilt options and the pause caption. Each text is
// the English dialog.tlk of the macOS Steam build word for word, Xbox leftovers ("Press 'A' to
// continue") and "(Default)" key names included, as the PC player sees them. One change: 41896's
// `"A"` is spelled `'A'` (42027's own spelling), because OpenFile stops copying at a double quote.
//
// `<Pause>`, `<MGActionLeft>`, `<ActionUp>`... are `stringtokens.2da` key tokens (the names come from
// the DirectInput keyboard's DIPROP_KEYNAME); they are expanded only where ParseStr runs on the
// fetched text: the tutorial box, dialogue, and the pause panel's reason and caption.
//
// English only: the other four languages keep the touch text.

#if defined(K2_UI_PC) && !KOTOR1

#include <string.h>

const char* GetLocalizationFolderName();

static const char* const s_aPcStrings[] = {
    "41896,Unable to load party information.  Press 'A' to continue.",
    "42027,Saved game appears to be damaged and cannot be used.  Press 'A' to continue.",
    "48223,[Temp Help Text] To put on or remove a space suit, simply click on the space suit "
        "object.",
    "48699,ON: Using the Up movement key will rotate your view downwards.\n\nOFF: Using the Up "
        "movement key will rotate your view upwards.",
    "48729,You must spend all of your points. Click on the +/- arrows or use the arrow keys to "
        "assign points.",
    "48732,ENEMY SIGHTED!\nPress the Pause key (<Pause> or Pause) to continue",
    "48735,MINE SIGHTED!\nPress the Pause button to continue",
    "48759,Press 'Enter' to Cancel Live Feed and Return to Interface.",
    "49007,PRESS THE PAUSE BUTTON TO CONTINUE",
    "49008,Pressing the Pause button will pause the game. While paused, you can issue commands "
        "to your party. Press the Pause button again when you want to unpause the game.",
    "49009,Left-clicking the mouse activates a selected combat feat. A character's available "
        "combat feats appear in the first icon of the TARGET MENU.",
    "49010,When a grenade is highlighted in your TARGET MENU, left-clicking with the mouse will "
        "throw it at the selected target. Anyone caught in the blast area of a grenade will be "
        "affected, including allies.",
    "49011,Left-clicking the mouse while a mine is selected in your ACTION MENU will place a "
        "mine. You can also disable and recover enemy mines and add them to your own inventory for "
        "later use.",
    "49012,Left-click the mouse to activate a Friendly Force Power selected in the first icon of "
        "the ACTION MENU. Some Force Powers affect only the Jedi using them, others affect the "
        "entire party.",
    "49013,Left-click the mouse to activate an Hostile Force Power selected in the second icon "
        "of the TARGET MENU. Some Force Powers affect only a single target, others affect all "
        "enemies within a specified range of the target.",
    "49014,You can have multiple powers or items in each icon of the ACTION MENU. Scroll up or "
        "down using the mouse wheel to cycle through the various options.",
    "49015,The icons at the top right of the screen give you access to your character record "
        "sheet, inventory, map, journal and other menus. You can use the mouse to select different "
        "menus from the tabs at the top of the screen. Use the Exit button to exit the menus and "
        "return to normal gameplay.",
    "49016,You can use the mouse to select eligible targets. While in combat, only hostiles will "
        "be targetable.",
    "49017,Left-click the mouse to accelerate and shift gears. Use the Movement keys "
        "(<MGActionLeft> and <MGActionRight>) to avoid obstacles and hit accelerator pads.",
    "49018,Left-clicking on a slot on the EQUIP screen brings up a list of items you can equip "
        "in the selected inventory slot.",
    "49019,The game pauses when you sight an enemy, and the selection reticle will show up in "
        "red. You can use the mouse to select other hostile targets. This auto-pause option can be "
        "disabled on the Options screen.",
    "49021,Left-clicking the Default Action icon adds a standard attack to the stack or 'queue' "
        "of actions. You can also add other powers, feats, and abilities in this way, as well as "
        "items. You can add multiple actions and they will be performed in the order you added them "
        "to the stack.",
    "49022,Left-clicking the Default Action icon adds a standard attack to the stack or 'queue' "
        "of actions. You can also add other powers, feats, and abilities in this way, as well as "
        "items. You can add multiple actions and they will be performed in the order you added them "
        "to the stack.",
    "49023,Clearing actions can only be used in combat. Left-clicking on the action queue will "
        "clear an action. If you have multiple actions queued up, left-clicking will clear the one "
        "that was added to the queue most recently.",
    "49025,Whenever you spot an enemy, left-click the selected enemy once to enter combat mode. "
        "This will lock the camera on your selected target. Left-clicking the target again causes "
        "your character to perform the Default Action and attack each round.",
    "49026,Remember: Only left-click the targetted enemy ONCE to engage. You will automatically "
        "attack your target each round. You do not have to left-click it again until you select a "
        "new target.",
    "49028,You can also use the Movement Keys (<ActionUp>, <ActionDown>, <ActionLeft> and "
        "<ActionRight>) to move around the area.",
    "74999,[You used the keycard you found to open the locker. The Impact Armor from this locker "
        "can be equipped on the Equip Screen. To open the Equip screen, click on the Blaster icon in "
        "the top right area of the screen or press U (Default).]",
    "106490,ON: Moving the mouse upwards will tilt view upwards.\n\nOFF: Moving the mouse "
        "downwards will tilt view upwards.",
    "123746,[Take the programming spike inside this container by clicking on the Get Item "
        "button. Use this spike to access the terminal nearby.]",
    "124181,[3C-FD is now a member of your party. To change which party member you control, "
        "click on their portrait or press TAB (Default).  Later, when you have more companions, you "
        "can add and remove party members from the Party screen in the Game Menu.]\n",
    "124183,[Talking to other characters is much like using the computer console in the cockpit. "
        "Scroll through your response options or move the mouse cursor, and then click to select a "
        "response.]",
    "124446,Click on a party member's portrait to use this item.",
    "124449,Click this panel to cancel.",
    "124781,[Change your target by pressing Q or E, or left click on the object you want to "
        "target. To attack an enemy, left click on the Sword icon or press 1. To Queue up actions, "
        "left click multiple times. Clear your last combat action by clicking in the queue, or by "
        "pressing Y.]",
    "124783,[There are two weapon configurations in the Equip screen. This is useful to switch "
        "between ranged and melee weapons. To switch or unequip weapons, click the two swords icon "
        "in the lower right area of the screen, or press H (Default).]",
    "124786,[You can pause and unpause combat by clicking on the hourglass icon in the lower "
        "right area of the screen, or press Spacebar. Pausing combat will help you plan your moves "
        "strategically.]",
    "124789,[You found a grenade in the container. With grenades you can damage multiple enemies "
        "at once. To throw a grenade, click the grenade icon above your selected target, or press 3 "
        "(Default).]",
    "124793,{Equip weapon}[You can equip a weapon from the Equip screen. To open the Equip "
        "screen, click on the Blaster icon in the top right area of the screen or press U (Default).]",
    "124798,EBON HAWK COMMUNICATIONS CONSOLE\nDAMAGED - MINIMAL FUNCTIONALITY\nPRIMARY FUNCTIONS "
        "OFF-LINE\n\n(Click your left mouse button to continue.)",
    "124803,Access cockpit cameras. (Left click to select this option.)",
    "124804,[To access the area beyond this door, you will have to use Solo mode (Default: V). "
        "Click the Solo mode button at the bottom right area of the screen. While in Solo mode, move "
        "3C-FD to the security computer to open the door for you.]",
    "126827,[You found a droid flamethrower. This is one of the special weapons that droids can "
        "use. To equip it open the Equip screen, by clicking on the Blaster icon in the top right "
        "area of the screen or press U (Default).]",
    "126840,[You used the keycard you found to open the locker. The Impact Armor from this "
        "locker can be equipped on the Equip Screen. To open the Equip screen, click on the Blaster "
        "icon in the top right area of the screen or press U (Default).]",
    "126841,[You found a droid flamethrower. This is one of the special weapons that droids can "
        "use. To equip it open the Equip screen, by clicking on the Blaster icon in the top right "
        "area of the screen or press U (Default).]",
    "127843,[The lock on this metal box is too difficult to open. You can use one of the "
        "Security Tunnelers you found to crack this lock. Select the Security Tunneler by clicking "
        "on the arrows above or below the Security icon.]",
    "127991,You've repaired 3C-FD and invited him into your party. Switch between T3-M4 and "
        "3C-FD by clicking on their portrait or by pressing TAB (Default). You can switch between "
        "Solo mode and Follow mode by clicking on the Solo Mode button or by pressing V (Default).",
    "128196,[You can use Repair Kits to heal droids like yourself. To use a Repair Kit, click on "
        "the Repair Kit on bottom of the screen or press 5(Default).]",
    "128553,[Your security skill is too low to open this footlocker. However, you can use your "
        "weapon to Bash it open. Click on the Bash icon or press 1 (Default).]",
    "130029,Note: In order to use this console, you will need a computer spike. The number of "
        "computer spikes you have is listed in the bottom window of the screen.\n\nUse your mouse or "
        "the arrow keys to highlight one of the response options below. You can also press a number "
        "(1, 2, etc.) on your keyboard to select a response.",
    "130030,[You can now plot a course to the Peragus mining station. To dock with Peragus, left "
        "click on the Peragus planet to select it and then left click on the Travel button.]",
    "130036,[If you need help exploring the Ebon Hawk, access the Map screen by pressing M "
        "(Default).]",
    "130047,{Equip weapon}[T3-M4 must equip a blaster before attacking the sensor droids. To "
        "open the Equip screen, click on the Blaster icon in the top right area of the screen or "
        "press U (Default).]",
    "130507,Switch between party members by clicking on their portrait or by pressing TAB "
        "(Default). You can switch between Solo mode and Follow mode by clicking on the Solo Mode "
        "button or by pressing V (Default).",
    "130726,Press W, A, S, D (Default) to move T3-M4 and turn. Press Caps Lock (Default) to "
        "toggle first-person Free Look. Hold Ctrl (Default) to Look About.",
    "130735,This room contains a Plasteel Cylinder and the Communications Console. To target one "
        "of these objects, move close to it, left click on it, or press Q or E (Default) to cycle "
        "through available targets. To interact with an object, first target it and then left click "
        "on it.",
    "136311,PRESS <Pause> TO CONTINUE",
};

// Line nIndex of the set, or "" past the end (CTlkTable::OpenFile's loop stops on an empty line).
const char* k2pcstrings_line(int nIndex)
{
    const char* pszFolder = GetLocalizationFolderName();
    if (strstr(pszFolder, "rench") || strstr(pszFolder, "talian") || strstr(pszFolder, "erman") ||
        strstr(pszFolder, "panish"))
        return "";
    if (nIndex < 0 || nIndex >= static_cast<int>(sizeof(s_aPcStrings) / sizeof(s_aPcStrings[0])))
        return "";
    return s_aPcStrings[nIndex];
}

#endif
