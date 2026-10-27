# Native Backends (Phase 35)

How to use elimgui outside the browser: build modes, consumer setup, and (as later
phases land) the SDL3 backend, the desktop example, idle redraw and platform notes.

---

## Build modes

### libc seam (`core/eli_platform.h`)

| Macro | Libc | Default for | Notes |
|---|---|---|---|
| `ELI_LIBC_SYSTEM` | host libc (`stdint` ... `float`, `stdio`) | every non-wasm32 target | Native apps and unit tests. `ELI_TEST_HOSTED` is an alias. |
| `ELI_LIBC_JACLIBC` | JAClibc (`vendor/jaclibc`) | wasm32 | On wasm32 it also turns on `ELI_JSIO` (JS interop) and the double-only `pow/log/exp` overrides. Natively it is opt-in and JS interop stays off. |

Defining both is a compile error. `ELI_ASSERT(x)` checks library invariants only when
`ELI_DEBUG` is defined (system libc); otherwise it compiles to nothing.

### Shared state across translation units (`core/eli_state.h`)

All library state that lives at file scope (current context, widget pools, clipboard
fallback, allocator hooks, popup stacks, ...) is declared with `ELI_GLOBAL` /
`ELI_GLOBAL_ARRAY` / `ELI_GLOBAL_ARRAY_INIT`.

- **Default:** `static` in every TU. One TU per program (the WASM model). Two TUs
  still link, but each gets its own context, clipboard and allocator.
- **`ELI_SHARED_STATE`:** `extern` in every TU and defined once, in the TU that
  defines `ELI_IMPLEMENTATION` before including elimgui. That TU also compiles the
  stb_truetype / stb_rect_pack bodies (external linkage); other TUs see declarations.

New file-scope state must use these macros (the check is
`grep -rn '^static [^(]*;' include/eli | grep -v 'static inline'`, which must print nothing).

### Host services (`util/eli_host_io.h`)

| Service | WASM | Native (`ELI_LIBC_SYSTEM`) |
|---|---|---|
| Settings file (`eli_load/save_ini_settings_*_disk`) | JS host import | `fopen`/`fread`/`fwrite` |
| Log to TTY / file | JS host import | `stdout` / append to the file |
| Open URL (`eli_platform_open_url`, `eli_text_link_open_url`) | `window.open` | `io.platform_open_url_fn(io.platform_user_data, url)`; NULL = no-op |
| Clipboard | fallback buffer + JS bridge | `io` clipboard callbacks set by the backend |

`io.ini_filename` defaults to NULL, so nothing is written unless the app asks. On iOS
the cwd is the read-only bundle: pass an absolute path (e.g. from `SDL_GetPrefPath`).

---

## Consumer setup

- Include paths: `elimgui/include`, `elimgui/vendor` (stb), `elimgui/backends` (SDL3 backend).
- Build flags in every TU: `-DELI_SHARED_STATE -DELI_DRAW_IDX_32` (recommended natively).
- Exactly one TU:

  ```c
  #define ELI_IMPLEMENTATION
  #include <eli/elimgui.h>
  ```

  Other TUs include `<eli/elimgui.h>` only.
- If the app also compiles stb_truetype or stb_rect_pack itself, those symbols clash
  with the copies in the `ELI_IMPLEMENTATION` TU under `ELI_SHARED_STATE`.

---

## SDL3 backend (`backends/sdl3/`)

Header-only, `static inline`, requires SDL >= 3.2 (developed on 3.4.10). The core never
includes SDL; the backend includes the core. Platform and renderer are separate headers
so either can be replaced alone.

| Header | Role |
|---|---|
| `sdl3/eli_sdl3_platform.h` | SDL events -> `eli_io`; cursor, clipboard, open URL, mouse warp; display size / density / delta time |
| `sdl3/eli_sdl3_renderer.h` | `eli_draw_data` -> `SDL_RenderGeometryRaw`; RGBA texture helpers; font-atlas upload |
| `sdl3/eli_sdl3_keymap.h` | SDL keycode/scancode -> `eli_key` (used by the platform header) |

