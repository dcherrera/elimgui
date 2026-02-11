# elimgui

A ground-up reimplementation of [Dear ImGui](https://github.com/ocornut/imgui) in pure C11, targeting WebAssembly. Not a binding or wrapper — a full rewrite from scratch designed for the browser.

## Why

Dear ImGui is the gold standard for immediate-mode GUIs, but it's C++ and assumes native platform targets. elimgui brings the same programming model to the web using raw WebAssembly — no Emscripten, no runtime bloat.

Originally conceived by **Rodney Giles**, this project is being built out by **David Herrera** due to time constraints.

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
├── elimgui.h          # Master include, core types, context
├── eli_draw.h         # Draw primitives, paths, beziers, channels
├── eli_font.h         # Font atlas, glyph ranges, text rendering
├── eli_input.h        # Mouse, keyboard, shortcuts, clipboard
├── eli_widgets.h      # Buttons, sliders, inputs, trees, tables
├── eli_layout.h       # Cursor, spacing, groups, content regions
├── eli_style.h        # Colors, sizing, theming
├── eli_tables.h       # Full table widget system
└── eli_docking.h      # Window docking (planned)
```

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

// Frame loop
eli_new_frame();

eli_begin("My Window", NULL, 0);
  eli_text("Hello from WASM");
  if (eli_button("Click me", (eli_vec2){0, 0})) {
      // handle click
  }
  eli_slider_float("Speed", &speed, 0.0f, 10.0f, "%.1f", 0);
eli_end();

eli_render();
eli_draw_data* draw = eli_get_draw_data();
// feed draw data to your Canvas2D/WebGL renderer
```

## Build

```bash
clang --target=wasm32 \
    -nostdlib \
    -Ivendor/jaclibc/include \
    -Iinclude \
    -O2 \
    -Wl,--no-entry \
    -Wl,--export-dynamic \
    -o web/demo.wasm \
    examples/demo/main.c
```

Or use the build script:

```bash
./build.sh demo     # Build the demo
./build.sh serve    # Start dev server at localhost:8080
```

## Status

Actively in development. Core systems are implemented; widget and window layers are in progress.

| Phase | Status |
|-------|--------|
| Core types & context | Done |
| Draw system (primitives, paths, beziers, channels) | Done |
| Font system (atlas, stb_truetype, glyph ranges) | Done |
| Input system (mouse, keyboard, shortcuts, clipboard) | Done |
| ID system & state management | In progress |
| Style system & theming | Planned |
| Windows & scrolling | Planned |
| Layout system | Planned |
| Widgets (buttons, sliders, inputs, trees, tables) | Planned |
| Web integration & renderers | Planned |

## License

MIT-T
