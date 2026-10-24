#pragma once
#include "k2stdint.h"

// The class-selection panel, with six preview tiles, four labels, BTN_BACK and the module to
// launch.

#ifndef CSWGUICLASSSELECTION_H
#define CSWGUICLASSSELECTION_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"

class CSWCCreature;
class CSWGuiControl;
class CSWGuiManager;

#if KOTOR1
// K1 declares the tile struct at namespace scope.
struct CSWGuiClassSelChar {
    CSWGuiButton m_button;
    CSWGui3DSceneView m_sceneView;
    CSWCCreature* m_pCreature;
    float m_fSlideTimer;
    int m_nSlideState;
};
K2_LAYOUT_ASSERT(sizeof(CSWGuiClassSelChar) == 0x330, "CSWGuiClassSelChar size drift");  // K1 stride
#endif

class CSWGuiClassSelection : public CSWGuiPanel {
public:
    // One class tile, holding the preview button, the live 3D model behind it and the creature
    // that model is built from.
#if KOTOR1
    typedef CSWGuiClassSelChar ClassOption;
#else
    struct ClassOption {
        CSWGuiButton m_button;
        CSWGui3DSceneView m_sceneView;
        CSWCCreature* m_pCreature;
        float m_fSlideTimer;
        int m_nSlideState;
    };
#endif

    // Loads ClassSel_p and builds the six preview creatures off the portraits 2DA.
    CSWGuiClassSelection(CSWGuiManager* pGuiManager, const CExoString& sModule);

    // User-declared. Deletes the six preview creatures.
    virtual ~CSWGuiClassSelection();

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    // Handlers bound to each tile's button.
    void OnSelectButton(CSWGuiControl* pControl);
    void OnEnterButton(CSWGuiControl* pControl);
    void HandleCancelButton();

    // Starts the game with the chosen class and module once char-gen finishes.
    void BeginPlayGame();

    char m_aReserved1[8];

    CSWCCreature* m_pSelectedCreature;

    ClassOption m_aOptions[6];

    CSWGuiLabel m_lblCharGen;
    CSWGuiLabel m_lblInstruction;
    CSWGuiLabel m_lblDesc;
    CSWGuiLabel m_lblClass;

    CSWGuiButton m_btnBack;
    CSWGuiButton m_btnAccept;

    CExoString m_sModuleName;

    // One row of CHAR_TYPES, holding class id, gender (1 == female), sound set and description
    // strref.
    struct ClassEntry {
        unsigned char m_nClass;
        unsigned char m_nGender;
        unsigned char m_nSoundSet;
        char _pad03[5];
        uint64_t m_nStrRef;
    };

    // Per-class table, indexed by class * 16.
    static ClassEntry CHAR_TYPES[6];
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiClassSelection::ClassOption) == 0x360, "ClassOption size drift");
#endif
K2_LAYOUT_ASSERT(sizeof(CSWGuiClassSelection::ClassEntry) == 0x10, "ClassEntry size drift");
#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiClassSelection) == 0x20b8, "CSWGuiClassSelection size drift");
#endif

#endif // CSWGUICLASSSELECTION_H
