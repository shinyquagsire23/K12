# The model viewer (k1view / k2view)

One model in a room, a coloured dynamic light circling it, and a Dear ImGui panel: a test bench for
the renderer (skins, bump maps, reflection maps, shadows, the frame-buffer passes) and a quick look at
a model for modders. Desktop only: built and run on macOS; the Linux and Windows executables are
wired the same way (`k2p_add_viewer` in their CMakeLists) but not yet built.

```sh
./build_run_viewer.sh            # KOTOR II: c_hutt in BioWare's test room 000test
./build_run_viewer.sh --k1       # KOTOR 1: lqa_dewback in BioWare's QA room mqatester
./build_run_viewer.sh --k1 --model c_rancor --room m10aa --anim run
```

Mouse: left drag orbits, right (or Shift + left) drag pans, the wheel zooms. WASD moves the camera
along and across the view (Q / E down and up, Shift faster) while no panel field has the keyboard.
Tab hides the panel.
The panel lists every model and layout in `chitin.key` and `override/`, the model's animations (its
own and its supermodels'), the `cm_*` reflection maps, the light (colour or a colour cycle,
brightness, radius, orbit, height, speed), the ambient colour, the wind (an area's None / Light /
Heavy, or any strength, direction and gusts), the grass, the backdrop and the renderer's switches.
The model can cast its shadow (the engine shadows character and item models only, from the light the
scene's focus picks: the model stands in for the player) and stand in the stealth field (the
distortion frame-buffer pass a stealthed creature gets). A backdrop is lit as its area is: the `.are` of the layout's name gives the sun's ambient and diffuse
colours (the moon's at night), the ambient for creatures and placeables, the fog and the shadows'
opacity, applied as `CSWCArea::SetAsDay` does (the sun model's `gidy_sun_amb` / `gidy_sun_diff`
recoloured), and in KOTOR II the three noise textures its rooms' "dirt" meshes are roughened with
(`Dirty*One` / `Two` / `Three`: Telos, Nar Shaddaa, Onderon); the Light section edits them. A layout with no `.are` is lit as nearly every area is: no
sun (its light is baked into the lightmaps) and an interior's ambient. A backdrop's grass is its area's too: the `.are`
of the layout's name in `modules/` (RIM, ERF or MOD), whose `Grass_*` fields `CSWCArea` files under
every surface material `surfacemat.2da` marks as grass; a layout with no `.are` (BioWare's test rooms)
gets the usual blade layout in the game's grass texture. The panel's Grass section changes any of it
and rebuilds the backdrop (blades are planted as the rooms spawn). The window's close button quits:
both games answer it with their quit dialog, which the viewer never shows.

## BioWare's QA room (KOTOR 1, `mqatester`)

One room built to exercise every renderer feature, a labelled station each, along the two long walls
(`K2VIEW_CAMERA=4.71,1.35,6` faces the east wall, `1.57,1.35,6` the west; `K2VIEW_TARGET` slides
along them, y 11..47):

* **East wall:** animated texture (`lqa_birdsheet`, a 4x4 `proceduretype cycle`), lightmapped /
  environment mapped, lightmapped / bumpyshiny, lightmapped / bumped, lightmapped / textured,
  lightmapped / dynamic, environment mapped, bumpyshiny, bumped, textured, water (procedural noise),
  grass.
* **West wall:** animated uv's, animated uv's / bumpyshiny, lens flare, animated lens flare, room
  animation, animated auroralight, particle emitter, danglymesh (the flag: the panel's Wind), and
  three skinned creatures referenced by the room (gizka, mykal, dewback: plain, bumped, bumpyshiny).

The textures' TXIs say what each tests: `lqa_panel2` reflects `CM_QATEST` (alpha = reflectivity),
`lqa_mykal` bump maps, `lqa_dewback` bump maps and reflects (`bumpyshinytexture`), `lqa_water` is
bumpy-shiny over a `proceduretype water` bump map. `CM_QATEST` is a colour-coded test cube (one
colour per face) shipped only as six face textures `cm_qatest0..5`; its TXI never shipped, and
`K2_QOL_FACE_CUBEMAPS` loads it from the faces. The grass station has no `.are`, so it gets the
viewer's default grass (Dantooine's `lda_grass2`). The lightmapped bump stations are see-through with no
dynamic light near, as on the PC (docs/prep/lmbump_embm_v2263.md); bring the light over them.

## How it works

The viewer is the game's own executable (`platforms/macos/main.cpp` finds the game as the game
does) plus three files:

* `viewer_host.cpp` installs a `K2HostViewer` (`src/port/k2viewerapi.h`) in `g_pK2HostViewer`
  before `main`. The engine's start-up (`WinMain` for KOTOR II, `GameInit` for KOTOR 1) runs as
  usual — resources, Aurora, the client's services and options — and then calls `Start` instead of
  the intro and the main menu, and `Frame` instead of the client's main loop each frame. SDL, the
  panel and the GL state around it live here.
* `viewer_engine.cpp` builds the scene from the engine's own pieces, as the client builds an area:
  the layout's rooms (`CLYT` + `Scene::SpawnRoom`), the sun (`gidy_sun`), ambient, static lighting,
  the model and the camera (the follow camera's yaw / pitch placement). It is compiled with the
  engine's flags, so it may include `src/`'s headers and no system header.
* `viewer.cmake` makes `<game>view` from the game executable's sources, settings and links.

The light is a text model (`kLightModel`): a dynamic light node and a small self-lit marker. The
engine still parses BioWare's text `.mdl` format; the viewer registers the console commands a text
model is made of (`newmodel`, `beginmodelgeom`, ...), which the shipped games do not, so a text
model in `override/` loads too.

KOTOR II's window is set through the engine's `SetResolution` (1024x768, or `K2_WINDOW`), KOTOR 1's
through `K1_WINDOW`; the scene is built on the first frame, after KOTOR II's re-initialisation.

## Switches

| variable | |
|---|---|
| `K2VIEW_MODEL=<resref>` | the model to start with (`none` for none) |
| `K2VIEW_ROOM=<layout>` | the backdrop (`none` for none) |
| `K2VIEW_ANIM=<name>` | the animation to start with |
| `K2VIEW_CAMERA=<yaw>,<pitch>,<dist>` | the camera about the model (radians, radians, metres) |
| `K2VIEW_TARGET=<x>,<y>,<z>` | what the camera looks at, instead of the model |
| `K2VIEW_PANEL=0` | start with the panel hidden (screenshots) |
| `K2VIEW_OFF=<name>,...` | renderer switches to start off, by the panel's first word (`Shadows`, `Bumpy-shiny`) |
| `K2VIEW_MODELPOS=<x>,<y>[,<z>]` | where the model, and the light's orbit, goes (no z: on the floor) |
| `K2VIEW_LIGHT=<r>,<g>,<b>,<brightness>,<orbit>,<height>` | a fixed-colour light |
| `K2VIEW_STEALTH=1` | start with the model in the stealth field |
| `K2VIEW_ENVMAP=<cm_*>` | the reflection map to start with, instead of the model's own |
| `K2_CTL=<file>` | the game's scripted actions (`shot:NAME` for a screenshot) |
