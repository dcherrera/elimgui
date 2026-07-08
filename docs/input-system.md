# Input System (Phase 4)

The input system tracks mouse, keyboard, and text input, derives per-frame state
(clicks, presses, drags, durations), and exposes shortcuts and clipboard access.
Backends feed raw events into a queue; elimgui drains and processes them once per
frame.

Include the whole category directly:

```c
#include <eli/input/eli_input.h>
```

or pull it in via the umbrella header once the library is wired (`<eli/elimgui.h>`).

## Data flow

```
backend callback ─► eli_io_add_*_event()  ─┐
                                           │  (buffered in io.input_events[])
eli_input_update_begin_frame() ────────────┘
    ├─ drains the event queue into live IO state
    ├─ derives mouse state (clicked/released/counts/drag/durations)
    └─ derives keyboard state (modifiers, per-key down durations)

... your UI reads eli_is_*/eli_get_* queries ...

eli_input_update_end_frame()
    └─ rolls mouse_pos_prev, clears wheel + text queue
```

All state lives in `eli_io` (see `eli/core/eli_io.h`) on the current context. The
input clock (`io.input_time`) advances by `io.delta_time` at each begin-frame and
drives click timing, so tests and hosts only need to set `io.delta_time`.

Configuration tunables are initialized lazily on the first begin-frame (or via
`eli_input_init_io`). Defaults match Dear ImGui: double-click time 0.30s, double-
click max distance 6px, drag threshold 6px, key repeat delay 0.275s, key repeat
rate 0.050s.

## Backend integration

| Function | Purpose |
|----------|---------|
| `void eli_io_add_mouse_pos_event(float x, float y)` | Queue a mouse move (top-left origin; use an offscreen value when the pointer leaves). |
| `void eli_io_add_mouse_button_event(int button, bool down)` | Queue a button press/release (`ELI_MOUSE_BUTTON_LEFT/RIGHT/MIDDLE`, ...). |
| `void eli_io_add_mouse_wheel_event(float wheel_x, float wheel_y)` | Queue a wheel delta (accumulated into the frame total). |
| `void eli_io_add_key_event(eli_key key, bool down)` | Queue a key press/release. |
| `void eli_io_add_key_analog_event(eli_key key, bool down, float value)` | Queue a key transition carrying an analog value in [0,1]. |
| `void eli_input_update_begin_frame(void)` | Drain events and derive per-frame state. Call once at frame start. |
| `void eli_input_update_end_frame(void)` | Roll per-frame accumulators. Call once at frame end. |
| `void eli_input_init_io(eli_io *io)` | Apply input defaults explicitly (optional; begin-frame does this lazily). |

## Mouse queries

| Function | Returns |
|----------|---------|
| `bool eli_is_mouse_down(button)` | Button currently held. |
| `bool eli_is_mouse_clicked(button)` | Button went down this frame. |
| `bool eli_is_mouse_released(button)` | Button went up this frame. |
| `bool eli_is_mouse_double_clicked(button)` | Second click of a double-click this frame. |
| `int  eli_get_mouse_clicked_count(button)` | Successive click count on the click frame (1, 2, ...), else 0. |
| `bool eli_is_any_mouse_down(void)` | Any button held. |
| `eli_vec2 eli_get_mouse_pos(void)` | Current mouse position. |
| `eli_vec2 eli_get_mouse_pos_on_opening_current_popup(void)` | Live position (popups pending a later phase). |
| `bool eli_is_mouse_pos_valid(const eli_vec2 *pos)` | Position is on-screen (`NULL` tests the current position). |
| `bool eli_is_mouse_hovering_rect(min, max, clip)` | Mouse inside a rect (min inclusive, max exclusive, expanded by touch padding). |
| `bool eli_is_mouse_dragging(button, lock_threshold)` | Held and dragged past the threshold (negative threshold = default). |
| `eli_vec2 eli_get_mouse_drag_delta(button, lock_threshold)` | Movement since the click origin, locked to (0,0) until the threshold is passed. |
| `void eli_reset_mouse_drag_delta(button)` | Reset the drag origin to the current position. |
| `eli_mouse_cursor eli_get_mouse_cursor(void)` | Requested cursor shape (resets to arrow each frame). |
| `void eli_set_mouse_cursor(cursor)` | Request a cursor shape for this frame. |
| `void eli_set_next_frame_want_capture_mouse(bool)` | Force `io.want_capture_mouse` next frame. |

