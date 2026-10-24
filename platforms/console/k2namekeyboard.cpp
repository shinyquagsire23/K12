// k2namekeyboard.cpp - the console on-screen keyboard (K2_UI_XBOX, and K1_UI_XBOX), built on the
// Switch port's CSWGuiNameChargen panel.
//
// KOTOR II: `name_x` is an Xbox layout that still ships in the Android data: a 13 x 5 grid of key
// buttons whose captions are the characters they type (BTN_STD1..65, and an accented second layer
// BTN_INT1..65), SPACE_BTN / BACKSPACE_BTN / END_BTN, NAME_BOX_LBL and the A/B/X/Y legend.
//
// KOTOR 1: the console game had the same panel. Its `NAME` layout is the keyboard (CSWGuiNameChargen's
// constructor binds BTN_STD%d / BTN_INT%d / SPACE_BTN / BACKSPACE_BTN / END_BTN / NAME_BOX_LBL /
// LBL_A..Y off "NAME"), but the Android data ships only the PC `name` layout (gui.bif: the title pair,
// an edit box, END / RANDOM / BACK). Every piece of art and text the keyboard needs is in the APK
// (swpc_tex_gui.erf: lbl_cg_namebk, border1/2, border1a/2a; dialog.tlk: 1558 Space, 1560 Delete, 1562
// Done, 236 Select, 219 Back, 1559 Random, 42282 International, 42360 / 42361 the two refusals), so
// the K1 arm builds the console layout in code from the PC one: the panel and its title pair come from
// `name`, and every key is a copy of its END_BTN placed at the console layout's 640x480 rectangle
// (scaled the way that layout's own extents were).
//
// It serves two panels, each of which stays the model (it owns the editbox text and the OK / Cancel
// work): CSWGuiNameChargen (character generation) and CSWGuiSaveNamePanel (naming a save). Their
// OnPanelAdded calls k2namekeyboard_push_* under K2_UI_XBOX / K1_UI_XBOX, which puts the keyboard on
// the modal stack above them and hides them while it is up. The system keyboard is never raised in
// these builds (shim_asl.cpp's ASLPlat_ShowVirtualKeyboard).

#include <stdint.h>
#include "strref.h"
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)

#include "swgui.h"
#include "swguicontrol.h"
#include "swguiingame.h"
#include "swguinamechargen.h"
#include "swguisaveload.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "exostring.h"
#include "exoresman.h"

#include <string.h>

#if KOTOR1
void K1ScaleLoadedExtent(CSWGuiExtent& extent, CResGFF* pGFF);   // swgui.cpp (pc / xbox builds)
#endif

namespace {

enum Kind { KIND_CHARGEN, KIND_SAVENAME };

const int kColumns = 13;
const int kKeys    = 65;               // 13 x 5, per layer
const int kMaxName = 15;               // the Switch refuses a key once the name is longer than this


enum { MOVE_UP = 0, MOVE_LEFT = 1, MOVE_DOWN = 2, MOVE_RIGHT = 3 };

#if KOTOR1
// The console NAME layout's key captions (cp1252), row by row, from its BTN_STD1..65 / BTN_INT1..65
// TEXT.
const unsigned char kStdKeys[kKeys] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '@', 0xa2, 0xd0, 0x80, 0x83, 0xa3, 0xa4, 0xa7, 0xb5, '-', '.', '\'', '"',
};
const unsigned char kIntKeys[kKeys] = {
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc,
    0xcd, 0xce, 0xcf, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd8, 0x8c, 0x8a, 0xd9,
    0xda, 0xdb, 0xdc, 0xdd, 0x9f, 0x8e, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6,
    0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf8, 0x9c, 0x9a, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xff, 0x9e, 0xdf,
};

