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
                    eli_io_add_input_*()   ─┤  (ordered, in io.input_events[], heap, grows)
eli_input_update_begin_frame() ────────────┘
    ├─ applies queued events in order (key/text trickle, see below)
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

The event queue starts at `ELI_INPUT_EVENT_QUEUE_SIZE` (256) and grows on the heap
(`eli_mem`); nothing is dropped unless allocation fails. A mouse-position event that
directly follows another one replaces it (coalescing), so fast motion cannot crowd
out buttons and keys.

**Trickle rule** (`io.config_input_trickle_disabled = false`, the default): within one
frame, text and key events are not mixed. Widgets read the frame's characters and
key presses as two separate sets, so "a", Backspace, "b" arriving together would
otherwise apply out of order. The first key-after-text or text-after-key event and
everything after it stay queued for the next frame, which is requested via the redraw
API.

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
#define ELI_MOD_SHORTCUT (1 << 11)  /* Cmd on macOS, Ctrl elsewhere (resolved when tested) */
#define ELI_MOD_CTRL     (1 << 12)
#define ELI_MOD_SHIFT    (1 << 13)
#define ELI_MOD_ALT      (1 << 14)
#define ELI_MOD_SUPER    (1 << 15)
```

Named key values are all below `1<<11`, so key and modifier bits never overlap.

**Cmd vs Ctrl.** `io.config_mac_os_behaviors` (default true when built for
`__APPLE__`; the SDL3 backend sets it per platform; the web glue sets it from the
browser platform) makes `ELI_MOD_SHORTCUT` mean Super (Cmd), else Ctrl. Use
`ELI_MOD_SHORTCUT` for app shortcuts; keep `ELI_MOD_CTRL` where a real Ctrl is meant (a
terminal view sending Ctrl+C). `eli_is_shortcut_mod_down(io)` tests the held modifier.

## Text input

| Function | Purpose |
|----------|---------|
| `void eli_io_add_input_character(unsigned int codepoint)` | Queue one typed code point as a TEXT event (0 ignored; surrogates and > U+10FFFF become U+FFFD). |
| `void eli_io_add_input_character_utf16(uint16_t unit)` | Queue a UTF-16 code unit; surrogate pairs are joined, unpaired ones become U+FFFD. |
| `void eli_io_add_input_characters_utf8(const char *utf8)` | Queue a UTF-8 string, one TEXT event per code point, no length limit. |

TEXT events are applied in order with keys and mouse at begin-frame into
`io.input_queue_characters` (`uint32_t`, heap, grows; initial capacity
`ELI_INPUT_QUEUE_CHAR_SIZE`) and cleared at end-frame. Emoji and other non-BMP
characters are preserved; `input_text` stores them as 4-byte UTF-8 (the font may still
draw them as the fallback glyph). Characters added during a frame apply next frame.

## Shortcuts and clipboard

| Function / macro | Purpose |
|------------------|---------|
| `bool eli_shortcut(chord)` | Chord fired this frame on the FOCUSED route (see Routing). |
| `bool eli_shortcut_ex(chord, flags)` | Chord through a route: `ELI_INPUT_ROUTE_FOCUSED` / `_GLOBAL` / `_ACTIVE`, optional `ELI_INPUT_REPEAT`. |
| `void eli_set_key_owner(eli_key, eli_id)` / `eli_get_key_owner(eli_key)` | Claim a key for an item (per frame; effective next frame). |
| `ELI_SHORTCUT_COPY/CUT/PASTE/UNDO/SELECT_ALL` | Edit chords on `ELI_MOD_SHORTCUT`. |
| `ELI_SHORTCUT_REDO` / `_REDO_ALT`, `eli_is_redo_chord_pressed()` | Shortcut+Shift+Z, plus Ctrl+Y off macOS. |
| `const char *eli_get_clipboard_text(void)` | Current clipboard text (never NULL). |
| `void eli_set_clipboard_text(const char *text)` | Set clipboard text (NULL clears). |
| `void eli_set_clipboard_callbacks(get_fn, set_fn, user_data)` | Install custom clipboard handlers. |

### Clipboard

IO callbacks take priority (the SDL3 backend installs them: `SDL_GetClipboardText`
cached and `SDL_free`d on the next get); otherwise an internal heap buffer is used that
grows to fit (no cap). In the browser build (`ELI_JSIO`), `eli_set_clipboard_text`
also forwards to the host page (`eli_host_set_clipboard`), and the host delivers OS
clipboard text back via the exported `eli_host_provide_clipboard`.

`input_text` copy/cut go through a heap temporary and paste decodes UTF-8 and inserts
in one step, so a 64 KB copy or paste is whole. Undo and Escape-revert snapshots are
still capped at `ELI_INPUT_TEXT_SNAPSHOT_MAX` (256): editing a longer buffer disables
them for that edit instead of restoring a truncated copy.

### Routing

| Route | Fires when |
|---|---|
| `ELI_INPUT_ROUTE_FOCUSED` (default, `eli_shortcut`) | the calling window is the focused window or in its tree; outside any window it acts like GLOBAL |
| `ELI_INPUT_ROUTE_GLOBAL` | anywhere, unless an active item owns the key |
| `ELI_INPUT_ROUTE_ACTIVE` | only for the active item (call right after the item) |

An active `input_text` claims its edit keys every frame: arrows, Home/End,
Backspace/Delete, Enter, Escape, Tab (with `ALLOW_TAB_INPUT`) and A/C/V/X/Y/Z (the edit
chords). So the app's `Shortcut+C` yields while typing, but `Shortcut+S` still fires.
Apps with their own keymap use GLOBAL and gate plain keys on `io.want_capture_keyboard`.

### Capture outputs

Computed at `eli_frame_end` (next-frame overrides still win):

| Field | Meaning |
|---|---|
| `io.want_capture_mouse` | no button held and the mouse is over an elimgui window, or a button is held that was pressed over one (a drag started in the app stays the app's) |
| `io.want_capture_keyboard` | an item is active |
| `io.want_text_input` | a text widget is active (backends start/stop IME/keyboards from it) |

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

- Keyboard navigation does not exist yet, so `want_capture_keyboard` is "an item is
  active" only.
- `eli_get_mouse_pos_on_opening_current_popup` returns the live mouse position
  until popups exist.
- The `clip` argument of `eli_is_mouse_hovering_rect` is accepted for API parity;
  window-clip integration lands with the window phase.
