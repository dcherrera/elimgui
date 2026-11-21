# Live Widgets

`include/eli/live/` (core) · `backends/sdl3/eli_sdl3_live.h` (macOS Core Animation)

A small retained layer for the few widgets that animate or update on their own: a pulsing
dot, a spinner, a progress bar, a short text (a clock, a counter). Each one is laid out by
the normal immediate-mode pass but drawn by a tiny platform layer that the OS compositor
animates. The main surface does not redraw while they animate, so its idle swapchain
release ([native-backends.md](native-backends.md#present-surface)) keeps working.

Why: any visible change in an immediate-mode UI redraws the whole window. A 12.5 fps pulse
keeps the window's drawable pool (2-3 window-sized IOSurfaces) and ~53 MB of driver GPU
memory alive: ~130 MB of graphics in Argus Desktop against ~30 MB idle.

---

## API

```c
/* Laid out at the cursor like any widget. Return the id for the setters. */
eli_id eli_live_dot(const char *str_id, float radius, eli_col32 col, const eli_live_pulse *pulse);
eli_id eli_live_spinner(const char *str_id, float radius, eli_col32 col);
eli_id eli_live_progress(const char *str_id, eli_vec2 size, float fraction);
eli_id eli_live_text(const char *str_id, const char *text, float width);

/* Placed: a rect in screen points inside the current window (custom-drawn views). */
void eli_live_dot_at(eli_id id, eli_rect rect, eli_col32 col, const eli_live_pulse *pulse);
void eli_live_spinner_at(eli_id id, eli_rect rect, eli_col32 col);
void eli_live_progress_at(eli_id id, eli_rect rect, float fraction);
void eli_live_text_at(eli_id id, eli_rect rect, const char *text);

/* Property updates by id, inside or between frames. With a backend: no frame. */
bool eli_live_set_fraction(eli_id id, float fraction);
bool eli_live_set_text(eli_id id, const char *text);
bool eli_live_set_color(eli_id id, eli_col32 col);

/* Setup and queries */
void eli_live_set_backend(const eli_live_backend *backend);   /* NULL = fallback for all */
void eli_live_set_fallback_interval(float seconds);           /* default 0.25; 0 = static */
eli_live_widget *eli_live_find(eli_id id);
const eli_live_stats *eli_live_get_stats(void);               /* count, layers, calls, overflow */
float eli_live_pulse_alpha(eli_live_pulse pulse, double t);   /* the curves, for backends */
float eli_live_spinner_angle(double t);
```

The values a submit passes win: a setter's change lasts until the next frame submits the
widget again, so the app passes its current value to both.

- `eli_live_pulse { float period; float min_alpha; }`: opacity runs 1 -> `min_alpha` -> 1
  on a cosine over `period` seconds. `ELI_LIVE_PULSE_DEFAULT` is `{ 1.4f, 0.3f }` (Argus's
  working dot). NULL = a solid dot.
- Spinner: a 270° arc, one turn per `ELI_LIVE_SPINNER_PERIOD` (1 s), thickness
  `radius / 4`.
- Progress: `ELI_COL_FRAME_BG` track, `ELI_COL_PLOT_HISTOGRAM` fill, `frame_rounding`; no
  overlay label. `size` 0 = item width x frame height, like `eli_progress_bar`.
- Text: `ELI_COL_TEXT`, the current font and size, one line, cut at the rect.
  `width <= 0` = the width of the text given. At most `ELI_LIVE_TEXT_MAX - 1` bytes
  (63; longer text is cut at a UTF-8 boundary).

### Ids and lifetime

- Ids come from the window id stack (`eli_get_id(str_id)`), so the same label in two
  windows is two widgets. `_at` takes an id the caller made (`eli_get_id`, `_int`).
- A widget exists from the first frame it is submitted until the end of the first built
  frame that does not submit it: window closed, collapsed or skipped, a tab hidden, a
  clipped list row. Then the backend removes its layer.
- Lifetime counts built frames only: while no frame is built nothing is removed.
- Destroying the context removes every live widget.

## Positioning

The immediate pass computes each widget's rect, its clip (the window clip intersected with
the draw list's clip stack: scrolling, child windows, tables) and its z (the owning draw
list). `eli_frame_end` (right after `eli_window_render`) compares that with what the
backend last received and sends only the difference as `changed` bits. A frame where
nothing moved makes no backend call. Clipping is the backend's job (a partly scrolled-out
widget is cut, not hidden).