// The console layout's rectangles, in its 640x480 design space.
struct Rect { int x, y, w, h; };
const Rect kRectName      = {184, 104, 272, 34};   // NAME_BOX_LBL
const int  kKeyLeft = 99, kKeyTop = 164, kKeySize = 34;
const Rect kRectKeyBack   = { 99, 164, 442, 204};  // the 13 x 5 grid and the bar under it
const Rect kRectBackspace = { 99, 334, 136, 34};   // BACKSPACE_BTN
const Rect kRectSpace     = {235, 334, 170, 34};   // SPACE_BTN
const Rect kRectEnd       = {405, 334, 136, 34};   // END_BTN
// Each legend entry, as the console layout has it, is a pad-glyph label (the APK's ios_button_*
// art, as on every other K1 console panel) and its caption, left-aligned.
const Rect kRectGlyphY    = {124, 408,  32, 32};   // LBL_Y
const Rect kRectRandom    = {154, 411,  88, 25};   // LBL_RANDOM
const Rect kRectGlyphX    = {238, 376,  32, 32};   // LBL_X
const Rect kRectIntl      = {270, 375, 182, 32};   // LBL_INTL
const Rect kRectGlyphA    = {270, 408,  32, 32};   // LBL_A
const Rect kRectAccept    = {302, 411,  88, 25};   // LBL_ACCEPT
const Rect kRectGlyphB    = {412, 408,  32, 32};   // LBL_B
const Rect kRectBack      = {440, 411,  88, 25};   // BACK_LBL

#endif

}  // namespace

class K2GuiNameKeyboard : public CSWGuiPanel {
public:
    K2GuiNameKeyboard(CSWGuiManager* pGuiManager, CSWGuiPanel* pModel, Kind kind);
    virtual ~K2GuiNameKeyboard() {}

    virtual void HandleInputEvent(int type, int val);
    virtual void Draw(float f);

    void OnKey(CSWGuiControl* pControl);
    void OnDone(CSWGuiControl* pControl);
    void OnBack(CSWGuiControl* pControl);
    void OnRandom(CSWGuiControl* pControl);
    void OnInternational(CSWGuiControl* pControl);

    // The model's side of things.
    CSWGuiEditbox& Editbox();
    const char* NameText();
    bool IsRandomName();
    void ClearRandomName();
    bool HasRandomName() const { return m_kind == KIND_CHARGEN; }
    bool RequiresName() const { return m_kind == KIND_CHARGEN; }

    void SetInternational(int bInternational);
    void LinkGrid(CSWGuiButton* aKeys);
    void RefreshName();
    void ShowMessage(uint64_t nStrRef);
    void Close();
#if KOTOR1
    void BuildK1Layout();
    void PlaceK1(CSWGuiControl& control, const Rect& rect);
    void MakeK1Key(CSWGuiButton& key, const Rect& rect);
    void CenterK1Legend(CSWGuiLabel& glyph, CSWGuiButton& caption, int nCentre);
#endif

    CSWGuiPanel*  m_pModel;            // the panel underneath
    Kind          m_kind;
    CSWGuiButton  m_aStd[kKeys];       // BTN_STD1..65
    CSWGuiButton  m_aInt[kKeys];       // BTN_INT1..65
    CSWGuiButton  m_btnSpace;          // SPACE_BTN
    CSWGuiButton  m_btnBackspace;      // BACKSPACE_BTN
    CSWGuiButton  m_btnEnd;            // END_BTN
    // The legend captions are Labels in the layout; bound as buttons (InitControl binds by tag and
    // ignores CONTROLTYPE) so that a touch screen can reach Back / Random / International too.
    CSWGuiButton  m_btnAccept;         // LBL_ACCEPT
    CSWGuiButton  m_btnBack;           // BACK_LBL
    CSWGuiButton  m_btnRandom;         // LBL_RANDOM
    CSWGuiButton  m_btnIntl;           // LBL_INTL
    CSWGuiLabel   m_lblTitle;          // MAIN_TITLE_LBL
    CSWGuiLabel   m_lblSubTitle;       // SUB_TITLE_LBL
    CSWGuiLabel   m_lblName;           // NAME_BOX_LBL
    CSWGuiLabel   m_aLblPad[4];        // LBL_A / LBL_B / LBL_X / LBL_Y (the pad-button glyphs)
#if KOTOR1
    CSWGuiLabel   m_lblKeyBack;        // an opaque plate under the keys (K1's PC background art)
#endif
    CExoString    m_sShown;            // the name NAME_BOX_LBL currently shows
    int           m_bInternational;
    int           m_bDefocused;
};

static K2GuiNameKeyboard* g_pNameKeyboard;   // the one on screen, if any

