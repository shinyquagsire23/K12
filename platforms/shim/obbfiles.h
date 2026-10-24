// Which of a folder's files are the game's expansion files (OBBs). Every platform reads the folder
// from K2_OBB_DIR: Android's K2Activity sets it to getObbDir(), iOS defaults it to the app's
// Documents folder, the desktop builds take it from the environment (macos/gamedirs.cpp).
//
// Aspyr names them <kind>.<version>.<package>.obb: kind main or patch, package com.aspyr.swkotor
// (KOTOR 1) or com.aspyr.swkotorii (KOTOR II), or a package named after it (an Android install
// renames them for its own: com.aspyr.swkotorii.hybrid). Of each kind this takes the game's at the
// highest version, compared as a number (the kinds are versioned apart: KOTOR II ships main.213 with
// patch.14), names compared case-blind, so one folder can hold both games' files and an older
// download left beside a newer one is passed over.
#pragma once
#include <stdint.h>

#include <string>
#include <vector>

struct K2ObbNames {
    std::string main;
    std::string patch;
};

inline const char* k2_obb_package(bool bKotor1) { return bKotor1 ? "com.aspyr.swkotor" : "com.aspyr.swkotorii"; }

// The game's main and patch OBB among a folder's file names (names only, no directory). The package
// is the game's, or an app's derived from it (com.aspyr.swkotorii.hybrid: platforms/android installs
// the OBBs renamed for its own package); KOTOR II's com.aspyr.swkotorii is not KOTOR 1's.
inline K2ObbNames k2_pick_obbs(const std::vector<std::string>& names, bool bKotor1)
{
    const std::string package = k2_obb_package(bKotor1);
    K2ObbNames out;
    int64_t nBestMain = -1, nBestPatch = -1;
    for (const std::string& name : names) {
        std::string n = name;
        for (char& c : n)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        const bool bMain = n.compare(0, 5, "main.") == 0;
        if ((!bMain && n.compare(0, 6, "patch.") != 0) || n.size() < 4 || n.compare(n.size() - 4, 4, ".obb") != 0)
            continue;
        size_t at = bMain ? 5 : 6, digits = at;
        int64_t nVersion = 0;
        for (; digits < n.size() && digits - at < 18 && n[digits] >= '0' && n[digits] <= '9'; digits++)
            nVersion = nVersion * 10 + (n[digits] - '0');
        if (digits == at || digits + 1 > n.size() - 4 || n[digits] != '.') continue;
        const std::string pkg = n.substr(digits + 1, n.size() - 4 - (digits + 1));
        if (pkg != package && pkg.compare(0, package.size() + 1, package + ".") != 0) continue;
        int64_t& nBest = bMain ? nBestMain : nBestPatch;
        if (nVersion > nBest) {
            nBest = nVersion;
            (bMain ? out.main : out.patch) = name;
        }
    }
    return out;
}