## Backend hooks

```c
typedef struct eli_live_widget {         /* read-only to backends, except `handle` */
    eli_live_kind kind;                  /* DOT, SPINNER, PROGRESS, TEXT */
    eli_id id;
    eli_rect rect, clip;                 /* screen points; clip may cut rect */
    bool visible;                        /* false: occluded or clipped out */
    eli_col32 col, track_col;
    eli_live_pulse pulse;                /* period 0 = solid */
    float rounding, fraction;
    const eli_font *font; float font_size;
    char text[ELI_LIVE_TEXT_MAX];
    void *handle;                        /* the backend's layer */
} eli_live_widget;

typedef struct eli_live_backend {
    void *user_data;
    unsigned kinds;                                       /* 1u << kind it can do */
    bool (*create)(void *user_data, eli_live_widget *w);  /* false = fall back */
    void (*update)(void *user_data, eli_live_widget *w, unsigned changed);
    void (*destroy)(void *user_data, eli_live_widget *w);
} eli_live_backend;
/* changed: ELI_LIVE_CHANGED_GEOMETRY | _VISIBLE | _STYLE | _VALUE | _TEXT */
```

All hooks run on the main thread, from `eli_frame_end` or from a setter. `create` gets a
fully filled widget; `update` only the bits that changed. Text backends rasterize with
`eli_live_text_raster()` (alpha8 coverage from the atlas pages at any scale; false when the
atlas has no CPU pixels).

## Memory and allocation

- A fixed pool: `ELI_LIVE_MAX` widgets (default 32, override before including elimgui) in
  static storage (`ELI_GLOBAL_ARRAY`, 5,888 bytes at the default), text inline. **No
  elimgui heap at all**, per frame or ever.
- Pool full: the extra widget is drawn by the fallback for that frame and counted in
  `eli_live_get_stats()->overflow`.
- Backends allocate platform objects on `create` and text bitmaps on a text change, never
  per frame. Nothing is allocated while a widget animates.

## Occlusion

At frame end each visible widget is tested against every draw list drawn after its own
(later windows, popups, menus, tooltips, the foreground list): their geometry bounds
(vertex bounding box within their clip rects). Any overlap hides the layer. A hidden
widget is drawn by the main surface instead, static (no animation, no timed redraw),
under whatever covers it. A visibility change requests one redraw so the next frame draws
or stops drawing the static copy. Content drawn later in the widget's own window does not
count: the layer is above its own window.

## Fallback

When there is no backend, the backend lacks the kind, `create` failed, the pool is full,
or text has no CPU atlas pixels, the widget draws itself into the window draw list every
frame (animation phase from `eli_get_time()`) and asks for
`eli_request_redraw_after(fallback_interval)`: a slow redraw (default 0.25 s = 4 fps).
Setters on a fallback widget call `eli_request_redraw()`.

## macOS backend (`backends/sdl3/eli_sdl3_live.h`)

```c
bool eli_sdl3_live_init(SDL_Renderer *renderer);  /* false (fallback) unless Metal */
void eli_sdl3_live_shutdown(void);
```

- Core Animation through the Objective-C runtime from C, like `eli_sdl3_metal.h`; no `.m`
  file. Link `-lobjc -framework QuartzCore -framework CoreGraphics`.
- One container layer (geometry flipped, points) added on top of the renderer's
  CAMetalLayer. Per widget a clip layer (`masksToBounds`) holding:
  - dot: a rounded `CALayer`, a `CABasicAnimation` on `opacity` (autoreverse, ease in-out,
    repeat forever);
  - spinner: a `CAShapeLayer` arc with a `transform.rotation.z` animation;
  - progress: track and fill layers; `set_fraction` changes the fill width;
  - text: a layer of the text color masked by an alpha8 image rastered from the atlas.