#if KOTOR1
// A control placed at a console-layout rectangle, scaled the way that layout's own extents were as
// they loaded (K1ScaleLoadedExtent: 640x480 -> 1024x768). Must run inside the layout load.
void K2GuiNameKeyboard::PlaceK1(CSWGuiControl& control, const Rect& rect)
{
    CSWGuiExtent extent;
    extent.x = rect.x;
    extent.y = rect.y;
    extent.w = rect.w;
    extent.h = rect.h;
    K1ScaleLoadedExtent(extent, m_pLayoutRes);
    control.SetExtent(extent);
}

// A legend entry (its glyph, then its caption) centred on x = nCentre of the PC `name` layout's
// 640-wide design space, by the caption's laid-out width. The glyph is the size an inline pad glyph
// draws at beside a caption on K1's other console panels (1.9 times the font height), centred on the
// caption's line, a quarter of itself before the text. Runs after ScaleExtentForResolution.
void K2GuiNameKeyboard::CenterK1Legend(CSWGuiLabel& glyph, CSWGuiButton& caption, int nCentre)
{
    float fScale = static_cast<float>(m_extent.w) / 640.0f;
    int nTextWidth = caption.m_text.GetIdealWidthAndHeight().w;
    int nSize = static_cast<int>(static_cast<float>(caption.m_text.GetFontHeight()) * 1.9f);
    int nGap = nSize / 4;
    int nX = static_cast<int>(static_cast<float>(nCentre) * fScale) - (nSize + nGap + nTextWidth) / 2;
    CSWGuiExtent extent = glyph.m_extent;
    extent.x = nX;
    extent.y = caption.m_extent.y + (caption.m_extent.h - nSize) / 2;
    extent.w = nSize;
    extent.h = nSize;
    glyph.SetExtent(extent);
    extent = caption.m_extent;
    extent.x = nX + nSize + nGap;
    if (extent.w < nTextWidth)
        extent.w = nTextWidth;
    caption.SetExtent(extent);
}

// Sets up the console key frame on a border, border2 / border1 at 4 units (border2a / border1a when
// selected) over an opaque fill. The PC background's own name box sits right behind the grid.
static void SetK1KeyFrame(CSWGuiBorder& border, const char* pszCorner, const char* pszEdge)
{
    border.m_params.m_nBorderDim = 4;
    border.m_params.m_nInnerOffset = 0;   // the PC button's 9-unit hilight inset
    border.m_params.SetCornerImage(CResRef(pszCorner), 1);
    border.m_params.SetEdgeImage(CResRef(pszEdge), 1);
    border.m_params.SetFillImage(CResRef("blackfill"), 1);
}

// Makes one key, a copy of the PC layout's END_BTN (colours, sounds) with the console key's frame
// and font (fnt_d16x16, since the PC button font has no lower case) and no moves of its own.
void K2GuiNameKeyboard::MakeK1Key(CSWGuiButton& key, const Rect& rect)
{
    InitControl(key, CExoString("END_BTN"), 0);
    SetK1KeyFrame(key.m_border, "border2", "border1");
    SetK1KeyFrame(key.m_borderHilight, "border2a", "border1a");
    key.m_text.m_params.SetBaseFont(CResRef("fnt_d16x16"));
    for (int nDir = 0; nDir < 4; nDir++)
        key.m_moveToControls[nDir] = 0;
    PlaceK1(key, rect);
}

