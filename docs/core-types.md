# Core Types & Context

This document covers the fundamental types, context management, and frame lifecycle in elimgui.

## Table of Contents

- [Core Types](#core-types)
  - [eli_vec2](#eli_vec2)
  - [eli_vec4](#eli_vec4)
  - [eli_rect](#eli_rect)
  - [eli_id](#eli_id)
- [Color System](#color-system)
  - [ELI_COL32 Macro](#eli_col32-macro)
  - [Color Conversion](#color-conversion)
- [Context Management](#context-management)
  - [eli_context](#eli_context)
  - [Creating & Destroying](#creating--destroying)
  - [Current Context](#current-context)
- [IO Structure](#io-structure)
- [Style Structure](#style-structure)
- [Frame Lifecycle](#frame-lifecycle)
- [Enums Reference](#enums-reference)

---

## Core Types

### eli_vec2

A 2D vector used for positions, sizes, and offsets.

```c
typedef struct eli_vec2 {
    float x, y;
} eli_vec2;
```

**Helper function:**
```c
eli_vec2 eli_make_vec2(float x, float y);
```

**Usage:**
```c
eli_vec2 pos = eli_make_vec2(100.0f, 200.0f);
eli_vec2 size = { 400.0f, 300.0f };  // Direct initialization
```

---

### eli_vec4

A 4D vector used for colors (RGBA) and other 4-component data.

```c
typedef struct eli_vec4 {
    float x, y, z, w;
} eli_vec4;
```

**Helper function:**
```c
eli_vec4 eli_make_vec4(float x, float y, float z, float w);
```

**Usage for colors:**
```c
eli_vec4 red = eli_make_vec4(1.0f, 0.0f, 0.0f, 1.0f);  // RGBA
eli_vec4 semi_transparent = { 0.5f, 0.5f, 0.5f, 0.5f };
```

---

### eli_rect

An axis-aligned bounding box defined by min and max points.

```c
typedef struct eli_rect {
    eli_vec2 min;  // Top-left corner
    eli_vec2 max;  // Bottom-right corner
} eli_rect;
```

**Helper functions:**
```c
eli_rect eli_make_rect(float min_x, float min_y, float max_x, float max_y);
bool eli_rect_contains(eli_rect r, eli_vec2 p);
float eli_rect_width(eli_rect r);
float eli_rect_height(eli_rect r);
eli_vec2 eli_rect_size(eli_rect r);
```

**Usage:**
```c
eli_rect bounds = eli_make_rect(10.0f, 10.0f, 110.0f, 60.0f);
eli_vec2 mouse = eli_make_vec2(50.0f, 30.0f);

if (eli_rect_contains(bounds, mouse)) {
    // Mouse is inside the rectangle
}

float w = eli_rect_width(bounds);   // 100.0f
float h = eli_rect_height(bounds);  // 50.0f
```

---

### eli_id

A unique identifier for widgets, used for state tracking.

```c
typedef uint32_t eli_id;
```

IDs are generated from strings using a hash function. The `##` and `###` separators work like Dear ImGui:

- `"Label##unique"` - Different ID, same display text
- `"Label###stable"` - Stable ID across label changes

---

## Color System

### ELI_COL32 Macro

Creates a 32-bit packed color from RGBA components (0-255).

```c
#define ELI_COL32(R, G, B, A) ...
```

**Predefined colors:**
```c
ELI_COL32_WHITE       // (255, 255, 255, 255)
ELI_COL32_BLACK       // (0, 0, 0, 255)
ELI_COL32_BLACK_TRANS // (0, 0, 0, 0)
```

**Usage:**
```c
uint32_t red = ELI_COL32(255, 0, 0, 255);
uint32_t semi_blue = ELI_COL32(0, 0, 255, 128);
```

**Color channel shifts:**
```c
ELI_COL32_R_SHIFT  // 0
ELI_COL32_G_SHIFT  // 8
ELI_COL32_B_SHIFT  // 16
ELI_COL32_A_SHIFT  // 24
ELI_COL32_A_MASK   // 0xFF000000
```

### Color Conversion

Convert between packed u32 and float vec4 formats:

```c
eli_vec4 eli_color_u32_to_vec4(uint32_t col);
uint32_t eli_color_vec4_to_u32(eli_vec4 col);
```

**Usage:**
```c
uint32_t packed = ELI_COL32(255, 128, 64, 255);
eli_vec4 floats = eli_color_u32_to_vec4(packed);
// floats = { 1.0f, 0.5f, 0.25f, 1.0f }

eli_vec4 color = eli_make_vec4(0.0f, 1.0f, 0.0f, 1.0f);
uint32_t green = eli_color_vec4_to_u32(color);
// green = ELI_COL32(0, 255, 0, 255)
```

---

## Context Management

### eli_context

The main context structure holds all GUI state:

```c
struct eli_context {
    eli_io io;           // Input/output configuration
    eli_style style;     // Visual styling
    eli_font* font;      // Current font
    float font_size;     // Current font size

    int frame_count;     // Total frames rendered
    bool within_frame_scope;  // Between new_frame/end_frame

    eli_id active_id;    // Currently active widget
    eli_id hot_id;       // Currently hovered widget

    eli_draw_data* draw_data;  // Render output
    eli_draw_list* draw_list;  // Current draw list

    bool initialized;    // Context is valid
};
```

### Creating & Destroying

```c
eli_context* eli_create_context(void);
void eli_destroy_context(eli_context* ctx);
```

**Usage:**
```c
// At startup
eli_context* ctx = eli_create_context();
eli_set_current_context(ctx);

// Configure
eli_io* io = eli_get_io();
io->display_size_x = 1280.0f;
io->display_size_y = 720.0f;

// ... application loop ...

// At shutdown
eli_destroy_context(ctx);
```

### Current Context

elimgui uses a global current context:

```c
eli_context* eli_get_current_context(void);
void eli_set_current_context(eli_context* ctx);
eli_io* eli_get_io(void);
eli_style* eli_get_style(void);
```

Most functions operate on the current context. You must set it before calling other functions.

---

## IO Structure

The `eli_io` structure handles input and output configuration:

```c
struct eli_io {
    // Display configuration
    float display_size_x, display_size_y;  // Canvas size in pixels
    float delta_time;                       // Time since last frame

    // Font configuration
    float font_global_scale;    // Global font scaling
    eli_font* font_default;     // Default font

    // Mouse input (set by application)
    eli_vec2 mouse_pos;         // Current mouse position
    bool mouse_down[5];         // Mouse button states
    float mouse_wheel;          // Vertical scroll
    float mouse_wheel_h;        // Horizontal scroll

    // Keyboard modifiers (set by application)
    bool key_ctrl;
    bool key_shift;
    bool key_alt;
    bool key_super;

    // Key states
    eli_key_data keys_data[154];  // Per-key state

    // Text input queue
    uint32_t input_queue_chars[16];
    int input_queue_chars_count;

    // Output flags (set by elimgui)
    bool want_capture_mouse;     // GUI wants mouse input
    bool want_capture_keyboard;  // GUI wants keyboard input
    bool want_text_input;        // Text input field active
    eli_mouse_cursor mouse_cursor;  // Desired cursor shape

    // Metrics
    float framerate;
    int metrics_render_vertices;
    int metrics_render_indices;
};
```

**Typical input handling:**
```c
eli_io* io = eli_get_io();

// Set display size
io->display_size_x = canvas_width;
io->display_size_y = canvas_height;

// Set timing
io->delta_time = 1.0f / 60.0f;  // Or actual delta

// Set mouse state (from JS events)
io->mouse_pos.x = mouse_x;
io->mouse_pos.y = mouse_y;
io->mouse_down[0] = left_button_pressed;
io->mouse_down[1] = right_button_pressed;
io->mouse_wheel = scroll_delta;
```

---

## Style Structure

The `eli_style` structure controls visual appearance:

```c
struct eli_style {
    // Opacity
    float alpha;           // Global alpha (1.0 = opaque)
    float disabled_alpha;  // Alpha for disabled items

    // Sizing
    eli_vec2 window_padding;    // Padding inside windows
    eli_vec2 frame_padding;     // Padding inside framed items
    eli_vec2 item_spacing;      // Spacing between items
    eli_vec2 item_inner_spacing;

    // Rounding
    float window_rounding;
    float frame_rounding;
    float popup_rounding;
    float scrollbar_rounding;
    float grab_rounding;
    float tab_rounding;

    // Borders
    float window_border_size;
    float frame_border_size;
    float popup_border_size;

    // Scrollbars
    float scrollbar_size;
    float grab_min_size;

    // Colors (60 color slots)
    eli_vec4 colors[ELI_COL_COUNT];

    // Anti-aliasing
    bool anti_aliased_lines;
    bool anti_aliased_fill;
    float curve_tessellation_tol;
};
```

**Accessing and modifying:**
```c
eli_style* style = eli_get_style();

// Modify sizing
style->window_padding.x = 10.0f;
style->window_padding.y = 10.0f;
style->frame_rounding = 4.0f;

// Modify colors
style->colors[ELI_COL_WINDOW_BG] = eli_make_vec4(0.1f, 0.1f, 0.1f, 1.0f);
style->colors[ELI_COL_BUTTON] = eli_make_vec4(0.2f, 0.4f, 0.8f, 1.0f);
```

---

## Frame Lifecycle

Every frame follows this pattern:

```c
// 1. Start new frame
eli_new_frame();

// 2. Submit UI
if (eli_begin("My Window", NULL, ELI_WINDOW_FLAGS_NONE)) {
    eli_text("Hello, World!");
    if (eli_button("Click Me")) {
        // Handle click
    }
}
eli_end();

// 3. Finalize and render
eli_render();

// 4. Get draw data for your renderer
eli_draw_data* draw_data = eli_get_draw_data();
my_renderer_draw(draw_data);
```

### Frame Functions

```c
void eli_new_frame(void);
```
Starts a new frame. Call once at the beginning of each frame after updating IO.

```c
void eli_end_frame(void);
```
Ends the current frame. Called automatically by `eli_render()` if needed.

```c
void eli_render(void);
```
Finalizes the frame and generates draw data.

```c
eli_draw_data* eli_get_draw_data(void);
```
Returns the draw data for rendering. Valid until the next `eli_new_frame()`.

---

## Enums Reference

### Direction

```c
typedef enum eli_dir {
    ELI_DIR_NONE  = -1,
    ELI_DIR_LEFT  = 0,
    ELI_DIR_RIGHT = 1,
    ELI_DIR_UP    = 2,
    ELI_DIR_DOWN  = 3,
    ELI_DIR_COUNT
} eli_dir;
```

### Condition

Used for conditional operations (set position, size, etc.):

```c
typedef enum eli_cond {
    ELI_COND_NONE           = 0,
    ELI_COND_ALWAYS         = 1 << 0,  // Always apply
    ELI_COND_ONCE           = 1 << 1,  // Apply once, then ignore
    ELI_COND_FIRST_USE_EVER = 1 << 2,  // Apply if never used before
    ELI_COND_APPEARING      = 1 << 3   // Apply when appearing
} eli_cond;
```

### Data Type

For generic scalar widgets:

```c
typedef enum eli_data_type {
    ELI_DATA_TYPE_S8,      // int8_t
    ELI_DATA_TYPE_U8,      // uint8_t
    ELI_DATA_TYPE_S16,     // int16_t
    ELI_DATA_TYPE_U16,     // uint16_t
    ELI_DATA_TYPE_S32,     // int32_t
    ELI_DATA_TYPE_U32,     // uint32_t
    ELI_DATA_TYPE_S64,     // int64_t
    ELI_DATA_TYPE_U64,     // uint64_t
    ELI_DATA_TYPE_FLOAT,   // float
    ELI_DATA_TYPE_DOUBLE,  // double
    ELI_DATA_TYPE_BOOL,    // bool
    ELI_DATA_TYPE_STRING,  // char*
    ELI_DATA_TYPE_COUNT
} eli_data_type;
```

### Window Flags

```c
ELI_WINDOW_FLAGS_NONE                    // Default
ELI_WINDOW_FLAGS_NO_TITLE_BAR            // No title bar
ELI_WINDOW_FLAGS_NO_RESIZE               // Not resizable
ELI_WINDOW_FLAGS_NO_MOVE                 // Not movable
ELI_WINDOW_FLAGS_NO_SCROLLBAR            // No scrollbars
ELI_WINDOW_FLAGS_NO_COLLAPSE             // No collapse button
ELI_WINDOW_FLAGS_ALWAYS_AUTO_RESIZE      // Auto-resize to content
ELI_WINDOW_FLAGS_NO_BACKGROUND           // No background
ELI_WINDOW_FLAGS_MENU_BAR                // Has menu bar
ELI_WINDOW_FLAGS_HORIZONTAL_SCROLLBAR    // Allow horizontal scroll

// Composites
ELI_WINDOW_FLAGS_NO_NAV        // No navigation
ELI_WINDOW_FLAGS_NO_DECORATION // No title, resize, scroll, collapse
ELI_WINDOW_FLAGS_NO_INPUTS     // No mouse or nav input
```

### Mouse Buttons

```c
ELI_MOUSE_BUTTON_LEFT   = 0
ELI_MOUSE_BUTTON_RIGHT  = 1
ELI_MOUSE_BUTTON_MIDDLE = 2
ELI_MOUSE_BUTTON_COUNT  = 5
```

### Mouse Cursors

```c
ELI_MOUSE_CURSOR_NONE        = -1  // No cursor
ELI_MOUSE_CURSOR_ARROW       = 0   // Default arrow
ELI_MOUSE_CURSOR_TEXT_INPUT        // Text input I-beam
ELI_MOUSE_CURSOR_RESIZE_ALL        // Move/resize all directions
ELI_MOUSE_CURSOR_RESIZE_NS         // Vertical resize
ELI_MOUSE_CURSOR_RESIZE_EW         // Horizontal resize
ELI_MOUSE_CURSOR_RESIZE_NESW       // Diagonal resize
ELI_MOUSE_CURSOR_RESIZE_NWSE       // Diagonal resize
ELI_MOUSE_CURSOR_HAND              // Hand/pointer
ELI_MOUSE_CURSOR_NOT_ALLOWED       // Disabled/forbidden
```

### Color Indices

Used to index into `style->colors[]`:

```c
ELI_COL_TEXT                  // Text color
ELI_COL_TEXT_DISABLED         // Disabled text
ELI_COL_WINDOW_BG             // Window background
ELI_COL_CHILD_BG              // Child window background
ELI_COL_POPUP_BG              // Popup background
ELI_COL_BORDER                // Border color
ELI_COL_FRAME_BG              // Frame background (input, checkbox)
ELI_COL_FRAME_BG_HOVERED      // Frame background (hovered)
ELI_COL_FRAME_BG_ACTIVE       // Frame background (active)
ELI_COL_TITLE_BG              // Title bar background
ELI_COL_TITLE_BG_ACTIVE       // Title bar (focused)
ELI_COL_BUTTON                // Button background
ELI_COL_BUTTON_HOVERED        // Button (hovered)
ELI_COL_BUTTON_ACTIVE         // Button (pressed)
ELI_COL_HEADER                // Header (collapsing, tree)
ELI_COL_HEADER_HOVERED
ELI_COL_HEADER_ACTIVE
ELI_COL_SCROLLBAR_BG          // Scrollbar background
ELI_COL_SCROLLBAR_GRAB        // Scrollbar grab
ELI_COL_CHECK_MARK            // Checkbox check mark
ELI_COL_SLIDER_GRAB           // Slider grab
ELI_COL_TAB                   // Tab background
ELI_COL_TAB_SELECTED          // Selected tab
// ... and 40+ more (see eli_col enum)
ELI_COL_COUNT                 // Total color count (60)
```

---

## See Also

- [API Quick Reference](README.md#api-quick-reference)
- [Dear ImGui Migration](README.md#dear-imgui-migration)
