// The desktop builds' search for the game's files and the player's (gamedirs.cpp).
#pragma once
#include <string>
#include <vector>

struct K2GameDirs {
    std::string assets;   // the game's files: the working directory
    std::string user;     // the player's (saves/, swkotor.ini ...): the shim's user directory when not assets
    const char* how;      // what decided: "K2_DATA_DIR", "an installed copy" ...
    std::vector<std::string> mods;   // KOTOR II: the Steam Workshop items of the install's library
    std::string saves;    // the install's Steam Cloud save folder, saves/ is redirected to; "" for none
    std::vector<std::string> obbs;   // the OBBs to mount, absolute, main first (KOTOR 1: main, patch); or none
};

// false, with the message for the player in `error`, when nothing named or found is the game.
bool k2_find_game_dirs(bool bKotor1, K2GameDirs& out, std::string& error);