// The console NAME layout, built from the PC `name` layout (see above). Every control is bound
// outside the ID table (the END_BTN copies would all claim its slot) and added after the load, so
// ConnectControlMoves never sees them and the grid's moves set by the ctor are the only ones.
void K2GuiNameKeyboard::BuildK1Layout()
{
    StartLoadFromLayout(CResRef("name"));
    InitControl(m_lblTitle, CExoString("MAIN_TITLE_LBL"), 0);
    InitControl(m_lblSubTitle, CExoString("SUB_TITLE_LBL"), 0);
    InitControl(m_lblName, CExoString("NAME_BOX_EDIT"), 0);   // a Label in the PC layout as well
    // The PC background draws its own name box where the console grid goes, so a black plate hides
    // it.
    InitControl(m_lblKeyBack, CExoString("SUB_TITLE_LBL"), 0);
    m_lblKeyBack.m_text.m_params.SetText(CExoString(""));
    m_lblKeyBack.m_border.m_params.SetFillImage(CResRef("blackfill"), 1);
    PlaceK1(m_lblKeyBack, kRectKeyBack);

    for (int i = 0; i < kKeys; i++) {
        Rect rect = {kKeyLeft + (i % kColumns) * kKeySize, kKeyTop + (i / kColumns) * kKeySize,
                     kKeySize, kKeySize};
        MakeK1Key(m_aStd[i], rect);
        m_aStd[i].m_text.m_params.SetText(CExoString(reinterpret_cast<const char*>(&kStdKeys[i]), 1));
        MakeK1Key(m_aInt[i], rect);
        m_aInt[i].m_text.m_params.SetText(CExoString(reinterpret_cast<const char*>(&kIntKeys[i]), 1));
    }
    MakeK1Key(m_btnBackspace, kRectBackspace);
    m_btnBackspace.m_text.m_params.SetStrRef(STRREF_DELETE);
    MakeK1Key(m_btnSpace, kRectSpace);
    m_btnSpace.m_text.m_params.SetStrRef(STRREF_SPACE);
    MakeK1Key(m_btnEnd, kRectEnd);
    m_btnEnd.m_text.m_params.SetStrRef(STRREF_DONE);

    // The name field gets the key frame around the console NAME_BOX_LBL.
    SetK1KeyFrame(m_lblName.m_border, "border2", "border1");
    PlaceK1(m_lblName, kRectName);

    // In the legend, captions are END_BTN copies (no frame until selected, and nothing in the pad
    // chain selects them) and glyphs are SUB_TITLE_LBL copies filled with the pad art.
    struct Legend {
        CSWGuiButton* pButton; const Rect* pRect; uint64_t nStrRef;
        CSWGuiLabel* pGlyph; const Rect* pGlyphRect; const char* pszGlyph;
    };
    const Legend aLegend[] = {
        {&m_btnRandom, &kRectRandom, STRREF_RANDOM, &m_aLblPad[3], &kRectGlyphY, "ios_button_y"},
        {&m_btnIntl,   &kRectIntl,   STRREF_INTERNATIONAL,   &m_aLblPad[2], &kRectGlyphX, "ios_button_x"},
        {&m_btnAccept, &kRectAccept, STRREF_SELECT, &m_aLblPad[0], &kRectGlyphA, "ios_button_a"},
        {&m_btnBack,   &kRectBack,   STRREF_BACK,   &m_aLblPad[1], &kRectGlyphB, "ios_button_b"},
    };
    for (unsigned int i = 0; i < sizeof(aLegend) / sizeof(aLegend[0]); i++) {
        CSWGuiButton& button = *aLegend[i].pButton;
        InitControl(button, CExoString("END_BTN"), 0);
        for (int nDir = 0; nDir < 4; nDir++)
            button.m_moveToControls[nDir] = 0;
        PlaceK1(button, *aLegend[i].pRect);
        button.m_text.m_params.SetStrRef(aLegend[i].nStrRef);
        button.m_text.m_params.SetAlignment(17);   // left, centred vertically
        CSWGuiLabel& glyph = *aLegend[i].pGlyph;
        InitControl(glyph, CExoString("SUB_TITLE_LBL"), 0);
        glyph.m_text.m_params.SetText(CExoString(""));
        glyph.m_border.m_params.SetFillImage(CResRef(aLegend[i].pszGlyph), 1);
        PlaceK1(glyph, *aLegend[i].pGlyphRect);
    }
    StopLoadFromLayout();

    AddControl(&m_lblTitle);
    AddControl(&m_lblSubTitle);
    AddControl(&m_lblName);
    AddControl(&m_lblKeyBack);
    for (int i = 0; i < kKeys; i++) {
        AddControl(&m_aStd[i]);
        AddControl(&m_aInt[i]);
    }
    AddControl(&m_btnBackspace);
    AddControl(&m_btnSpace);
    AddControl(&m_btnEnd);
    AddControl(&m_btnRandom);
    AddControl(&m_btnIntl);
    AddControl(&m_btnAccept);
    AddControl(&m_btnBack);
    for (int i = 0; i < 4; i++)
        AddControl(&m_aLblPad[i]);
    // As CSWGuiNameChargen does, scale the 1024x768 layout to the screen's height; AddPanel's flag
    // 2 centres it.
    ScaleExtentForResolution();
}
#endif

