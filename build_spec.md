# elimgui - Build Specification

## Overview

**elimgui** is a pure C99 rewrite of Dear ImGui, designed for WebAssembly targets. It provides an immediate-mode GUI API familiar to ImGui users while being fully compatible with JAClibc and the Canvil framework.

This is NOT a binding or wrapper - it's a ground-up reimplementation in C.

## Goals

1. **Full feature parity with Dear ImGui** - All widgets, features, and capabilities
2. **ImGui-familiar API** - Developers who know ImGui should feel at home
3. **Pure C99** - No C++ features, compiles with clang to wasm32
4. **Header-only** - Single include, no separate compilation units
5. **Zero dependencies** - Only JAClibc (for WASM) and optionally stb_truetype
6. **Minimal footprint** - Small WASM binary size
7. **Renderer-agnostic** - Outputs draw lists, not pixels

## Non-Goals

- Native platform support (WASM-first, native is bonus)
- Backwards compatibility with ImGui code (similar API, not identical)

## Architecture

### Core Components

```
eli_context     → Global state container
eli_io          → Input/output (mouse, keyboard, display)
eli_style       → Colors, sizes, spacing
eli_draw_list   → Accumulated draw commands
eli_draw_data   → Final render output
```

### Header Structure

| Header | Purpose |
|--------|---------|
| `elimgui.h` | Master include, core types, context management |
| `eli_draw.h` | Draw primitives (rect, line, circle, text, etc.) |
| `eli_widgets.h` | All widgets (button, slider, checkbox, etc.) |
| `eli_layout.h` | Layout system (same-line, columns, spacing) |
| `eli_input.h` | Input handling (mouse, keyboard, focus) |
| `eli_font.h` | Font loading and text rendering |
| `eli_style.h` | Styling and theming |
| `eli_tables.h` | Table widget |
| `eli_docking.h` | Window docking (future) |

### Data Flow

```
User code
    ↓
eli_new_frame()          # Start frame, process input
    ↓
eli_begin() / eli_end()  # Windows
eli_button(), etc.       # Widgets
    ↓
eli_render()             # Finalize draw data
    ↓
eli_draw_data            # Vertex buffers + draw commands
    ↓
Your renderer            # WebGL, WebGPU, etc.
```

## Core Types

### Vectors and Rects

```c
typedef struct { float x, y; } eli_vec2;
typedef struct { float x, y, z, w; } eli_vec4;
typedef struct { float x, y, w, h; } eli_rect;
```

### Colors

```c
// 32-bit RGBA (0xRRGGBBAA or use ELI_COL32)
typedef uint32_t eli_col32;

#define ELI_COL32(r, g, b, a) (((uint32_t)(a)<<24) | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(r))
#define ELI_COL32_WHITE  0xFFFFFFFF
#define ELI_COL32_BLACK  0x000000FF
```

### Draw Commands

```c
typedef struct {
    eli_rect clip_rect;      // Scissor rectangle
    uint32_t texture_id;     // Texture handle (0 = font atlas)
    uint32_t vtx_offset;     // Offset into vertex buffer
    uint32_t idx_offset;     // Offset into index buffer
    uint32_t elem_count;     // Number of indices
} eli_draw_cmd;

typedef struct {
    float x, y;              // Position
    float u, v;              // Texture coordinates
    eli_col32 col;           // Color
} eli_draw_vert;

typedef uint16_t eli_draw_idx;
```

### Draw List

```c
typedef struct {
    eli_draw_cmd* cmds;      // Draw commands
    uint32_t cmd_count;
    uint32_t cmd_capacity;

    eli_draw_vert* vtx;      // Vertex buffer
    uint32_t vtx_count;
    uint32_t vtx_capacity;

    eli_draw_idx* idx;       // Index buffer
    uint32_t idx_count;
    uint32_t idx_capacity;
} eli_draw_list;
```

### IO Structure

```c
typedef struct {
    // Display
    eli_vec2 display_size;       // Canvas size in pixels
    float delta_time;            // Time since last frame

    // Mouse
    eli_vec2 mouse_pos;
    bool mouse_down[5];          // Button states
    float mouse_wheel;           // Scroll delta

    // Keyboard
    bool keys_down[512];
    bool key_ctrl;
    bool key_shift;
    bool key_alt;

    // Text input
    char input_chars[64];
    int input_char_count;

    // Output (set by elimgui)
    bool want_capture_mouse;
    bool want_capture_keyboard;
} eli_io;
```