## Keyboard queries

| Function | Returns |
|----------|---------|
| `bool eli_is_key_down(key)` | Key currently held. |
| `bool eli_is_key_pressed(key)` | Key went down this frame (no repeat). |
| `bool eli_is_key_pressed_ex(key, repeat)` | As above, optionally including typematic repeats. |
| `bool eli_is_key_released(key)` | Key went up this frame. |
| `bool eli_is_key_chord_pressed(chord)` | Modifier chord fired this frame (mods must match exactly). |
| `int  eli_get_key_pressed_amount(key, delay, rate)` | Presses (with repeats) this frame; negative delay/rate use IO defaults. |
| `const char *eli_get_key_name(key)` | Static human-readable key name. |
| `void eli_set_next_frame_want_capture_keyboard(bool)` | Force `io.want_capture_keyboard` next frame. |

Modifiers (`io.key_ctrl/shift/alt/super` and the `ELI_KEY_MOD_*` keys) are derived
each frame from the physical left/right modifier keys.

### Key chords

A chord is an `eli_key` OR'd with modifier flags:

```c
#define ELI_MOD_CTRL  (1 << 12)
#define ELI_MOD_SHIFT (1 << 13)
#define ELI_MOD_ALT   (1 << 14)
#define ELI_MOD_SUPER (1 << 15)
```

Named key values are all below `1<<12`, so key and modifier bits never overlap.

## Text input

| Function | Purpose |
|----------|---------|
| `void eli_io_add_input_character(unsigned int codepoint)` | Append one typed character (0 ignored; non-BMP stored as U+FFFD). |
| `void eli_io_add_input_characters_utf8(const char *utf8)` | Append a UTF-8 string, decoding each code point. |

Characters land in `io.input_queue_characters[]` (16-bit code units) and are cleared
at end-frame.

## Shortcuts and clipboard

| Function / macro | Purpose |
|------------------|---------|
| `bool eli_shortcut(chord)` | Whether a shortcut chord fired this frame (routing/focus deferred; currently the raw chord state). |
| `ELI_SHORTCUT_COPY/CUT/PASTE/UNDO/REDO/SELECT_ALL` | Predefined Ctrl-based edit chords. |
| `const char *eli_get_clipboard_text(void)` | Current clipboard text (never NULL). |
| `void eli_set_clipboard_text(const char *text)` | Set clipboard text (NULL clears). |
| `void eli_set_clipboard_callbacks(get_fn, set_fn, user_data)` | Install custom clipboard handlers. |

Clipboard resolution: IO callbacks take priority; otherwise an internal static
buffer is used. In the browser build (`ELI_JSIO`), `eli_set_clipboard_text` also
forwards to the host page (`eli_host_set_clipboard`), and the host delivers OS
clipboard text back via the exported `eli_host_provide_clipboard`. Hosted unit
builds have `ELI_JSIO` undefined and use the static buffer only.

## Example

```c
#include <eli/input/eli_input.h>

void frame(eli_context *ctx, float dt) {
    ctx->io.delta_time = dt;

    /* Backend pushes raw events (e.g. from browser listeners). */
    eli_io_add_mouse_pos_event(120.0f, 80.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_io_add_key_event(ELI_KEY_LEFT_CTRL, true);
    eli_io_add_key_event(ELI_KEY_C, true);

    eli_input_update_begin_frame();

    if (eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT)) { /* handle click */ }
    if (eli_shortcut(ELI_SHORTCUT_COPY)) {
        eli_set_clipboard_text("copied!");
    }

    eli_input_update_end_frame();
}
```

## Notes / deferred

- `eli_shortcut` does not yet perform routing or focus arbitration — that needs the
  window/focus system (later phases). It currently reports the raw chord state.
- `eli_get_mouse_pos_on_opening_current_popup` returns the live mouse position
  until popups exist.
- The `clip` argument of `eli_is_mouse_hovering_rect` is accepted for API parity;
  window-clip integration lands with the window phase.