K2GuiNameKeyboard::K2GuiNameKeyboard(CSWGuiManager* pGuiManager, CSWGuiPanel* pModel, Kind kind)
    : CSWGuiPanel(pGuiManager), m_pModel(pModel), m_kind(kind), m_bInternational(0),
      m_bDefocused(0)
{
    typedef void (CSWGuiObject::*Handler)(CSWGuiControl*);

#if KOTOR1
    BuildK1Layout();
#else
    StartLoadFromLayout(CResRef("name_x"), 1, 1, 1);
    for (int i = 0; i < kKeys; i++) {
        CExoString sTag;
        sTag.Format(const_cast<char*>("BTN_STD%d"), i + 1);
        InitControl(m_aStd[i], sTag, 1, 1, 0, 0, 0);
        sTag.Format(const_cast<char*>("BTN_INT%d"), i + 1);
        InitControl(m_aInt[i], sTag, 1, 1, 0, 0, 0);
    }
    InitControl(m_btnSpace,     CExoString("SPACE_BTN"),      1, 1, 0, 0, 0);
    InitControl(m_btnBackspace, CExoString("BACKSPACE_BTN"),  1, 1, 0, 0, 0);
    InitControl(m_btnEnd,       CExoString("END_BTN"),        1, 1, 0, 0, 0);
    InitControl(m_lblTitle,     CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle,  CExoString("SUB_TITLE_LBL"),  1, 1, 0, 0, 0);
    InitControl(m_lblName,      CExoString("NAME_BOX_LBL"),   1, 1, 0, 0, 0);
    InitControl(m_btnAccept,    CExoString("LBL_ACCEPT"),     1, 1, 0, 0, 0);
    InitControl(m_btnBack,      CExoString("BACK_LBL"),       1, 1, 0, 0, 0);
    InitControl(m_btnRandom,    CExoString("LBL_RANDOM"),     1, 1, 0, 0, 0);
    InitControl(m_btnIntl,      CExoString("LBL_INTL"),       1, 1, 0, 0, 0);
    InitControl(m_aLblPad[0],   CExoString("LBL_A"),          1, 1, 0, 0, 0);
    InitControl(m_aLblPad[1],   CExoString("LBL_B"),          1, 1, 0, 0, 0);
    InitControl(m_aLblPad[2],   CExoString("LBL_X"),          1, 1, 0, 0, 0);
    InitControl(m_aLblPad[3],   CExoString("LBL_Y"),          1, 1, 0, 0, 0);
    StopLoadFromLayout();
    // A 640x480 console layout would otherwise hang from the screen's top-left corner.
    CenterPanel();
#endif

    for (int i = 0; i < kKeys; i++) {
        m_aStd[i].AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
        m_aStd[i].AddEvent(0x2d, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
        m_aInt[i].AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
        m_aInt[i].AddEvent(0x2d, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
    }
    m_btnSpace.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
    m_btnSpace.AddEvent(0x2d, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
    m_btnBackspace.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
    m_btnBackspace.AddEvent(0x2d, this, static_cast<Handler>(&K2GuiNameKeyboard::OnKey));
    m_btnEnd.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnDone));
    m_btnEnd.AddEvent(0x2d, this, static_cast<Handler>(&K2GuiNameKeyboard::OnDone));

    // The legend buttons are touch-only. Nothing in the pad chain points at them, and the pad
    // reaches their actions through A / B / X / Y (HandleInputEvent).
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnDone));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnBack));
    m_btnRandom.m_tail.bFocusOnHover = 0;
    m_btnRandom.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnRandom));
    m_btnIntl.m_tail.bFocusOnHover = 0;
    m_btnIntl.AddEvent(0x27, this, static_cast<Handler>(&K2GuiNameKeyboard::OnInternational));

    LinkGrid(m_aStd);
    LinkGrid(m_aInt);
    m_btnBackspace.m_moveToControls[MOVE_LEFT]  = &m_btnEnd;
    m_btnBackspace.m_moveToControls[MOVE_RIGHT] = &m_btnSpace;
    m_btnSpace.m_moveToControls[MOVE_LEFT]      = &m_btnBackspace;
    m_btnSpace.m_moveToControls[MOVE_RIGHT]     = &m_btnEnd;
    m_btnEnd.m_moveToControls[MOVE_LEFT]        = &m_btnSpace;
    m_btnEnd.m_moveToControls[MOVE_RIGHT]       = &m_btnBackspace;

    if (!HasRandomName()) {
        // With no random name on this panel, the Y legend goes and Y does nothing
        // (HandleInputEvent).
        m_btnRandom.m_tail.bVisible = 0;
        m_aLblPad[3].m_tail.bVisible = 0;
    }
    if (m_kind == KIND_SAVENAME) {
        // The save panel's own title ("Save Game"), and no chargen sub-title under it.
        CSWGuiSaveNamePanel* pSave = static_cast<CSWGuiSaveNamePanel*>(m_pModel);
        m_lblTitle.m_text.m_params.SetText(pSave->m_lblTitle.m_text.m_params.m_sText);
        m_lblSubTitle.m_text.m_params.SetText(CExoString(""));
    }