### Context

```c
typedef struct {
    eli_io io;
    eli_style style;
    eli_draw_list draw_list;

    // Window state
    eli_window* windows;
    uint32_t window_count;
    eli_id active_id;
    eli_id hot_id;

    // Frame state
    uint32_t frame_count;
    eli_window* current_window;
} eli_context;
```

## Widget API

### Windows

```c
bool eli_begin(const char* name, bool* p_open, eli_window_flags flags);
void eli_end(void);

// Window flags
#define ELI_WINDOW_NO_TITLEBAR     (1 << 0)
#define ELI_WINDOW_NO_RESIZE       (1 << 1)
#define ELI_WINDOW_NO_MOVE         (1 << 2)
#define ELI_WINDOW_NO_SCROLLBAR    (1 << 3)
#define ELI_WINDOW_NO_COLLAPSE     (1 << 4)
#define ELI_WINDOW_AUTO_RESIZE     (1 << 5)
#define ELI_WINDOW_NO_BACKGROUND   (1 << 6)
```

### Basic Widgets

```c
// Text
void eli_text(const char* fmt, ...);
void eli_text_colored(eli_col32 col, const char* fmt, ...);
void eli_label_text(const char* label, const char* fmt, ...);

// Buttons
bool eli_button(const char* label);
bool eli_button_sized(const char* label, eli_vec2 size);
bool eli_small_button(const char* label);
bool eli_invisible_button(const char* str_id, eli_vec2 size);

// Checkboxes and Radio
bool eli_checkbox(const char* label, bool* v);
bool eli_radio_button(const char* label, bool active);

// Input
bool eli_input_text(const char* label, char* buf, size_t buf_size);
bool eli_input_int(const char* label, int* v);
bool eli_input_float(const char* label, float* v);

// Sliders
bool eli_slider_float(const char* label, float* v, float v_min, float v_max);
bool eli_slider_int(const char* label, int* v, int v_min, int v_max);

// Drag
bool eli_drag_float(const char* label, float* v, float speed);
bool eli_drag_int(const char* label, int* v, float speed);

// Color
bool eli_color_edit3(const char* label, float col[3]);
bool eli_color_edit4(const char* label, float col[4]);

// Combo/Dropdown
bool eli_begin_combo(const char* label, const char* preview);
void eli_end_combo(void);
bool eli_selectable(const char* label, bool selected);
```

### Layout

```c
void eli_same_line(void);
void eli_new_line(void);
void eli_separator(void);
void eli_spacing(void);
void eli_indent(float width);
void eli_unindent(float width);

// Groups
void eli_begin_group(void);
void eli_end_group(void);

// Columns (simple)
void eli_columns(int count);
void eli_next_column(void);

// Size
void eli_set_next_item_width(float width);
void eli_push_item_width(float width);
void eli_pop_item_width(void);
```

### Trees and Collapsing

```c
bool eli_tree_node(const char* label);
void eli_tree_pop(void);
bool eli_collapsing_header(const char* label);
```

### Menus

```c
bool eli_begin_main_menu_bar(void);
void eli_end_main_menu_bar(void);
bool eli_begin_menu(const char* label);
void eli_end_menu(void);
bool eli_menu_item(const char* label, const char* shortcut, bool* selected);
```

### Popups

```c
void eli_open_popup(const char* str_id);
bool eli_begin_popup(const char* str_id);
bool eli_begin_popup_modal(const char* name, bool* p_open);
void eli_end_popup(void);
void eli_close_current_popup(void);
```

### Tables

```c
bool eli_begin_table(const char* str_id, int columns, eli_table_flags flags);
void eli_end_table(void);
void eli_table_next_row(void);
void eli_table_next_column(void);
void eli_table_set_column_index(int column);
void eli_table_setup_column(const char* label, eli_table_column_flags flags);
void eli_table_headers_row(void);
```

## Frame Lifecycle

