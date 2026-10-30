# Dear ImGui (vendored)

[Dear ImGui](https://github.com/ocornut/imgui) **v1.93.0**, commit
`c1aaac65e896a45f81f47a4d68f9959e0f86c859` (tag `v1.93.0`, 2026-10-14), unmodified.

MIT — see `LICENSE.txt`.

It draws the model viewer's panel (`platforms/viewer/`, the `k1view` / `k2view` executables). Nothing
in the games links it.

Vendored: the core (`imgui*.cpp/.h`, `imconfig.h`, the three `imstb_*.h`; not `imgui_demo.cpp`) and
two backends from `backends/`:

* `imgui_impl_sdl2` — input, from the SDL2 events the engine's loop pumps;
* `imgui_impl_opengl2` — the fixed-function renderer. Both games run in a GL 2.1 compatibility
  context (macOS offers no other way to the ARB/1.x surface the engine renders with), where the
  `opengl3` backend's vertex array objects do not exist. The viewer clears the state this backend
  leaves to its caller (the GLSL program, the buffer bindings, the cube-map enable) around its draw.

To update: copy the same files from a new release, refresh the commit line above, rebuild `k1view`.