#if KOTOR1
    // The PC background has its own plates for the legend: BTN_RANDOM's above the bar (x 218-421)
    // and END_BTN's / BTN_BACK's below it (113-271 / 369-527). The console rectangles put three
    // entries on that lower row, so each entry is centred on a plate instead: International above;
    // Select and Back on the two lower plates, or Random / Select / Back with Select between them.
    CenterK1Legend(m_aLblPad[2], m_btnIntl, 320);
    if (HasRandomName()) {
        CenterK1Legend(m_aLblPad[3], m_btnRandom, 192);
        CenterK1Legend(m_aLblPad[0], m_btnAccept, 320);
    } else {
        CenterK1Legend(m_aLblPad[0], m_btnAccept, 192);
    }
    CenterK1Legend(m_aLblPad[1], m_btnBack, 448);
#endif

    m_bInternational = 1;   // force SetInternational(0) to do the whole job
    SetInternational(0);
    // Naming a save over an existing one starts on Done, so A-A-A keeps its name; an empty name
    // starts on the first key like the character's.
    SetActiveControl(m_kind == KIND_SAVENAME && NameText()[0] ? static_cast<CSWGuiControl*>(&m_btnEnd)
                                                               : &m_aStd[0], 0);
    // Unconditionally the first time, because an empty name equals the empty m_sShown, and
    // RefreshName would then leave NAME_BOX_LBL on the layout's placeholder ("Entered Name").
    m_sShown = NameText();
    m_lblName.m_text.m_params.SetText(m_sShown);
}

CSWGuiEditbox& K2GuiNameKeyboard::Editbox()
{
    if (m_kind == KIND_CHARGEN)
        return static_cast<CSWGuiNameChargen*>(m_pModel)->m_editBox;
    return static_cast<CSWGuiSaveNamePanel*>(m_pModel)->m_editBox;
}

const char* K2GuiNameKeyboard::NameText()
{
    const char* s = Editbox().m_editText.m_sText.m_sString;
    return s ? s : "";
}

bool K2GuiNameKeyboard::IsRandomName()
{
    return m_kind == KIND_CHARGEN && static_cast<CSWGuiNameChargen*>(m_pModel)->m_bNameGenerated;
}

void K2GuiNameKeyboard::ClearRandomName()
{
    if (m_kind == KIND_CHARGEN)
        static_cast<CSWGuiNameChargen*>(m_pModel)->m_bNameGenerated = 0;
}

// Links the keys as the Switch ctor's grid does, a wrapping 13-column grid in which the top row's
// "up" and the bottom row's "down" lead to the bottom bar button under that column.
void K2GuiNameKeyboard::LinkGrid(CSWGuiButton* aKeys)
{
    for (int i = 0; i < kKeys; i++) {
        int nCol = i % kColumns;
        int nRow = i / kColumns;
        CSWGuiButton* pBar = nCol <= 3 ? &m_btnBackspace : nCol <= 8 ? &m_btnSpace : &m_btnEnd;
        CSWGuiControl** pMove = aKeys[i].m_moveToControls;
        pMove[MOVE_LEFT]  = &aKeys[nCol == 0 ? i + kColumns - 1 : i - 1];
        pMove[MOVE_RIGHT] = &aKeys[nCol == kColumns - 1 ? i - kColumns + 1 : i + 1];
        pMove[MOVE_UP]    = nRow == 0 ? static_cast<CSWGuiControl*>(pBar) : &aKeys[i - kColumns];
        pMove[MOVE_DOWN]  = nRow == kKeys / kColumns - 1 ? static_cast<CSWGuiControl*>(pBar) : &aKeys[i + kColumns];
    }
}

