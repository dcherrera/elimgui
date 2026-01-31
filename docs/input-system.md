# Input System

This document covers mouse input, keyboard input, text input, shortcuts, and clipboard handling in elimgui.

## Table of Contents

- [Overview](#overview)
- [Mouse Input](#mouse-input)
  - [Position](#position)
  - [Buttons](#buttons)
  - [Hovering](#hovering)
  - [Dragging](#dragging)
  - [Cursor](#cursor)
- [Keyboard Input](#keyboard-input)
  - [Key State](#key-state)
  - [Modifiers](#modifiers)
  - [Key Names](#key-names)
- [Text Input](#text-input)
- [Shortcuts](#shortcuts)
- [Clipboard](#clipboard)
- [Backend Integration](#backend-integration)

---

## Overview

The input system provides:
- **Mouse queries** - Position, buttons, hovering, dragging, cursor
- **Keyboard queries** - Key state, modifiers, key names
- **Text input** - Unicode character input queue
- **Shortcuts** - Key chord detection (Ctrl+C, etc.)
- **Clipboard** - Copy/paste via callbacks

**Data flow:**
```
Platform Events → eli_io_add_*_event() → eli_io state → eli_is_*() queries → Widgets
```

---

## Mouse Input

### Position

```c
// Get current mouse position
eli_vec2 pos = eli_get_mouse_pos();

// Check if mouse position is valid
if (eli_is_mouse_pos_valid(NULL)) {
    // Mouse is within window
}

// Get mouse position when popup was opened (for context menus)
eli_vec2 popup_pos = eli_get_mouse_pos_on_opening_current_popup();
```

### Buttons

```c
// Is button currently held down?
if (eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT)) { ... }

// Was button clicked this frame?
if (eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT, false)) { ... }

// With repeat (for scrolling, etc.)
if (eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT, true)) { ... }

// Was button released this frame?
if (eli_is_mouse_released(ELI_MOUSE_BUTTON_LEFT)) { ... }

// Was button double-clicked?
if (eli_is_mouse_double_clicked(ELI_MOUSE_BUTTON_LEFT)) { ... }

// Get click count (for triple-click detection)
int clicks = eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT);

// Is any mouse button down?
if (eli_is_any_mouse_down()) { ... }
```

**Mouse Buttons:**
| Constant | Value | Description |
|----------|-------|-------------|
| `ELI_MOUSE_BUTTON_LEFT` | 0 | Left button |
| `ELI_MOUSE_BUTTON_RIGHT` | 1 | Right button |
| `ELI_MOUSE_BUTTON_MIDDLE` | 2 | Middle button |

### Hovering

```c
// Check if mouse is hovering a rect (screen coordinates)
eli_vec2 min = eli_make_vec2(100, 100);
eli_vec2 max = eli_make_vec2(200, 150);

if (eli_is_mouse_hovering_rect(min, max, false)) {
    // Mouse is over the rect
}
```

### Dragging

```c
// Is user dragging with left button?
if (eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, -1.0f)) {
    // Get drag delta since button was pressed
    eli_vec2 delta = eli_get_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT, -1.0f);

    // Use delta for dragging...

    // Reset delta when done
    eli_reset_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT);
}
```

The `lock_threshold` parameter (-1.0f = use default) is the minimum distance before dragging is considered active.

### Cursor

```c
// Get current cursor type
eli_mouse_cursor cursor = eli_get_mouse_cursor();

// Set cursor type (for next frame)
eli_set_mouse_cursor(ELI_MOUSE_CURSOR_RESIZE_EW);

// Request mouse capture
eli_set_next_frame_want_capture_mouse(true);
```

**Cursor Types:**
| Constant | Description |
|----------|-------------|
| `ELI_MOUSE_CURSOR_NONE` | No cursor |
| `ELI_MOUSE_CURSOR_ARROW` | Default arrow |
| `ELI_MOUSE_CURSOR_TEXT_INPUT` | I-beam for text |
| `ELI_MOUSE_CURSOR_RESIZE_ALL` | Four-way resize |
| `ELI_MOUSE_CURSOR_RESIZE_NS` | North-south resize |
| `ELI_MOUSE_CURSOR_RESIZE_EW` | East-west resize |
| `ELI_MOUSE_CURSOR_RESIZE_NESW` | Diagonal NE-SW |
| `ELI_MOUSE_CURSOR_RESIZE_NWSE` | Diagonal NW-SE |
| `ELI_MOUSE_CURSOR_HAND` | Hand pointer |
| `ELI_MOUSE_CURSOR_NOT_ALLOWED` | Not allowed |

---

## Keyboard Input

### Key State

```c
// Is key currently held down?
if (eli_is_key_down(ELI_KEY_SPACE)) { ... }

// Was key pressed this frame?
if (eli_is_key_pressed(ELI_KEY_ENTER, false)) { ... }

// With repeat
if (eli_is_key_pressed(ELI_KEY_BACKSPACE, true)) { ... }

// Was key released this frame?
if (eli_is_key_released(ELI_KEY_ESCAPE)) { ... }

// Get key hold duration (seconds, or -1 if not held)
float duration = eli_get_key_pressed_amount(ELI_KEY_A);
```

### Modifiers

```c
// Check modifier state
eli_io* io = eli_get_io();
if (io->key_ctrl) { ... }   // Ctrl is down
if (io->key_shift) { ... }  // Shift is down
if (io->key_alt) { ... }    // Alt is down
if (io->key_super) { ... }  // Super/Cmd is down

// Check specific modifier combination
if (eli_is_key_mod_down(ELI_MOD_CTRL | ELI_MOD_SHIFT)) {
    // Ctrl+Shift held
}
```

**Modifier Constants:**
| Constant | Description |
|----------|-------------|
| `ELI_MOD_NONE` | No modifiers |
| `ELI_MOD_CTRL` | Ctrl key |
| `ELI_MOD_SHIFT` | Shift key |
| `ELI_MOD_ALT` | Alt key |
| `ELI_MOD_SUPER` | Super/Cmd/Windows key |

### Key Names

```c
// Get human-readable key name
const char* name = eli_get_key_name(ELI_KEY_ENTER);  // "Enter"

// Request keyboard capture
eli_set_next_frame_want_capture_keyboard(true);
```

---

## Text Input

Text input is handled through a character queue that the platform backend fills:

```c
// Platform backend adds characters
eli_io_add_input_character('H');
eli_io_add_input_character('i');

// Or add UTF-8 string
eli_io_add_input_characters_utf8("Hello!");

// Read input queue in widgets
eli_io* io = eli_get_io();
for (int i = 0; i < io->input_queue_chars_count; i++) {
    uint32_t c = io->input_queue_chars[i];
    // Process character...
}
```

The input queue is automatically cleared at the end of each frame.

---

## Shortcuts

```c
// Check for key chord (key + modifiers)
if (eli_shortcut(ELI_KEY_S | ELI_MOD_CTRL, false)) {
    // Ctrl+S pressed
}

// With repeat
if (eli_shortcut(ELI_KEY_Z | ELI_MOD_CTRL, true)) {
    // Ctrl+Z pressed (repeating)
}

// Predefined shortcuts
if (eli_shortcut(ELI_SHORTCUT_COPY, false)) { ... }   // Ctrl+C
if (eli_shortcut(ELI_SHORTCUT_PASTE, false)) { ... }  // Ctrl+V
if (eli_shortcut(ELI_SHORTCUT_CUT, false)) { ... }    // Ctrl+X
if (eli_shortcut(ELI_SHORTCUT_UNDO, false)) { ... }   // Ctrl+Z
if (eli_shortcut(ELI_SHORTCUT_REDO, false)) { ... }   // Ctrl+Y
if (eli_shortcut(ELI_SHORTCUT_SELECT_ALL, false)) { ... } // Ctrl+A
```

---

## Clipboard

Clipboard requires platform-specific callbacks:

```c
// Set up clipboard callbacks
const char* my_get_clipboard(void* user_data) {
    // Return clipboard text from platform
    return platform_get_clipboard();
}

void my_set_clipboard(void* user_data, const char* text) {
    // Set clipboard text via platform
    platform_set_clipboard(text);
}

eli_set_clipboard_callbacks(my_get_clipboard, my_set_clipboard, NULL);

// Use clipboard
const char* text = eli_get_clipboard_text();
eli_set_clipboard_text("Hello clipboard!");
```

---

## Backend Integration

The platform backend must feed input events to elimgui each frame:

```c
// At start of frame, before eli_new_frame()

// Mouse position
eli_io_add_mouse_pos_event(mouse_x, mouse_y);

// Mouse buttons
eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, left_button_down);
eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_RIGHT, right_button_down);

// Mouse wheel
eli_io_add_mouse_wheel_event(wheel_x, wheel_y);

// Keyboard
eli_io_add_key_event(ELI_KEY_A, a_key_down);
eli_io_add_key_event(ELI_KEY_ENTER, enter_key_down);
// ... etc

// Text input
eli_io_add_input_character(typed_char);
```

### Complete Frame Loop

```c
void frame() {
    // 1. Process platform events, call eli_io_add_*_event()

    // 2. Update IO state
    eli_io* io = eli_get_io();
    io->display_size_x = window_width;
    io->display_size_y = window_height;
    io->delta_time = frame_time;

    // 3. Start frame
    eli_new_frame();

    // 4. Build UI (widgets use eli_is_*() queries internally)
    // ...

    // 5. Render
    eli_render();

    // 6. Draw via renderer
    // ...
}
```

---

## Timing Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `ELI_MOUSE_DOUBLE_CLICK_TIME` | 0.30s | Max time between double-click |
| `ELI_MOUSE_DOUBLE_CLICK_DIST` | 6px | Max distance for double-click |
| `ELI_MOUSE_DRAG_THRESHOLD` | 6px | Min distance to start drag |
| `ELI_KEY_REPEAT_DELAY` | 0.275s | Initial delay before repeat |
| `ELI_KEY_REPEAT_RATE` | 0.050s | Repeat rate once started |

---

## See Also

- [Core Types & Context](core-types.md)
- [Draw System](draw-system.md)
- [Font System](font-system.md)
