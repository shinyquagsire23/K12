#include "k2stdint.h"
#include "dmconsole.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "nwchandlemessage.h"
#include "nwsrules.h"
#include "nwrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwccreature.h"
#include "nwcobject.h"
#include "nwcarea.h"
#include "nwcmodule.h"
#include "guiingame.h"
#include "nwsobject.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "clientoptions.h"
#include "exoiniinternal.h"
#include "exostring.h"
#include "exoresman.h"
#include "exosound.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "netlayer.h"
#include "Vector.h"
#include "math3d.h"
#include "PortDevMode.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "nwmessagetypes.h"

extern CExoSound* g_pExoSound;
extern CTlkTable* g_pTlkTable;
extern CExoString g_returnCode;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

#include "port/crtapi.h"
#include "exostringinline.h"

// The appearance.2da rows (KOTOR 2's labels) whose models the animation census skips.
enum {
    APPEARANCE_P_FEM_A_SML_05 = 103,
    APPEARANCE_P_FEM_B_MED_04 = 116,
};

// The animations.2da rows the census walks: the first 88 rows, and rows 200 up to (not including)
// 271. A creature without the full humanoid set skips rows 20 and 22 (sitdown, standup), a test the
// second range can never meet.
enum {
    CENSUS_FIRST_ROWS_END    = 88,
    CENSUS_SECOND_ROWS_BEGIN = 200,
    CENSUS_SECOND_ROWS_END   = 271,
};

// The Win32 DEVMODEA size EnumDisplaySettingsA is handed (KOTOR 1 stamps the older, shorter
// structure's), and the refresh rate above which KOTOR 1 needs the high-frequency option.
enum {
#if KOTOR1
    DEVMODE_SIZE = 124,
#else
    DEVMODE_SIZE = 156,
#endif
    MAX_STANDARD_REFRESH_HZ = 85,
};

// CGuiInGame::AppendToMsgBuffer's message type for a debug line.
enum {
    kMessageTypeDebug = 0x10000000,
};


static char szResult[256];

static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

// `spawnitem <resref> [<x> <y> <z>]`: the first space terminates the template resref and the rest
// is an optional offset from the player, added to the player's position after the direction part is
// normalized.
char* spawnitem(char* sArgs) {
    CSWCCreature* pCreature;
    CSWCArea* pArea;
    CSWCMessage* pMessage;
    if ((pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature()) == nullptr
        || (pArea = pCreature->m_pArea) == nullptr
        || (pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage()) == nullptr) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
        return szResult;
    }

    char szTemplate[256];
    char* pSpace = strchr(sArgs, ' ');
    if (pSpace) {
        *pSpace = '\0';
        __strcpy_chk(szTemplate, sArgs, sizeof(szTemplate));
        sArgs = pSpace + 1;
    } else {
        __strcpy_chk(szTemplate, sArgs, sizeof(szTemplate));
    }

    Vector vStart = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_position;
    Vector vOrientation = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_orientation;
    Vector vPos = vStart + normalize(vOrientation);

    char* pSpace2 = strchr(sArgs, ' ');
    if (pSpace2) {
        *pSpace2++ = '\0';
        vPos.x = static_cast<float>(atof(sArgs));
        char* pSpace3 = strchr(pSpace2, ' ');
        if (pSpace3) {
            *pSpace3++ = '\0';
            vPos.y = static_cast<float>(atof(pSpace2));
            if (strlen(pSpace3)) {
                vPos.z = static_cast<float>(atof(pSpace3));
            } else {
                vPos.z = 0.2f;
            }
        }
    }

    pMessage->SendPlayerToServerItem_Add(pArea->m_objectId, vPos, CResRef(szTemplate), 0, 0, 0);
    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// `spawnweapon <resref> [<a> <b> <c>]`: the same body as spawnitem, except the three trailing