- Implicit animations are off for geometry and value changes.
- The CAMetalLayer is never redrawn for an animation, so the drawables stay released.
- `eli_sdl3_live_process_event` keeps the container sized to the window and re-rasters
  text on a display scale change.
- Known limit: during a layout change (scroll, window move) a layer can lead the main
  surface by one presented frame; they are committed separately.

### Measured

`tests/probe/live_footprint_probe.sh` (macOS 26, M-series, Metal, the SDL3 example at
1280x800 pt / 2560x1600 px, window visible, `--release-idle 1000`, footprint sampled at
8/12/16/20 s, median MiB, two passes agreed within 0.5):

| Run | Frames in 20 s | CPU | Graphics-owned (unmapped) | IOAccelerator (graphics) | Footprint |
|---|---|---|---|---|---|
| idle (nothing animates) | 0 | 0.012% | 2.0 | 1.2 | 69.0-69.2 |
| pulsing live dot, Core Animation layer (`--live-dot`) | 0 | 0.014% | 2.0 | 1.2 | 69.1-69.3 |
| same dot redrawn at 12.5 fps (`--live-fallback --live-fallback-interval 0.08`) | 240 | 4.2% | 54.0 | 4.1-4.5 | 127.9-128.5 |

The layer dot costs what idle costs. IOSurface read 46.9 MiB in all three runs (and with
the #9 binary, no live code): it does not separate them on this machine; the
graphics-owned block is what a redraw pulse keeps alive.

## Later backends

| Platform | Mechanism | Notes |
|---|---|---|
| Linux (Wayland) | one `wl_subsurface` per widget, `wp_viewporter`, `wp_alpha_modifier_v1` | No compositor-side animation: the client commits the small subsurface at the animation rate (opacity via alpha modifier, no new buffer). The main surface stays idle. |
| Windows | DirectComposition visuals over the swap chain | `IDCompositionAnimation` drives opacity/rotation in the compositor, like Core Animation. |
| Web | absolutely positioned DOM elements over the canvas | CSS animations; the canvas loop stays idle. |

Until then those builds use the fallback.

## Tests

| Suite / check | What |
|---|---|
| `test_live_frames` | A pulsing dot + spinner + progress + text with a backend: 0 main-surface frames in 30 s of animation, nothing drawn on the main surface; the fallback's 0.25 s redraw; setters without a frame |
| `test_live_allocs` | 0 elimgui allocations in 100 frames after 100 warm-up (text and fraction changing every frame, setters, fallback, overflow); a full pool leaves the heap byte-identical, under 8 MB |
| `test_live_lifetime` | Created once, removed when not submitted or collapsed, steady frames make no backend call, a move sends geometry only, per-window ids, overflow and refused create fall back, kind change, context destroy, text raster |
| `test_live_occlusion` | A window or the foreground list over the widget hides it and the main surface draws it static (no timed redraw); uncovered it comes back; clipping |
| `--live-selftest` (SDL3 example, macOS) | Real Core Animation layers: one tree per widget with its animation, drawn at the widget's place and upright (offscreen render of the layer tree, not a screen capture), no frame for 2.5 s while animating with the swapchain released, `set_fraction` without a frame, occlusion, removal, fallback after shutdown. Clean under ASan + UBSan |
| `tests/probe/red_proof_live.sh` | 9 mutations against the unit suites; all go red |
| `tests/probe/red_proof_live_sdl3.sh` | 5 mutations against `--live-selftest` (no pulse, wrong flip, visibility or value not applied, a layer widget asking for frames); all go red |
| `tests/probe/live_footprint_probe.sh` | The measurement above |

## Example

The SDL3 example's Editor pane has a "Live widgets" toggle: a pulsing dot, a spinner, the
background job's progress (`eli_live_set_fraction` from the worker's wake event) and a
clock ticked by an SDL timer (`eli_live_set_text`). None of them builds a frame; the
example's loop builds one only when elimgui asks.
