#ifndef MACMAIN_H
#define MACMAIN_H

// The Mac/iOS platform shim's input and path block.
//
// Mixed scope: the ASL namespace, the plain-C and `wgl*` names and the `bink_*` helpers are only
// declared here and stay undefined.

// Unsigned on purpose: callers subtract 3 from the result to get a table index.
namespace ASL { unsigned int GetLanguage(); }

// One finger, in the 1024x768 virtual touch space.
class TrackedTouch {
public:
    int m_nStartX;
    int m_nStartY;
    int m_nX;
    int m_nY;
    unsigned int m_nTime;
};

// Free functions the port layer calls, all of them defined in MacMain.cpp. Some callers keep their
// own local declarations instead of including this header.
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
const char* GetLocalizationFolderName();
const char* GetOverridePath(int nIndex, bool bFlag);
bool IosAnyTouches();
void IosTemporaryDisableInteraction(int nDisable);
int  IosGetTemporaryDisableInteraction();
int  IosGetRelativeTouch(float& fX, float& fY);
float GetIosControl(bool bUpDown, float fScale, float fDeadZone, bool bAnalog);
float GetIosControl(bool bUpDown, float fScale, float fDeadZone);
void ClearIosControl(bool bUpDown);
void RevealAndroidWindowIfNeeded();

// A running minigame's pause, which the on-screen pause button and losing focus raise (K2 defines
// it in MacMain.cpp, K1 in main.cpp).
extern bool Ios_minigamePaused;

#endif // MACMAIN_H