### Platform API

```c
bool eli_sdl3_platform_init(SDL_Window *window);   /* clipboard + open-URL callbacks, starts text input */
void eli_sdl3_platform_shutdown(void);
bool eli_sdl3_platform_process_event(const SDL_Event *e); /* true = consumed (build a frame) */
void eli_sdl3_platform_new_frame(void);   /* display size (points), fb scale (density), dt */
void eli_sdl3_platform_end_frame(void);   /* cursor shape, mouse warp */
```

- Mouse buttons SDL 1/3/2/X1/X2 -> eli 0/1/2/3/4; wheel `(-x, y)` like the web glue.
- Keys: keypad and punctuation by scancode, named keys/letters/digits by keycode.
- Text: `SDL_EVENT_TEXT_INPUT` -> `eli_io_add_input_characters_utf8`. Text input is
  started once at init (desktop). want_text_input-driven start/stop, the IME rect and
  the on-screen keyboard are later phases.
- Focus lost releases every held key and mouse button; mouse leaving the window sends
  an invalid mouse position.
- Clipboard: `SDL_GetClipboardText` result is cached and `SDL_free`d on the next get
  or at shutdown; no size cap in the backend.

### Renderer API

```c
bool           eli_sdl3_renderer_init(SDL_Renderer *r);
void           eli_sdl3_renderer_shutdown(void);
bool           eli_sdl3_renderer_create_fonts_texture(eli_font_atlas *atlas); /* sets tex id */
void           eli_sdl3_renderer_destroy_fonts_texture(void);
eli_texture_id eli_sdl3_texture_create_rgba(int w, int h, const void *pixels);
bool           eli_sdl3_texture_update_rgba(eli_texture_id id, const SDL_Rect *rect,
                                            const void *pixels, int pitch);
void           eli_sdl3_texture_destroy(eli_texture_id id);
void           eli_sdl3_renderer_render_draw_data(const eli_draw_data *dd);
```

`render_draw_data` sets `SDL_SetRenderScale(framebuffer_scale)` itself (draw data and
clip rects stay in points), expands each list's RGBA8 colors to `SDL_FColor` once per
list (SDL3 has no 8-bit color geometry path), honors `cmd->vtx_offset` and user
callbacks, and restores viewport, clip and scale. Textures are `SDL_PIXELFORMAT_RGBA32`,
static, blended, linear. The app clears and presents.

### DPI and fonts

- Create the window with `SDL_WINDOW_HIGH_PIXEL_DENSITY`.
- `io.display_size` = `SDL_GetWindowSize` (points); `io.display_framebuffer_scale` =
  `SDL_GetWindowPixelDensity`.
- Bake fonts with `rasterizer_density` = pixel density (see font-system.md); rebake and
  re-upload on `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`.
- User content scale (e.g. Linux 125%): `SDL_GetWindowDisplayScale / density`; multiply
  font sizes by it and call `eli_style_scale_all_sizes(&style, s)` once.

### Consumer build (static SDL3)

```c
/* every TU: -DELI_SHARED_STATE -DELI_DRAW_IDX_32
   -I elimgui/include -I elimgui/vendor -I elimgui/backends */
#define ELI_IMPLEMENTATION          /* exactly one TU */
#include <eli/elimgui.h>
#include <sdl3/eli_sdl3_platform.h>
#include <sdl3/eli_sdl3_renderer.h>
```

Link flags for a static SDL3 3.4.10 (`pkg-config --static --libs sdl3`):

- **macOS:** `libSDL3.a -framework CoreMedia -framework CoreVideo -framework Cocoa
  -weak_framework UniformTypeIdentifiers -framework IOKit -framework ForceFeedback
  -framework Carbon -framework CoreAudio -framework AudioToolbox -framework AVFoundation
  -framework Foundation -framework GameController -framework Metal -framework QuartzCore
  -weak_framework CoreHaptics -lpthread -lm`