```c
// 1. Start frame (call once per frame)
void eli_new_frame(void);

// 2. Build UI (call widgets)
eli_begin("Window", NULL, 0);
if (eli_button("Click me")) { /* handle */ }
eli_slider_float("Value", &val, 0, 100);
eli_end();

// 3. Finalize (call once after all UI)
void eli_render(void);

// 4. Get render data
eli_draw_data* eli_get_draw_data(void);
```

## ID System

Widget identity for state persistence (like ImGui):

```c
typedef uint32_t eli_id;

eli_id eli_get_id(const char* str);
eli_id eli_get_id_ptr(const void* ptr);
void eli_push_id(const char* str);
void eli_push_id_int(int int_id);
void eli_pop_id(void);
```

## Styling

```c
typedef struct {
    // Colors
    eli_col32 colors[ELI_COL_COUNT];

    // Sizes
    float window_padding;
    float window_rounding;
    float frame_padding;
    float frame_rounding;
    float item_spacing;
    float item_inner_spacing;
    float indent_spacing;
    float scrollbar_size;
    float grab_min_size;

    // Font
    float font_size;
} eli_style;

// Color indices
enum {
    ELI_COL_TEXT,
    ELI_COL_TEXT_DISABLED,
    ELI_COL_WINDOW_BG,
    ELI_COL_BORDER,
    ELI_COL_FRAME_BG,
    ELI_COL_FRAME_BG_HOVERED,
    ELI_COL_FRAME_BG_ACTIVE,
    ELI_COL_BUTTON,
    ELI_COL_BUTTON_HOVERED,
    ELI_COL_BUTTON_ACTIVE,
    ELI_COL_HEADER,
    ELI_COL_HEADER_HOVERED,
    ELI_COL_HEADER_ACTIVE,
    ELI_COL_SCROLLBAR_BG,
    ELI_COL_SCROLLBAR_GRAB,
    ELI_COL_COUNT
};

void eli_style_colors_dark(void);
void eli_style_colors_light(void);
```

## Memory Model

elimgui uses a simple arena/bump allocator model:

```c
// User provides memory
void eli_set_allocator(void* (*alloc)(size_t), void (*free)(void*));

// Or use default (JAClibc malloc)
void eli_init(void);
void eli_shutdown(void);
```

## Font System

```c
// Load font (uses stb_truetype internally)
eli_font* eli_font_load(const void* ttf_data, size_t ttf_size, float size_pixels);
eli_font* eli_font_load_default(float size_pixels);

// Set current font
void eli_push_font(eli_font* font);
void eli_pop_font(void);

// Font atlas (generated texture for rendering)
void eli_font_get_tex_data(unsigned char** out_pixels, int* out_w, int* out_h);
```

## Example Usage

```c
#include <elimgui.h>

static float slider_value = 0.5f;
static bool checkbox_value = false;
static char text_buf[256] = "Hello";

void frame(void) {
    eli_new_frame();

    eli_begin("Demo Window", NULL, 0);

    eli_text("Welcome to elimgui!");
    eli_separator();

    if (eli_button("Click Me")) {
        // Button was clicked
    }

    eli_slider_float("Slider", &slider_value, 0.0f, 1.0f);
    eli_checkbox("Checkbox", &checkbox_value);
    eli_input_text("Text", text_buf, sizeof(text_buf));

    eli_end();

    eli_render();

    // Get draw data and render with your backend
    eli_draw_data* data = eli_get_draw_data();
    my_render_function(data);
}
```

## Implementation Notes

### Immediate Mode

All widget functions return whether interaction occurred that frame:
- `eli_button()` returns true the frame it was clicked
- `eli_checkbox()` returns true when value changed
- `eli_slider_*()` returns true while being dragged

### State Hashing

Widget state (is this button pressed? what's the scroll position?) is keyed by ID:
- IDs are hashed from label strings by default
- Use `##` to add hidden ID components: `"Button##1"`, `"Button##2"`
- Use `###` to keep ID stable while label changes: `"Value: 42###value_widget"`

### Clipping

Draw commands include clip rectangles. Renderers must set scissor state accordingly.

### Z-Order

Windows have implicit z-order based on focus. Popups render above all windows.