// numbers are parsed with atoi rather than atof and become the item's three byte counts.
char* spawnweapon(char* sArgs) {
    CSWCCreature* pCreature;
    CSWCArea* pArea;
    CSWCMessage* pMessage;
    if ((pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature()) == nullptr
        || (pArea = pCreature->m_pArea) == nullptr
        || (pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage()) == nullptr) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
        return szResult;
    }

    char szTemplate[256];
    char* pSpace = strchr(sArgs, ' ');
    if (pSpace) {
        *pSpace = '\0';
        __strcpy_chk(szTemplate, sArgs, sizeof(szTemplate));
        sArgs = pSpace + 1;
    } else {
        __strcpy_chk(szTemplate, sArgs, sizeof(szTemplate));
    }

    Vector vStart = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_position;
    Vector vOrientation = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_orientation;
    Vector vPos = vStart + normalize(vOrientation);

    int nA = 0;
    int nB = 0;
    int nC = 0;
    char* pSpace2 = strchr(sArgs, ' ');
    if (pSpace2) {
        *pSpace2++ = '\0';
        nA = atoi(sArgs);
        char* pSpace3 = strchr(pSpace2, ' ');
        if (pSpace3) {
            *pSpace3++ = '\0';
            nB = atoi(pSpace2);
            nC = atoi(pSpace3);
        }
    }

    pMessage->SendPlayerToServerItem_Add(pArea->m_objectId, vPos, CResRef(szTemplate),
                                         static_cast<unsigned char>(nA), static_cast<unsigned char>(nB), static_cast<unsigned char>(nC));
    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// `visualeffect <id> <duration>` plays the visualeffects.2da row on the current target. The Type_FD
// column classifies the row: a row that is not "F" forces the duration to 0, and an "F" or "D" row
// with a non-positive duration defaults it to 1.
char* visualeffect(char* sArgs) {
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (!pInGameGui || !pMessage) {
#if KOTOR1
        return const_cast<char*>("Failure: NULL pointers");
#else
        return const_cast<char*>("Failure: nullptr pointers");
#endif
    }

    unsigned long long objIdSelf = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
    unsigned long long objIdTarget = g_pAppManager->m_pClientExoApp->GetLastTarget();
    if (objIdSelf == OBJECT_INVALID || objIdTarget == OBJECT_INVALID) {
        return const_cast<char*>("Failure: Missing target or source");
    }

    float fDuration = 0.0f;
    int nEffect = 0;
    static_cast<void>(sscanf(sArgs, "%i %f", &nEffect, &fDuration));

    CExoString sType;
    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(CExoString(nEffect),
                                                              CExoString("Type_FD"), &sType);
    if (sType.CompareNoCase("F") != 0) {
        fDuration = 0.0f;
    } else if (sType.CompareNoCase("D") != 0) {
        if (fDuration <= 0.0f) {
            fDuration = 1.0f;
        }
    }

    pMessage->SendPlayerToServerCheat_PlayVisualEffect(objIdTarget, static_cast<unsigned short>(nEffect),
                                                       fDuration, objIdSelf, 0);
    return const_cast<char*>("Success");
}

char* playerpathfindrule(char* sArgs) {
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    char* pszResult;
    if (pMessage) {
        pMessage->SendPlayerToServerCheat_PlayerPathfindRule(static_cast<unsigned char>(atoi(sArgs)));
        pszResult = const_cast<char*>("Success");
    } else {
        pszResult = const_cast<char*>("Failure");
    }
    sprintf_s(szResult, sizeof(szResult), pszResult);
    return szResult;
}

// Plays the HEAL_PLAYER one-shot and posts the raw DM message with sub-code 2 straight to the
// server through the net layer.
char* dm_heal(char* sArgs) {
    K2_UNUSED(sArgs);
    if (!g_pAppManager->m_pClientExoApp->GetSWCMessage()) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
        return szResult;
    }

    g_pExoSound->PlayOneShotSound(CResRef("HEAL_PLAYER"), 21, 0, 0, 0.0f, 0.0f);

    unsigned char* pData = new unsigned char[3];
    pData[0] = MSG_PLAYER_TO_SERVER;
    pData[1] = MSG_MAJOR_CHEAT;
    pData[2] = MSG_CHEAT_RESURRECT;
    g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(CNetLayer::PLAYERID_SERVER,
                                                                      pData, 3, 0);
    delete[] pData;

    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

