# elimgui

A ground-up reimplementation of [Dear ImGui](https://github.com/ocornut/imgui) in pure C11, targeting WebAssembly. Not a binding or wrapper — a full rewrite from scratch designed for the browser.

## Why

Dear ImGui is the gold standard for immediate-mode GUIs, but it's C++ and assumes native platform targets. elimgui brings the same programming model to the web using raw WebAssembly — no Emscripten, no runtime bloat.

Originally conceived by **Rodney Giles**, this project is being built out by **David Herrera** due to time constraints.

## Visual Cheatsheet

The repo ships an interactive **visual cheatsheet** — an elimgui app that showcases every widget live and pairs it with the exact `eli_*()` call plus a copy-paste snippet. It's the fastest way to see what elimgui can do and how to call it (and it dogfoods the whole library).

- Searchable, category-filtered browser of 80+ widget examples
- Each card shows the **live interactive widget** + its API signature + a **copyable code snippet**
- A live **docking playground** — drag panels to dock / undock / split / tab

### Run it

```bash
./build.sh serve
```

That builds the cheatsheet to WebAssembly, starts a local server, and opens it at **http://localhost:8080/cheatsheet.html**. (Use `./build.sh serve demo.html` for the simpler widget demo.)

**Why a browser?** elimgui is WASM/browser-first: the library emits renderer-agnostic draw lists, and the only bundled renderer is the Canvas2D backend in `web/elimgui.js`. So the app runs in the browser — there's no native (SDL/GLFW) backend yet. For headless checks there's `./build.sh dogfood`, and the unit tests run natively via `./build.sh test`.

## Tech Stack

| | |
|---|---|
| **Language** | C11 (header-only) |
| **Target** | WebAssembly via `clang --target=wasm32` |
| **Libc** | [JAClibc](https://github.com/dcherrera/jaclibc) — WASM-first, header-only C library |
| **Font Rendering** | stb_truetype (vendored) |
| **JS Interop** | JAClibc's `jsio.h` (`JS_CODE`, `JS_EXPORT`, `JS_IMPORT`) |
| **Toolchain** | Clang + wasm-ld (no Emscripten) |

## Architecture

elimgui outputs renderer-agnostic draw lists — vertex buffers and draw commands that can be consumed by Canvas2D, WebGL, or WebGPU.

```
User code → eli_*() widgets → eli_draw_list → eli_draw_data → Your renderer → Canvas
```

### Header Structure

```
include/eli/
├── elimgui.h        # umbrella — one include pulls in the whole library
├── core/            # types, enums, IO, style struct, context, frame lifecycle
├── draw/            # draw lists, primitives, paths, beziers, channels
├── font/            # atlas (stb_truetype), embedded font, glyph ranges, text
├── input/           # mouse, keyboard, text, shortcuts, clipboard
├── id/              # ID hashing/stack, active/hot id, storage
├── style/           # themes, style stacks, color utilities
├── window/          # windows: move/resize/scroll, child windows
├── layout/          # cursor, item layout, groups, sizing
├── widgets/         # every widget: text, buttons, sliders, inputs, color,
│                    #   combo, trees, menus, popups, tooltips, tables, tabs, …
├── interaction/     # disabling, clipping, focus
├── util/            # list clipper, viewport, settings, logging, memory
├── demo/            # demo + debug windows
└── docking/         # dock nodes, split, dock space
```

Header-only, but organized into small, focused files by category (nothing over ~1000 LOC). A single `#include <eli/elimgui.h>` pulls in everything. See [`docs/`](docs/) for per-phase API docs.

## Design Decisions

- **Header-only** — No separate compilation units. `#include` and go.
- **WASM-first** — No POSIX assumptions. All platform interaction goes through JAClibc's JS interop layer.
- **No Emscripten** — Direct `clang --target=wasm32` compilation with `-nostdlib`. The resulting binary contains only what's needed.
- **No hidden allocations** — User controls all memory.
- **Immediate mode** — No retained widget state except what's explicitly stored.
- **ImGui-familiar API** — Same patterns, different prefix (`eli_` instead of `ImGui::`).

## Code Example

```c
#include <eli/elimgui.h>

eli_context* ctx = eli_create_context();
eli_set_current_context(ctx);
eli_style_colors_dark(NULL);
// ... build a font atlas, set io.display_size ...

// Each frame:
eli_frame_begin();                    // composes the per-subsystem frame hooks

eli_begin("My Window", NULL, 0);
    eli_text("Hello from WASM");
    if (eli_button("Click me")) {
        // handle click
    }
    static float speed = 1.0f;
    eli_slider_float("Speed", &speed, 0.0f, 10.0f, "%.1f", 0);
eli_end();

eli_frame_end();                      // assembles the frame's draw data
eli_draw_data* draw = eli_get_draw_data();
// feed draw data to your Canvas2D/WebGL renderer
```

## Build & Run

Clone with submodules so JAClibc is present, and use a wasm-capable clang + `wasm-ld`
(on macOS: `brew install llvm lld`):

```bash
git submodule update --init --recursive

./build.sh serve            # build the cheatsheet, serve it, open the browser
./build.sh serve demo.html  # …the simpler widget demo instead
./build.sh test             # build + run the native unit-test suite
./build.sh dogfood          # headless: run the app natively and dump UI state
./build.sh clean            # remove build artifacts
```

Under the hood, a build is a single `clang` invocation — no Emscripten:

```bash
clang --target=wasm32 -nostdlib \
    -Ivendor/jaclibc/include -Iinclude -Ivendor -Os \
    -Wl,--no-entry -Wl,--export-dynamic \
    -o web/cheatsheet.wasm examples/cheatsheet/main.c
```

## Status

Feature-complete: the full Dear ImGui feature set is implemented, covered by a native unit-test suite, and runs in the browser via the Canvas2D renderer.

| Area | Status |
|------|--------|
| Core — types, context, draw, font, input, id, style, memory | ✅ |
| Windows, layout, scrolling, child windows | ✅ |
| Widgets — text, buttons, sliders, drags, inputs, color, combo, trees, tables, tabs, … | ✅ |
| Menus, popups, modals, tooltips, drag & drop | ✅ |
| Disabling/clipping, list clipper, viewports, settings, logging | ✅ |
| Docking — dock space, split, tab, tear-out | ✅ |
| Demo & debug windows, visual cheatsheet | ✅ |
| Web integration — WASM loader, Canvas2D renderer, input | ✅ |
| Native rendering backend (SDL/GLFW/OpenGL) | Not yet |

## License

MIT-T