- **Linux:** `libSDL3.a -lm -pthread`. SDL3 loads X11/Wayland/ALSA/etc. with `dlopen` by
  default, so they are not link-time dependencies (they must be present at runtime).

`./build.sh sdl3-deps` builds SDL3 from the release tarball as a static library into
`examples/sdl3/build/deps/prefix` (never system-wide); `ELI_SDL3_STATIC=1` makes the
example link it. Extra CMake flags go in `SDL3_CMAKE_ARGS`.

---

## Desktop example (`examples/sdl3/`)

Three translation units built with `ELI_SHARED_STATE` + `ELI_DRAW_IDX_32`: `main.c` (the
`ELI_IMPLEMENTATION` TU: SDL, fonts, loop), `shell.c` (viewport dockspace split into
Files | Editor / Perf fixture, a table, single- and multi-line input) and
`perf_fixture.c` (120 x 48 monospace text block, ~17.8K vertices visible, plus a frame
counter overlay with build and submit times).

```
./build.sh sdl3-example            # -O2, dynamic SDL3 from pkg-config
./build.sh sdl3-example-debug      # -O0 -g, ASan + UBSan, ELI_DEBUG
ELI_SDL3_STATIC=1 ./build.sh sdl3-example
examples/sdl3/build/elimgui_sdl3 [--frames N] [--screenshot out.bmp] [--font f.ttf]
                                 [--font-size PX] [--driver NAME]
```

`--frames N` renders N frames and exits 0 (used for the sanitizer runs); `--screenshot`
saves the last frame as BMP. On macOS the debug build uses Homebrew LLVM: Apple clang
17's ASan runtime hangs at startup on macOS 26 (even for an empty `main`).

### Results (Phase B)

| Check | macOS 26 arm64 (M4) | Linux x86-64 (Ubuntu 26.04, gcc 15) |
|---|---|---|
| Unit suites (`./build.sh test`, both passes) | 142/142 | 142/142 |
| Example renderer | `metal`, density 2 | `software` under `SDL_VIDEODRIVER=offscreen` (no display or X11/Wayland dev files on the test box) |
| `--frames 120` under ASan + UBSan | clean | clean |
| Binary, dynamic SDL3 (-O2) | 200 KB + libSDL3.dylib | — |
| Binary, static SDL3 (-O2) | 3.06 MB (2.65 MB stripped) | 3.94 MB |
| Size probe: 1 TU vs 5 TU text (-O2) | 112 KB vs 192 KB (+71%) | 136 KB vs 362 KB (+166%) |

The size probe (`tests/probe/tu_size_probe.sh`) builds the same five files as one TU
and as five: each TU that uses a widget family carries its own copy of the
`static inline` code. Acceptable on desktop; D4(c) (one compiled implementation) stays
the fallback if it matters on iOS.

---

## JAClibc on native targets (probe)

`tests/probe/jaclibc_native_probe.sh` builds every unit suite with `-DELI_LIBC_JACLIBC`
against `vendor/jaclibc` and the system compiler. A target may use JAClibc natively only
if every suite builds and passes with `-Werror`. Results are recorded, not fixed here
(JAClibc is a separate project).

| Target | Date | Result | First errors |
|---|---|---|---|
| macOS arm64 (Apple clang 17) | 2026-10-27 | 0/70 build | JAClibc's `sys/time.h:286` calls undeclared `clock_settime`; `threads.h:370` calls undeclared `pthread_cond_timedwait` (both in every TU) |
| Linux x86-64 (gcc 15) | 2026-10-27 | 0/71 build | `fatal error: x/ config_has.h: No such file or directory` (malformed include in JAClibc) |

Verdict: native builds use the system libc.