char* makesafe(char* sArgs) {
    K2_UNUSED(sArgs);
    if (!g_pAppManager->m_pClientExoApp->GetSWCMessage()) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
        return szResult;
    }

    unsigned char* pData = new unsigned char[3];
    pData[0] = MSG_PLAYER_TO_SERVER;
    pData[1] = MSG_MAJOR_CHEAT;
    pData[2] = MSG_CHEAT_COMPUTE_SAFE_LOCATION;
    g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(CNetLayer::PLAYERID_SERVER,
                                                                      pData, 3, 0);
    delete[] pData;

    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// The message object is fetched and discarded, so the failure branch never runs.
char* NetProfile(char* sArgs) {
    K2_UNUSED(sArgs);
    if (!g_pAppManager->m_pClientExoApp->GetSWCMessage()) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
    }
    return szResult;
}

// The message object is fetched for its side effects and its value dropped; the body posts through
// the net layer instead.
char* showeffects(char* sArgs) {
    K2_UNUSED(sArgs);
    g_pAppManager->m_pClientExoApp->GetSWCMessage();

    unsigned char* pData = new unsigned char[3];
    pData[0] = MSG_PLAYER_TO_SERVER;
    pData[1] = MSG_MAJOR_CHEAT;
    pData[2] = MSG_CHEAT_TOGGLE_FLOATY_EFFECTS;
    g_pAppManager->m_pClientExoApp->GetNetLayer()->SendMessageToPlayer(CNetLayer::PLAYERID_SERVER,
                                                                      pData, 3, 0);
    delete[] pData;

    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// `spawnnasty [<type> <resref>]`: the type defaults to 6 and the creature to "dagger".
char* spawnnasty(char* sArgs) {
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWCArea* pArea = pCreature ? pCreature->m_pArea : 0;
    if (!pCreature || !pMessage || !pArea) {
        sprintf_s(szResult, sizeof(szResult), "Failure");
        return szResult;
    }

    char szName[256];
    strcpy(szName, "dagger");
    int nType = 6;
    char* pSpace = strchr(sArgs, ' ');
    if (pSpace) {
        *pSpace++ = '\0';
        nType = atoi(sArgs);
        char* pSpace2 = strchr(pSpace, ' ');
        if (pSpace2) {
            *pSpace2 = '\0';
        }
        __strcpy_chk(szName, pSpace, sizeof(szName));
    }

    pMessage->SendPlayerToServerCheat_SummonNasty(pArea->m_objectId, static_cast<unsigned char>(nType),
                                                  CResRef(szName));
    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// `dm_setrace <race>` accepts the seven playable race names, case-insensitively, or a raw numeric
// row, validates the row against appearance.2da's label column and then asks the server to switch
// appearance. The label text is what gets validated; the entry lookup's return value is ignored.
char* dm_setrace(char* sArgs) {
    unsigned short nRace;
    if (_stricmp(sArgs, "human") == 0) {
        nRace = 6;
    } else if (_stricmp(sArgs, "halforc") == 0) {
        nRace = 5;
    } else if (_stricmp(sArgs, "elf") == 0) {
        nRace = 1;
    } else if (_stricmp(sArgs, "halfling") == 0) {
        nRace = 3;
    } else if (_stricmp(sArgs, "dwarf") == 0) {
        nRace = 0;
    } else if (_stricmp(sArgs, "gnome") == 0) {
        nRace = 2;
    } else if (_stricmp(sArgs, "halfelf") == 0) {
        nRace = 4;
    } else {
        nRace = static_cast<unsigned short>(atoi(sArgs));
    }

    CExoString sLabel;
    char* pszResult = const_cast<char*>("Failure");
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(nRace, CExoString("LABEL"), &sLabel);
    if (sLabel.GetLength() >= 1) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        pszResult = g_pAppManager->m_pClientExoApp->GetSWCMessage()
                        ->SendPlayerToServerUpdate_AppearanceRace(pCreature->m_objectId, nRace)
                        ? const_cast<char*>("Success")
                        : const_cast<char*>("Failure");
    }
    return pszResult;
}

// Walks every appearance.2da row, applies it to the party's player creature and forces its
// animations to rebuild. Returns "Done!" unconditionally.
char* verifyallcreatures(char* sArgs) {
    K2_UNUSED(sArgs);
    CExoString sLabel;
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (!pCreature) {
        return const_cast<char*>("Done!");
    }

    int nRows = g_pRules->m_p2DArrays->m_pAppearance->m_nRows;
    if (nRows < 1) {
        return const_cast<char*>("Done!");
    }

    for (int i = 0; i < nRows; i++) {
        int nFound = g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            i, CExoString("Label"), &sLabel);
        if (nFound != 1) {
            continue;
        }

        Gob* pGob;
        CCAppearanceInfo* pInfo = new CCAppearanceInfo;
        pInfo->core.m_nAppearanceId = static_cast<unsigned short>(i);
        pInfo->core.m_nPhenotype = 0;
        pInfo->core.m_nHeadNumber = 0;
        pInfo->core.m_nGender = 0;
        pCreature->SetAppearance(pInfo, 1, 1, 0);

        if (i != APPEARANCE_P_FEM_A_SML_05 && i != APPEARANCE_P_FEM_B_MED_04) {
            pGob = pCreature->GetAnimationBase()->GetModel(CSWCObject::ANIM_MODEL_BODY);
            if (pGob) {
                pGob->GetPart(const_cast<char*>("impact"));   // result unused
            }
        }

        for (int nAnim = CENSUS_SECOND_ROWS_BEGIN; nAnim != CENSUS_SECOND_ROWS_END; nAnim++) {
            if (pCreature->GetAnimationBase()->m_creatureType != 1) {
                if (pCreature->GetAnimationBase()->m_creatureType == 1) {
                    continue;
                }
                if ((nAnim | 2) == CSWCAnimBase::CLIENT_ANIMATION_STAND_UP) {
                    continue;
                }
            }
            if (pCreature->GetAnimationBase()
                    ->AnimationExists(static_cast<unsigned short>(nAnim))) {
                continue;
            }
            CExoString sName = pCreature->GetAnimationBase()
                                   ->GetAnimationName(static_cast<unsigned short>(nAnim));
            sName.CStr();
        }

        for (int nAnim = 0; nAnim != CENSUS_FIRST_ROWS_END; nAnim++) {
            if (pCreature->GetAnimationBase()->m_creatureType != 1) {
                if (pCreature->GetAnimationBase()->m_creatureType == 1) {
                    continue;
                }
            }
            if (pCreature->GetAnimationBase()
                    ->AnimationExists(static_cast<unsigned short>(nAnim))) {
                continue;
            }
            CExoString sName = pCreature->GetAnimationBase()
                                   ->GetAnimationName(static_cast<unsigned short>(nAnim));
            sName.CStr();
        }

        delete pInfo;
    }

    return const_cast<char*>("Done!");
}

