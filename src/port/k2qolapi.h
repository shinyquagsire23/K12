#ifndef K2QOLAPI_H
#define K2QOLAPI_H

// The quality-of-life layer's entry points (platforms/qol/), declared once for the engine TUs that
// call them under K2_QOL_CONTROLLER / K2_QOL_IMPROVEMENTS and for the files that define them.
// Neither macro is defined by default, so these are then declared and left unused.

class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiInGameAreaTransition;
class CSWGuiInGameSkillEntry;
class CSWGuiListBox;
class CSWGuiStatusSummary;
class CSWGuiText;
struct CSWGuiExtent;

// ---- the settings (qol/k2qol.cpp) ---------------------------------------------------------------
extern "C" void k2qol_startup(void);   // from the platform main: settings + the prompt art
extern "C" int  k2qol_icons(void);     // 0 automatic, 1 Xbox, 2 PlayStation, 3 Switch, 4 GameCube
extern "C" void k2qol_set_icons(int nStyle);
extern "C" int  k2qol_have_art(int nStyle);
extern "C" int  k2qol_no_pitch(void);
extern "C" void k2qol_set_no_pitch(int bOn);
extern "C" int  k2qol_dialog_text_scale(void);   // percent, 25..250 in steps of 25
extern "C" void k2qol_set_dialog_text_scale(int nPercent);
extern "C" int  k2qol_rumble(void);   // 0 off, 1 controller, 2 the device vibrator
extern "C" void k2qol_set_rumble(int nMode);
extern "C" int  k2qol_have_device_vibrator(void);
extern "C" void k2qol_test_rumble(void);
extern "C" void k2qol_set_rumble_motors(float fLow, float fHigh, unsigned int nMs);
extern "C" int  k2qol_swap_face_buttons(void);   // 0 standard, 1 A <-> B and X <-> Y
extern "C" void k2qol_set_swap_face_buttons(int bOn);
extern "C" int  k2qol_faster_movement(void);   // 1: the drive speed x1.3, as KOTOR 1 on mobile
extern "C" void k2qol_set_faster_movement(int bOn);
extern "C" int  k2qol_achievements(void);   // KOTOR 1: 0 = unlocks are neither recorded nor announced
extern "C" void k2qol_set_achievements(int bOn);
extern "C" const char* k2qol_texture_name(const char* pszName);   // the prompt textures renamed
extern "C" const char* k2qol_glyph_atlas(int nSize);              // KOTOR 1's prompt atlases

// For the dialog text size, strings built while a scope is open are made at the Text Size setting.
extern "C" void  k2qol_dialog_text_begin(void);
extern "C" void  k2qol_dialog_text_end(void);
extern "C" float k2qol_text_size_factor(void);   // NewCAurGUIString's multiplier

// One dialog-text scope, for a constructor or SetReplies.
struct K2QolDialogText {
    K2QolDialogText() { k2qol_dialog_text_begin(); }
    ~K2QolDialogText() { k2qol_dialog_text_end(); }
};

// ---- the layout at the Text Size (qol/k2qolpage.cpp) --------------------------------------------
// K2QolFitText: the string as large as the setting asks while its box can still draw it, never below
// 100 %; returns its height. A marked text is fitted on every change.
int  K2QolFitText(CSWGuiText& text, int nMaxHeight);
void K2QolMarkFitText(CSWGuiText* pText);
void K2QolUnmarkFitText(CSWGuiText* pText);
bool K2QolIsFitText(const CSWGuiText* pText);
void K2QolFitLabel(CSWGuiText* pText);
void K2QolMarkLayoutLabel(CSWGuiControl& control, const CExoString& sControlName);
void K2QolGrowListRow(CSWGuiListBox* pList, CSWGuiControl* pRow, int nRows);
int  K2QolListRowLimit(const CSWGuiListBox& listBox, int nSlack);
void K2QolNarrowSelectorCaption(CSWGuiButton& button, const CSWGuiControl& left, const CSWGuiControl& right);
void K2QolGrowItemCount(CSWGuiText& text, CSWGuiExtent& extent, float fGrow);
void K2QolGrowFloatyText(CSWGuiText& text, CSWGuiExtent& extent);
void K2QolResetStatusSummary(CSWGuiStatusSummary& summary);
void K2QolLayoutStatusSummary(CSWGuiStatusSummary& summary);
void K2QolLayoutAreaTransition(CSWGuiInGameAreaTransition& banner);
int  K2QolSkillFlowHeight(int nWidth, int nHeight);
// swguiskillflowchart.cpp: a skill info row sized to the list.
void K2QolSizeSkillInfoRow(CSWGuiInGameSkillEntry& row, const CSWGuiListBox& list);

#endif // K2QOLAPI_H
