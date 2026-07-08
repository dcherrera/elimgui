# Web Integration (Phase 31)

This phase makes elimgui *run*: a WebAssembly build of the library plus a
Canvas2D renderer and DOM input wiring, so the immediate-mode UI is interactive
in a browser. No Emscripten — the module is built with `clang --target=wasm32`
and linked with `wasm-ld`.

## Pieces

| File | Role |
|------|------|
| `examples/demo/main.c` | The app: exported WASM entrypoints + a self-contained demo UI. |
| `examples/demo/eli_wasm_runtime.h` | Freestanding libc glue so the module links standalone. |
| `web/elimgui.js` | WASM loader, Canvas2D renderer, and DOM event wiring. |
| `web/index.html` | Full-window `<canvas>` shell that boots the loader. |
| `web/style.css` | Full-viewport canvas styling. |

## Build & serve

```bash
./build.sh demo     # clang --target=wasm32 ... -> web/demo.wasm
./build.sh serve    # python3 -m http.server 8080, serving web/
```

Then open <http://localhost:8080>. You should see a draggable window with tabs
(*Widgets*, *Table*, *About*) exercising text, buttons, a checkbox, radio
buttons, a slider, a drag float, numeric + text inputs, a color editor, a
progress bar, and a bordered table — all rendered to the canvas and responsive
to the mouse and keyboard.

## The WASM module (`main.c`)

The app talks to JavaScript through exported functions (`JS_EXPORT`) — there is
no hidden `main()` loop. The exported surface:

- **Lifecycle**
  - `js_start()` — create the context, apply the dark theme, bake the default
    font atlas, publish its pixels via the atlas getters, and make the font
    current.
  - `frame(dt)` — set `io.delta_time`, run `eli_frame_begin()`, build the demo
    window, then `eli_frame_end()`. `dt` is seconds since the previous frame.
  - `eli_set_display_size(w, h)` — set `io.display_size`; called on load/resize.
- **Font atlas getters** — `eli_atlas_pixels/width/height` expose the baked
  RGBA32 texture so the renderer can upload it once.
- **Draw-data getters** — `eli_dd_list_count`, and per-list
  `eli_dd_vtx_ptr/vtx_count/idx_ptr/idx_count/cmd_ptr/cmd_count`. These hand the
  renderer raw pointers into WASM linear memory, which it reads with a
  `DataView`.
- **Input entrypoints** — `eli_on_mouse_pos/mouse_button/mouse_wheel/key/char`
  forward straight into the library's `eli_io_add_*` event queue.

### Binary layouts read by the renderer

Read directly from linear memory (little-endian):

```
vertex  (stride 20): float x, y, u, v;  uint32 col     // col is RGBA, R in low byte
command (stride 40): float clip.x, y, w, h;            // scissor, origin+size
                     uint32 texture_id, vtx_offset,
                            idx_offset, elem_count;
                     ptr    user_callback, user_data;   // ignored by this backend
index   (uint16)
```

### Why the runtime-glue header

`build.sh` compiles with `-nostdlib` and routes libc through JAClibc. JAClibc is
header-only, but in this checkout its full-implementation bundle (`<static.h>`,
enabled by `JACL_MAIN`) does not configure for the wasm/jsrun target
(`JACL_FMT` is undefined), and its `printf`/`strtod` path pulls 128-bit
`long double` soft-float builtins (`__addtf3`, `__multf3`, …) that compiler-rt
for wasm32 would have to supply — which `-nostdlib` excludes.

JAClibc's header-only *declarations* otherwise link cleanly. With `NDEBUG`
(no `assert`), the only runtime symbols the elimgui library leaves undefined are
the allocator, `qsort`, `strtod`/`atof`, and the bounded formatter.
`eli_wasm_runtime.h` supplies exactly those, all self-contained and free of
`long double`:

- a first-fit free-list allocator (`malloc`/`free`/`calloc`/`realloc`) over
  linear memory grown in whole pages past `__heap_base`;
- an insertion-sort `qsort` (used once during atlas baking);
- a double-based `strtod`/`atof`;
- a compact `vsnprintf`/`snprintf` covering the conversions widget labels use.

It is demo-only build glue, not part of the library API, and is included once
from `main.c` after `<eli/elimgui.h>`.

## The loader + renderer (`elimgui.js`)

`loadElimgui('demo.wasm')`:

1. **Instantiates** the module, supplying the one env import the library needs,
   `eli_host_set_clipboard(ptr)` (decodes the C string and best-effort writes to
   `navigator.clipboard`).
2. Calls `js_start()`, then builds a `FontAtlas`: the RGBA32 texels are copied
   into an offscreen canvas via `ImageData`. A small cache produces color-tinted
   copies of the atlas (draw atlas → `source-in` fill with the color) so glyphs
   can be blitted in any vertex color; pure white text uses the base atlas.
3. Sizes the canvas to the viewport (devicePixelRatio-aware, `ctx.setTransform`
   for crisp text) and reports the CSS size with `eli_set_display_size`.
4. Installs DOM listeners (`mousemove`, `mousedown`/`mouseup`, `wheel`,
   `keydown`/`keyup`, `keypress`, `contextmenu`, `resize`) that call the input
   entrypoints. Mouse buttons are remapped to elimgui's order (left=0, right=1,
   middle=2); `KeyboardEvent.code` is mapped to `eli_key` codes.
5. Runs a `requestAnimationFrame` loop: compute `dt`, `frame(dt)`, clear the
   canvas, then paint the draw data.

### Rendering approach

For each draw list, for each command: set the clip rect
(`save` → `rect` → `clip`), then walk the command's index range as triangles.

- **Solid geometry** — a triangle whose three vertices share one UV samples the
  atlas white texel; it is filled as a colored triangle path.
- **Text** — a triangle with varying UVs is part of a glyph quad. elimgui emits
  each glyph as two triangles over an axis-aligned rectangle, so the renderer
  consumes them in pairs, computes the screen rect (min/max x,y) and atlas
  sub-rect (min/max u,v → texels), and `drawImage`s the (tinted) atlas sub-rect
  onto the screen rect. Vertex alpha is applied via `globalAlpha`.

Because the atlas is re-baked to a single texture (glyphs + white texel), every
command uses one texture and the per-triangle UV test is enough to route between
the two paths. Linear memory can grow during `frame()`, so the renderer
re-acquires `memory.buffer` (a fresh `DataView`) every frame.

### Approximations

This is a "correct-enough" 2D backend, not a GPU pipeline:

- Only axis-aligned glyph quads are blitted; a hypothetical lone textured
  triangle is skipped (elimgui text always emits full quads).
- Anti-aliased line/fill geometry from the library is drawn as plain filled
  triangles (no edge feathering) — the draw-list path is non-anti-aliased by
  design, and Canvas2D smooths blits itself.
- Tinted-atlas canvases are cached per RGB color; the demo uses only a handful.

## Verifying without a browser

The C pipeline can be exercised headless in Node (no DOM needed): instantiate
`web/demo.wasm`, provide the `eli_host_set_clipboard` stub, call `js_start()`,
`eli_set_display_size(...)`, and `frame(dt)`, then read the draw-data getters. A
populated frame yields one draw list with ~1300 vertices / ~2000 indices across
~12 commands, split into solid triangles and glyph quads — confirming the whole
context → widgets → draw-data path runs without trapping. Pixel-level
verification (that it *looks* right) requires opening it in a browser as above.