char* setphenotype(char* sArgs) {
    unsigned char nPhenotype;
    if (_stricmp(sArgs, "normal") == 0) {
        nPhenotype = 0;
    } else if (_stricmp(sArgs, "skinny") == 0) {
        nPhenotype = 1;
    } else if (_stricmp(sArgs, "fat") == 0) {
        nPhenotype = 2;
    } else {
        return const_cast<char*>("Failure");
    }

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    return g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerUpdate_AppearancePhenoType(
               pCreature->m_objectId, nPhenotype)
               ? const_cast<char*>("Success")
               : const_cast<char*>("Failure");
}

char* setgender(char* sArgs) {
    unsigned char nGender;
    if (_stricmp(sArgs, "male") == 0) {
        nGender = 0;
    } else if (_stricmp(sArgs, "female") == 0) {
        nGender = 1;
    } else {
        return const_cast<char*>("Failure");
    }

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    return g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerUpdate_AppearanceGender(
               pCreature->m_objectId, nGender)
               ? const_cast<char*>("Success")
               : const_cast<char*>("Failure");
}

// `setpltcolor <layer> <color>`: the layer is one of skin, hair, tattoo1 or tattoo2 (0/1/8/9).
char* setpltcolor(char* sArgs) {
    char* pSpace = strchr(sArgs, ' ');
    if (!pSpace) {
        return const_cast<char*>("Failure");
    }
    *pSpace++ = '\0';
    int nColor = atoi(pSpace);

    int nLayer;
    if (_stricmp(sArgs, "skin") == 0) {
        nLayer = 0;
    } else if (_stricmp(sArgs, "hair") == 0) {
        nLayer = 1;
    } else if (_stricmp(sArgs, "tattoo1") == 0) {
        nLayer = 8;
    } else if (_stricmp(sArgs, "tattoo2") == 0) {
        nLayer = 9;
    } else {
        return const_cast<char*>("Failure");
    }

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    return g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerUpdate_AppearancePLTColor(
               pCreature->m_objectId, static_cast<unsigned char>(nLayer), static_cast<unsigned char>(nColor))
               ? const_cast<char*>("Success")
               : const_cast<char*>("Failure");
}