// Show one grid, hide the other, keep the selected key's index, and re-point the bottom bar's
// up/down at the visible grid.
void K2GuiNameKeyboard::SetInternational(int bInternational)
{
    bInternational = bInternational != 0;
    if (bInternational == m_bInternational)
        return;
    m_bInternational = bInternational;

    CSWGuiButton* aShow = bInternational ? m_aInt : m_aStd;
    CSWGuiButton* aHide = bInternational ? m_aStd : m_aInt;
    for (int i = 0; i < kKeys; i++) {
        aShow[i].m_tail.bVisible = 1;
        aHide[i].m_tail.bVisible = 0;
    }

    for (int i = 0; i < kKeys; i++) {
        if (m_pActiveControl == &aHide[i]) {
            SetActiveControl(&aShow[i], 1);
            break;
        }
    }

    const int nBottomRow = kKeys - kColumns;
    m_btnBackspace.m_moveToControls[MOVE_UP]   = &aShow[nBottomRow + 1];
    m_btnBackspace.m_moveToControls[MOVE_DOWN] = &aShow[1];
    m_btnSpace.m_moveToControls[MOVE_UP]       = &aShow[nBottomRow + 6];
    m_btnSpace.m_moveToControls[MOVE_DOWN]     = &aShow[6];
    m_btnEnd.m_moveToControls[MOVE_UP]         = &aShow[nBottomRow + 11];
    m_btnEnd.m_moveToControls[MOVE_DOWN]       = &aShow[11];
}

void K2GuiNameKeyboard::RefreshName()
{
    const char* sName = NameText();
    if (m_sShown == sName)
        return;
    m_sShown = sName;
    m_lblName.m_text.m_params.SetText(m_sShown);
}

// HandleDoneButton's own message-box call, with the strref as the argument.
void K2GuiNameKeyboard::ShowMessage(uint64_t nStrRef)
{
    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetCallback(0, 0, 0);
    pBox->SetMessage(nStrRef);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

// Off the modal stack and queued for deletion, with the model shown again underneath: whatever
// runs next (Done, Cancel) pops that one.
void K2GuiNameKeyboard::Close()
{
    m_pGuiManager->PopModalPanel();
#if KOTOR1
    // K1: no panel chain; state 4 (bits 8-10) is "remove and delete" (CSWGuiManager::Draw).
    m_stateFlags = static_cast<unsigned short>((m_stateFlags & ~0x700) | 0x400);
#else
    CSWGuiPanel* pPanel = this;
    do {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~0x380u) | 0x200u;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel);
#endif
    m_pModel->SetVisible(1);
    if (g_pNameKeyboard == this)
        g_pNameKeyboard = 0;
}

void K2GuiNameKeyboard::OnKey(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;

    CSWGuiEditText& editText = Editbox().m_editText;
    const char* sName = NameText();
    int nLength = static_cast<int>(strlen(sName));

    if (pControl == &m_btnBackspace) {
        if (nLength > 0)
            editText.SetText(CExoString(sName, nLength - 1));
        ClearRandomName();
        RefreshName();
        return;
    }

    if (nLength > kMaxName) {
        ShowMessage(STRREF_NAME_TOO_LONG);
        return;
    }

    if (pControl == &m_btnSpace) {
        if (IsRandomName()) {
            // A random name is replaced by the next key; a space cannot start a name.
            editText.SetText(CExoString(""));
        } else if (nLength > 0 && sName[nLength - 1] != ' ') {
            CExoString sNew(sName);
            sNew = sNew + CExoString(" ");
            editText.SetText(sNew);
        }
        ClearRandomName();
        RefreshName();
        return;
    }

    CSWGuiButton* pKey = static_cast<CSWGuiButton*>(pControl);
    const CExoString& sKey = pKey->m_text.m_params.m_sText;
    if (!sKey.m_sString || !sKey.m_sString[0])
        return;
    if (IsRandomName())
        editText.SetText(sKey);
    else
        editText.SetText(CExoString(sName) + sKey);
    ClearRandomName();
    RefreshName();
}

