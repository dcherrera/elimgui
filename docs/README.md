# elimgui Documentation

 Pure C11 Endless Loop Immediate Mode GUI Library - A ground-up reimplementation of Dear ImGui targeting WebAssembly.

## Table of Contents

### Getting Started

- [Core Types & Context](core-types.md) - Fundamental types, context management, and frame lifecycle

### Systems

- [Draw System](draw-system.md) - Draw lists, primitives, and rendering

- [Font System](font-system.md) - Font loading, atlas, and text rendering

- [Input System](input-system.md) - Mouse, keyboard, text input, and shortcuts

### Systems (Coming Soon)

- ID System - Widget identity and state management

- Style System - Colors, theming, and styling

### Widgets (Coming Soon)

- Basic Widgets - Text, buttons, checkboxes

- Sliders & Drags - Value adjustment widgets

- Input Widgets - Text and numeric input

- Color Widgets - Color editing and picking

- Combo & Selectable - Dropdowns and selection

- Trees & Collapsing - Hierarchical widgets

- Menus - Menu bars and items

- Popups & Modals - Popup windows

- Tooltips - Hover tooltips

- Tables - Full table widget

- Tab Bars - Tab navigation

### Advanced (Coming Soon)

- Windows - Window management

- Layout System - Positioning and sizing

- Drag & Drop - Drag and drop system

- Images - Image display

- Plotting - Simple data plots

### Reference

- [API Quick Reference](#api-quick-reference)

- [Dear ImGui Migration](#dear-imgui-migration)

---

## API Quick Reference

### Context Management

```
eli_context* eli_create_context(void);    // Create new context
void eli_destroy_context(eli_context*);   // Destroy context
eli_context* eli_get_current_context(void); // Get active context
void eli_set_current_context(eli_context*); // Set active context
eli_io* eli_get_io(void);                 // Get IO struct
eli_style* eli_get_style(void);           // Get style struct
```

### Frame Lifecycle

```
void eli_new_frame(void);                 // Start new frame
void eli_end_frame(void);                 // End current frame
void eli_render(void);                    // Generate draw data
eli_draw_data* eli_get_draw_data(void);   // Get draw data for rendering
```

### Core Types

```
typedef struct { float x, y; } eli_vec2;
typedef struct { float x, y, z, w; } eli_vec4;
typedef struct { eli_vec2 min, max; } eli_rect;
typedef uint32_t eli_id;
```

### Color Macros

```
ELI_COL32(r, g, b, a)    // Create 32-bit color
ELI_COL32_WHITE          // 0xFFFFFFFF
ELI_COL32_BLACK          // 0x000000FF
ELI_COL32_BLACK_TRANS    // 0x00000000
```

---

## Dear ImGui Migration

elimgui follows Dear ImGui's API patterns with C11 adaptations:

| Dear ImGui             | elimgui              | Notes           |
| ---------------------- | -------------------- | --------------- |
| ImGui::CreateContext() | eli_create_context() | Returns pointer |
| ImGui::NewFrame()      | eli_new_frame()      | -               |
| ImGui::Render()        | eli_render()         | -               |
| ImGui::GetIO()         | eli_get_io()         | Returns pointer |
| ImVec2                 | eli_vec2             | -               |
| ImVec4                 | eli_vec4             | -               |
| IM_COL32()             | ELI_COL32()          | -               |
| ImGuiWindowFlags_      | ELI_WINDOW_FLAGS_    | Enum prefix     |
| ImGuiCol_              | ELI_COL_             | Color indices   |

### Naming Conventions

- **Functions**: `eli_` prefix, snake_case (`eli_new_frame`)

- **Types**: `eli_` prefix, snake_case (`eli_vec2`, `eli_context`)

- **Enums**: `ELI_` prefix, SCREAMING_SNAKE_CASE (`ELI_WINDOW_FLAGS_NO_RESIZE`)

- **Macros**: `ELI_` prefix, SCREAMING_SNAKE_CASE (`ELI_COL32`)

---

## Building

elimgui targets WebAssembly via clang:

```
clang --target=wasm32 \
    -nostdlib \
    -Ivendor/jaclibc/include \
    -Iinclude \
    -O2 \
    -Wl,--no-entry \
    -Wl,--export-dynamic \
    -o output.wasm \
    your_app.c
```

See the main [README](../README.md) for full build instructions.