char* setheadvariation(char* sArgs) {
    unsigned char nVariation = static_cast<unsigned char>(atoi(sArgs));
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    return g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerUpdate_AppearanceHeadVariation(
               pCreature->m_objectId, nVariation)
               ? const_cast<char*>("Success")
               : const_cast<char*>("Failure");
}

char* setalwaysrun(char* sArgs) {
    int nValue = atoi(sArgs);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAlwaysRun(nValue);
    return const_cast<char*>("Success");
}

// `playanim <name> [<speed> <flag>]` resolves the animation name against the creature's animation
// base, case-insensitively over the first 300 slots, and either queues it with a speed or plays it
// immediately. With a zero speed the looping flag decides between the two, otherwise the animation
// is queued.
char* playanim(char* sArgs) {
    char szName[256];
    szName[0] = '\0';
    float fSpeed = 0.0f;
    int nFlag = 1;
    static_cast<void>(sscanf(sArgs, "%s %f %i", szName, &fSpeed, &nFlag));

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (!pCreature || szName[0] == '\0') {
        return const_cast<char*>("Failure");
    }

    _strlwr(szName);

    unsigned short nAnim;
    for (nAnim = 0; nAnim < 300; nAnim++) {
        if (pCreature->GetAnimationBase()->GetAnimationName(nAnim).LowerCase() ==
            szName) {
            break;
        }
    }
    if (nAnim == 300) {
        return const_cast<char*>("Failure");
    }

    if (fSpeed == 0.0f) {
        if (pCreature->AnimationLooping(nAnim)) {
            if (!pCreature->SetAnimation(nAnim, 0, 0)) {
                return const_cast<char*>("Failure");
            }
        } else {
            pCreature->AddFireForgetAnimation(nAnim, 0);
        }
    } else {
        g_pAppManager->m_pClientExoApp->m_pInternal->AddQueuedAnim(CExoString(""), nAnim, fSpeed,
                                                                   nFlag);
    }
    return const_cast<char*>("Oh boy...");
}