// Trailing spaces are dropped before anything else. An empty character name keeps the keyboard up
// and raises the "enter a name" box over it; otherwise the keyboard comes down first, because the
// model's Done pops the top modal panel.
void K2GuiNameKeyboard::OnDone(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;

    const char* sName = NameText();
    int nLength = static_cast<int>(strlen(sName));
    int nTrimmed = nLength;
    while (nTrimmed > 0 && sName[nTrimmed - 1] == ' ')
        nTrimmed--;
    if (nTrimmed != nLength)
        Editbox().m_editText.SetText(CExoString(sName, nTrimmed));
    RefreshName();

    if (nTrimmed == 0 && RequiresName()) {
        ShowMessage(STRREF_MUST_SPECIFY_NAME);
        return;
    }
    CSWGuiPanel* pModel = m_pModel;
    Kind kind = m_kind;
    Close();
    if (kind == KIND_CHARGEN)
        static_cast<CSWGuiNameChargen*>(pModel)->HandleDoneButton(pControl);
    else
        static_cast<CSWGuiSaveNamePanel*>(pModel)->HandleOkButton(pControl);
}

void K2GuiNameKeyboard::OnBack(CSWGuiControl* pControl)
{
    if (pControl && !pControl->m_tail.nCurrentEventArg)
        return;
    m_pGuiManager->PlayGuiSound(0);
    CSWGuiPanel* pModel = m_pModel;
    Kind kind = m_kind;
    Close();
    if (kind == KIND_CHARGEN)
        static_cast<CSWGuiNameChargen*>(pModel)->HandleCancelButton();
    else
        static_cast<CSWGuiSaveNamePanel*>(pModel)->HandleCancelButton(0);
}

void K2GuiNameKeyboard::OnRandom(CSWGuiControl* pControl)
{
    if (pControl && !pControl->m_tail.nCurrentEventArg)
        return;
    if (!HasRandomName())
        return;
    m_pGuiManager->PlayGuiSound(0);
    static_cast<CSWGuiNameChargen*>(m_pModel)->HandleRandomName();
    RefreshName();
}

void K2GuiNameKeyboard::OnInternational(CSWGuiControl* pControl)
{
    if (pControl && !pControl->m_tail.nCurrentEventArg)
        return;
    m_pGuiManager->PlayGuiSound(0);
    SetInternational(!m_bInternational);
}

// The Switch name panel's arms, with X as International (the Switch used X for its system keyboard).
void K2GuiNameKeyboard::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case 0x28:
        case 0x2e:
            OnBack(0);
            return;
        case 0x29:
            OnInternational(0);
            return;
        case 0x2a:
            OnRandom(0);
            return;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void K2GuiNameKeyboard::Draw(float f)
{
    // The model's OnPanelAdded focused its editbox; take the focus back so the keyboard owns input.
    if (!m_bDefocused) {
        m_bDefocused = 1;
        CSWGuiEditbox& editbox = Editbox();
        if (m_pGuiManager->m_pFocusedEditbox == &editbox)
            editbox.HandleFocusChange(0);
    }
    RefreshName();
    CSWGuiPanel::Draw(f);
}

static void PushKeyboard(CSWGuiPanel* pModel, Kind kind)
{
    if (!pModel || !pModel->m_pGuiManager)
        return;
    if (g_pNameKeyboard && g_pNameKeyboard->m_pModel == pModel)
        return;   // re-added under its own keyboard (a message box closing, say)

    K2GuiNameKeyboard* pKeyboard = new K2GuiNameKeyboard(pModel->m_pGuiManager, pModel, kind);
    g_pNameKeyboard = pKeyboard;
    pModel->SetVisible(0);
#if KOTOR1
    // K1's AddPanel: flag 2 centres the panel (state bit 3) and counts it as full-screen, as the
    // chargen name page's own push does; RemovePanel undoes both.
    pModel->m_pGuiManager->AddPanel(pKeyboard, 3, 0);
#else
    pModel->m_pGuiManager->AddPanel(pKeyboard, 1, 0);
#endif
}

void k2namekeyboard_push_chargen(CSWGuiNameChargen* pPanel) { PushKeyboard(pPanel, KIND_CHARGEN); }
void k2namekeyboard_push_savename(CSWGuiSaveNamePanel* pPanel) { PushKeyboard(pPanel, KIND_SAVENAME); }

#endif  // K2_UI_XBOX || K1_UI_XBOX