// Prints the module's time-of-day as text.
char* dm_gettime(char* sArgs) {
    K2_UNUSED(sArgs);
    if (!g_pAppManager->m_pClientExoApp->GetModule()) {
        return const_cast<char*>("Failure");
    }

    char szBuffer[128];
    const char* pszTime;
    switch (g_pAppManager->m_pClientExoApp->GetModule()->m_nTimeOfDayState) {
    case CSWCModule::TIME_OF_DAY_DAY:
        pszTime = "TimeOfDayState: Day";
        break;
    case CSWCModule::TIME_OF_DAY_NIGHT:
        pszTime = "TimeOfDayState: Night";
        break;
    case CSWCModule::TIME_OF_DAY_DAWN:
        pszTime = "TimeOfDayState: Dawn";
        break;
    case CSWCModule::TIME_OF_DAY_DUSK:
        pszTime = "TimeOfDayState: Dusk";
        break;
    default:
        pszTime = "TimeOfDayState: Uh oh...";
        break;
    }
    sprintf_s(szBuffer, sizeof(szBuffer), pszTime);
    return const_cast<char*>("Success");
}

char* DebugMode(char* sArgs) {
    int bOn = atoi(sArgs);
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerCheat_DebugMode(bOn);
    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// `setvideomode [<w> <h> <bpp> <hz>]` walks the port's display-mode list for an exact match and asks
// the client to switch to it. Defaults are 800x600x32 at 60 Hz.
char* setvideomode(char* sArgs) {
    int nWidth = 800;
    int nHeight = 600;
    int nBpp = 32;
    int nRefresh = 60;
    static_cast<void>(sscanf(sArgs, "%i %i %i %i", &nWidth, &nHeight, &nBpp, &nRefresh));

    PortDevMode devMode = PortDevMode();
    devMode.m_size = DEVMODE_SIZE;

    unsigned int nMode = 0;
    while (EnumDisplaySettingsA(0, nMode, &devMode)) {
        if (devMode.m_pelsWidth == static_cast<unsigned int>(nWidth) &&
            devMode.m_pelsHeight == static_cast<unsigned int>(nHeight) &&
            devMode.m_bitsPerPel == static_cast<unsigned int>(nBpp) &&
#if KOTOR1  // K1: modes above 85 Hz only when the client allows high monitor frequencies
            devMode.m_displayFrequency == static_cast<unsigned int>(nRefresh) &&
            (g_pAppManager->m_pClientExoApp->GetAllowHighMonitorFrequency() != 0 ||
             devMode.m_displayFrequency <= MAX_STANDARD_REFRESH_HZ)) {
#else
            devMode.m_displayFrequency == static_cast<unsigned int>(nRefresh)) {
#endif
            g_pAppManager->m_pClientExoApp->SetVideoMode(nMode);
            return const_cast<char*>("Success");
        }
        ++nMode;
        __builtin_memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = DEVMODE_SIZE;
    }
    return const_cast<char*>("Failure");
}

// `displaystrref <strref> <strref> [<gender>]` fetches every strref in the range, in either order,
// and appends each to the in-game message buffer, then plays an empty one-shot.
char* displaystrref(char* sArgs) {
    unsigned int nFrom = 0;
    unsigned int nTo = 0;
    unsigned int nGender = ~0u;
    STR_RES strRes;
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    static_cast<void>(sscanf(sArgs, "%u %u %u", &nFrom, &nTo, &nGender));
    if (nGender == ~0u) {
        return const_cast<char*>("");
    }
    if (nTo < nFrom) {
        nTo = nFrom;
    }
    g_returnCode = sArgs;
    for (unsigned int i = nFrom; i <= nTo; i++) {
        g_pTlkTable->Fetch(i, strRes, static_cast<unsigned char>(nGender));
        pInGameGui->AppendToMsgBuffer(strRes.text, kMessageTypeDebug, 0);
    }
    g_pExoSound->PlayOneShotSound(strRes.soundResRef, 0, 0, 0, 0.0f, 0.0f);
    return g_returnCode.CStr();
}

// Shared body of the nine dice commands: RollDice with the die size fixed per command.
char* RollDice(char* sArgs, int nSides) {
    int nCount = atoi(sArgs);
    nCount = nCount > 1 ? nCount : 1;
    int nResult = g_pRules->RollDice(static_cast<unsigned char>(nCount), static_cast<unsigned char>(nSides));
    sprintf_s(szResult, sizeof(szResult), "Roll Dice %dd%d: %d", nCount, nSides, nResult);
    return szResult;
}

char* d2(char* sArgs) { return RollDice(sArgs, 2); }
char* d3(char* sArgs) { return RollDice(sArgs, 3); }
char* d4(char* sArgs) { return RollDice(sArgs, 4); }
char* d6(char* sArgs) { return RollDice(sArgs, 6); }
char* d8(char* sArgs) { return RollDice(sArgs, 8); }
char* d10(char* sArgs) { return RollDice(sArgs, 10); }
char* d12(char* sArgs) { return RollDice(sArgs, 12); }
char* d20(char* sArgs) { return RollDice(sArgs, 20); }
char* d100(char* sArgs) { return RollDice(sArgs, 100); }

// Spawns a party member near the player's position, nudged by a random offset of up to 2 units
// horizontally and a flat 0.2 vertically.
char* addpartymember(char* sArgs) {
    K2_UNUSED(sArgs);
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (!pCreature) {
#if KOTOR1
        sprintf_s(szResult, sizeof(szResult), "Failure - Player creture is NULL");
#else
        sprintf_s(szResult, sizeof(szResult), "Failure - Player creture is nullptr");
#endif
        return szResult;
    }
    CSWCArea* pArea = pCreature->m_pArea;
    if (!pArea) {
#if KOTOR1
        sprintf_s(szResult, sizeof(szResult), "Failure - Area is NULL");
#else
        sprintf_s(szResult, sizeof(szResult), "Failure - Area is nullptr");
#endif
        return szResult;
    }
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (!pMessage) {
#if KOTOR1
        sprintf_s(szResult, sizeof(szResult), "Failure - Message is NULL");
#else
        sprintf_s(szResult, sizeof(szResult), "Failure - Message is nullptr");
#endif
        return szResult;
    }

    Vector vPos = pCreature->m_position;
    float fOffsetX = (rand_wincompatible() % 100 < 50)
                         ? 1.0f + (rand_wincompatible() % 100) / 100.0f
                         : -1.0f - (rand_wincompatible() % 100) / 100.0f;
    float fOffsetY = (rand_wincompatible() % 100 < 50)
                         ? 1.0f + (rand_wincompatible() % 100) / 100.0f
                         : -1.0f - (rand_wincompatible() % 100) / 100.0f;
    vPos.x += fOffsetX;
    vPos.y += fOffsetY;
    vPos.z += 0.0f;
    vPos.z += 0.2f;

    pMessage->SendPlayerToServerParty_Add(pArea->m_objectId, vPos, CExoString("PARTYMEMBER"));
    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}

// Reads every `power<N>` key out of forcepowers.ini's [Force Powers] section and grants each spell id
// it names, in index order, up to 999 entries.
char* giveforcepowers(char* sArgs) {
    K2_UNUSED(sArgs);
    CExoIni ini;
    CExoString sValue;
    CExoString sKey;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pCreature =
        pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());

    int bAny = 0;
    for (int i = 1; i < 1000; i++) {
        sKey.Format(const_cast<char*>("power%d"), i);
#if KOTOR1
        if (ini.ReadIniEntry(sValue, CExoString("forcepowers.ini"),
#else
        if (ini.ReadIniEntry(sValue, CExoString(".\\forcepowers.ini"),
#endif
                             CExoString("Force Powers"), sKey) != 1) {
            if (!bAny) {
                sprintf_s(szResult, sizeof(szResult), "Failed: no entries");
                return szResult;
            }
            break;
        }
        int nSpell = sValue.AsINT();
        if (!pCreature->m_pStats->HasSpell(0, nSpell, 1)) {
            pCreature->AddSpellWithPrerequisites(nSpell);
        }
        bAny = 1;
    }

    sprintf_s(szResult, sizeof(szResult), "Success");
    return szResult;
}
