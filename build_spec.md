# elimgui - Build Specification

## Overview

**elimgui** is a pure C11 rewrite of Dear ImGui, designed for WebAssembly targets. It provides an immediate-mode GUI API familiar to ImGui users while being fully compatible with JAClibc and the Canvil framework.

This is NOT a binding or wrapper - it's a ground-up reimplementation in C with **full feature parity** with Dear ImGui.

## Goals

1. **Full feature parity with Dear ImGui** - All widgets, features, and capabilities
2. **ImGui-familiar API** - Developers who know ImGui should feel at home
3. **Pure C11** - No C++ features, compiles with clang to wasm32
4. **Header-only** - Single include, no separate compilation units
5. **Zero dependencies** - Only JAClibc (for WASM) and optionally stb_truetype
6. **Minimal footprint** - Small WASM binary size
7. **Renderer-agnostic** - Outputs draw lists, not pixels

## Non-Goals

- Native platform support (WASM-first, native is bonus)
- Backwards compatibility with ImGui code (similar API, not identical)

---

## Architecture

### Core Components

```
eli_context     → Global state container
eli_io          → Input/output (mouse, keyboard, display)
eli_style       → Colors, sizes, spacing (60+ properties)
eli_draw_list   → Accumulated draw commands
eli_draw_data   → Final render output
eli_font_atlas  → Font texture management
```

### Header Structure

| Header | Purpose |
|--------|---------|
| `elimgui.h` | Master include, core types, context management |
| `eli_draw.h` | Draw primitives (rect, line, circle, bezier, text, images) |
| `eli_widgets.h` | All widgets (button, slider, checkbox, etc.) |
| `eli_layout.h` | Layout system (same-line, columns, spacing) |
| `eli_input.h` | Input handling (mouse, keyboard, focus, shortcuts) |
| `eli_font.h` | Font loading and text rendering |
| `eli_style.h` | Styling and theming |
| `eli_tables.h` | Table widget (full system) |
| `eli_tabs.h` | Tab bar widget |
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
eli_end_frame()          # End frame processing
    ↓
eli_render()             # Finalize draw data
    ↓
eli_draw_data            # Vertex buffers + draw commands
    ↓
Your renderer            # WebGL, Canvas2D, WebGPU, etc.
```

---

## Enumerations & Flags

### Window Flags

```c
typedef int eli_window_flags;
enum eli_window_flags_ {
    ELI_WINDOW_NONE                      = 0,
    ELI_WINDOW_NO_TITLEBAR               = 1 << 0,
    ELI_WINDOW_NO_RESIZE                 = 1 << 1,
    ELI_WINDOW_NO_MOVE                   = 1 << 2,
    ELI_WINDOW_NO_SCROLLBAR              = 1 << 3,
    ELI_WINDOW_NO_SCROLL_WITH_MOUSE      = 1 << 4,
    ELI_WINDOW_NO_COLLAPSE               = 1 << 5,
    ELI_WINDOW_AUTO_RESIZE               = 1 << 6,
    ELI_WINDOW_NO_BACKGROUND             = 1 << 7,
    ELI_WINDOW_NO_SAVED_SETTINGS         = 1 << 8,
    ELI_WINDOW_NO_MOUSE_INPUTS           = 1 << 9,
    ELI_WINDOW_MENU_BAR                  = 1 << 10,
    ELI_WINDOW_HORIZONTAL_SCROLLBAR      = 1 << 11,
    ELI_WINDOW_NO_FOCUS_ON_APPEARING     = 1 << 12,
    ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 13,
    ELI_WINDOW_ALWAYS_VERTICAL_SCROLLBAR = 1 << 14,
    ELI_WINDOW_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
    ELI_WINDOW_NO_NAV_INPUTS             = 1 << 16,
    ELI_WINDOW_NO_NAV_FOCUS              = 1 << 17,
    ELI_WINDOW_UNSAVED_DOCUMENT          = 1 << 18,
    // Combinations
    ELI_WINDOW_NO_NAV                    = ELI_WINDOW_NO_NAV_INPUTS | ELI_WINDOW_NO_NAV_FOCUS,
    ELI_WINDOW_NO_DECORATION             = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_SCROLLBAR | ELI_WINDOW_NO_COLLAPSE,
    ELI_WINDOW_NO_INPUTS                 = ELI_WINDOW_NO_MOUSE_INPUTS | ELI_WINDOW_NO_NAV_INPUTS | ELI_WINDOW_NO_NAV_FOCUS,
};
```

### Child Window Flags

```c
typedef int eli_child_flags;
enum eli_child_flags_ {
    ELI_CHILD_NONE                       = 0,
    ELI_CHILD_BORDERS                    = 1 << 0,
    ELI_CHILD_ALWAYS_USE_WINDOW_PADDING  = 1 << 1,
    ELI_CHILD_RESIZE_X                   = 1 << 2,
    ELI_CHILD_RESIZE_Y                   = 1 << 3,
    ELI_CHILD_AUTO_RESIZE_X              = 1 << 4,
    ELI_CHILD_AUTO_RESIZE_Y              = 1 << 5,
    ELI_CHILD_ALWAYS_AUTO_RESIZE         = 1 << 6,
    ELI_CHILD_FRAME_STYLE                = 1 << 7,
    ELI_CHILD_NAV_FLATTENED              = 1 << 8,
};
```

### Item Flags

```c
typedef int eli_item_flags;
enum eli_item_flags_ {
    ELI_ITEM_NONE                        = 0,
    ELI_ITEM_NO_TAB_STOP                 = 1 << 0,
    ELI_ITEM_NO_NAV                      = 1 << 1,
    ELI_ITEM_NO_NAV_DEFAULT_FOCUS        = 1 << 2,
    ELI_ITEM_BUTTON_REPEAT               = 1 << 3,
    ELI_ITEM_AUTO_CLOSE_POPUPS           = 1 << 4,
    ELI_ITEM_ALLOW_DUPLICATE_ID          = 1 << 5,
    ELI_ITEM_DISABLED                    = 1 << 6,
};
```

### Input Text Flags

```c
typedef int eli_input_text_flags;
enum eli_input_text_flags_ {
    ELI_INPUT_TEXT_NONE                  = 0,
    ELI_INPUT_TEXT_CHARS_DECIMAL         = 1 << 0,
    ELI_INPUT_TEXT_CHARS_HEXADECIMAL     = 1 << 1,
    ELI_INPUT_TEXT_CHARS_SCIENTIFIC      = 1 << 2,
    ELI_INPUT_TEXT_CHARS_UPPERCASE       = 1 << 3,
    ELI_INPUT_TEXT_CHARS_NO_BLANK        = 1 << 4,
    ELI_INPUT_TEXT_ALLOW_TAB_INPUT       = 1 << 5,
    ELI_INPUT_TEXT_ENTER_RETURNS_TRUE    = 1 << 6,
    ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL     = 1 << 7,
    ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE = 1 << 8,
    ELI_INPUT_TEXT_READ_ONLY             = 1 << 9,
    ELI_INPUT_TEXT_PASSWORD              = 1 << 10,
    ELI_INPUT_TEXT_ALWAYS_OVERWRITE      = 1 << 11,
    ELI_INPUT_TEXT_AUTO_SELECT_ALL       = 1 << 12,
    ELI_INPUT_TEXT_PARSE_EMPTY_REF_VAL   = 1 << 13,
    ELI_INPUT_TEXT_DISPLAY_EMPTY_REF_VAL = 1 << 14,
    ELI_INPUT_TEXT_NO_HORIZONTAL_SCROLL  = 1 << 15,
    ELI_INPUT_TEXT_NO_UNDO_REDO          = 1 << 16,
    ELI_INPUT_TEXT_ELIDE_LEFT            = 1 << 17,
    ELI_INPUT_TEXT_CALLBACK_COMPLETION   = 1 << 18,
    ELI_INPUT_TEXT_CALLBACK_HISTORY      = 1 << 19,
    ELI_INPUT_TEXT_CALLBACK_ALWAYS       = 1 << 20,
    ELI_INPUT_TEXT_CALLBACK_CHAR_FILTER  = 1 << 21,
    ELI_INPUT_TEXT_CALLBACK_RESIZE       = 1 << 22,
    ELI_INPUT_TEXT_CALLBACK_EDIT         = 1 << 23,
};
```

### Tree Node Flags

```c
typedef int eli_tree_node_flags;
enum eli_tree_node_flags_ {
    ELI_TREE_NODE_NONE                   = 0,
    ELI_TREE_NODE_SELECTED               = 1 << 0,
    ELI_TREE_NODE_FRAMED                 = 1 << 1,
    ELI_TREE_NODE_ALLOW_OVERLAP          = 1 << 2,
    ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN   = 1 << 3,
    ELI_TREE_NODE_NO_AUTO_OPEN_ON_LOG    = 1 << 4,
    ELI_TREE_NODE_DEFAULT_OPEN           = 1 << 5,
    ELI_TREE_NODE_OPEN_ON_DOUBLE_CLICK   = 1 << 6,
    ELI_TREE_NODE_OPEN_ON_ARROW          = 1 << 7,
    ELI_TREE_NODE_LEAF                   = 1 << 8,
    ELI_TREE_NODE_BULLET                 = 1 << 9,
    ELI_TREE_NODE_FRAME_PADDING          = 1 << 10,
    ELI_TREE_NODE_SPAN_AVAIL_WIDTH       = 1 << 11,
    ELI_TREE_NODE_SPAN_FULL_WIDTH        = 1 << 12,
    ELI_TREE_NODE_SPAN_TEXT_WIDTH        = 1 << 13,
    ELI_TREE_NODE_SPAN_ALL_COLUMNS       = 1 << 14,
    ELI_TREE_NODE_NAV_LEFT_JUMPS_BACK_HERE = 1 << 15,
    // Combinations
    ELI_TREE_NODE_COLLAPSING_HEADER      = ELI_TREE_NODE_FRAMED | ELI_TREE_NODE_NO_TREE_PUSH_ON_OPEN | ELI_TREE_NODE_NO_AUTO_OPEN_ON_LOG,
};
```

### Popup Flags

```c
typedef int eli_popup_flags;
enum eli_popup_flags_ {
    ELI_POPUP_NONE                       = 0,
    ELI_POPUP_MOUSE_BUTTON_LEFT          = 0,
    ELI_POPUP_MOUSE_BUTTON_RIGHT         = 1,
    ELI_POPUP_MOUSE_BUTTON_MIDDLE        = 2,
    ELI_POPUP_NO_REOPEN                  = 1 << 5,
    ELI_POPUP_NO_OPEN_OVER_EXISTING_POPUP = 1 << 7,
    ELI_POPUP_NO_OPEN_OVER_ITEMS         = 1 << 8,
    ELI_POPUP_ANY_POPUP_ID               = 1 << 10,
    ELI_POPUP_ANY_POPUP_LEVEL            = 1 << 11,
    ELI_POPUP_ANY_POPUP                  = ELI_POPUP_ANY_POPUP_ID | ELI_POPUP_ANY_POPUP_LEVEL,
};
```

### Selectable Flags

```c
typedef int eli_selectable_flags;
enum eli_selectable_flags_ {
    ELI_SELECTABLE_NONE                  = 0,
    ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS  = 1 << 0,
    ELI_SELECTABLE_SPAN_ALL_COLUMNS      = 1 << 1,
    ELI_SELECTABLE_ALLOW_DOUBLE_CLICK    = 1 << 2,
    ELI_SELECTABLE_DISABLED              = 1 << 3,
    ELI_SELECTABLE_ALLOW_OVERLAP         = 1 << 4,
    ELI_SELECTABLE_HIGHLIGHT             = 1 << 5,
};
```

### Combo Flags

```c
typedef int eli_combo_flags;
enum eli_combo_flags_ {
    ELI_COMBO_NONE                       = 0,
    ELI_COMBO_POPUP_ALIGN_LEFT           = 1 << 0,
    ELI_COMBO_HEIGHT_SMALL               = 1 << 1,
    ELI_COMBO_HEIGHT_REGULAR             = 1 << 2,
    ELI_COMBO_HEIGHT_LARGE               = 1 << 3,
    ELI_COMBO_HEIGHT_LARGEST             = 1 << 4,
    ELI_COMBO_NO_ARROW_BUTTON            = 1 << 5,
    ELI_COMBO_NO_PREVIEW                 = 1 << 6,
    ELI_COMBO_WIDTH_FIT_PREVIEW          = 1 << 7,
};
```

### Tab Bar Flags

```c
typedef int eli_tab_bar_flags;
enum eli_tab_bar_flags_ {
    ELI_TAB_BAR_NONE                     = 0,
    ELI_TAB_BAR_REORDERABLE              = 1 << 0,
    ELI_TAB_BAR_AUTO_SELECT_NEW_TABS     = 1 << 1,
    ELI_TAB_BAR_TAB_LIST_POPUP_BUTTON    = 1 << 2,
    ELI_TAB_BAR_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 3,
    ELI_TAB_BAR_NO_TAB_LIST_SCROLLING_BUTTONS = 1 << 4,
    ELI_TAB_BAR_NO_TOOLTIP               = 1 << 5,
    ELI_TAB_BAR_DRAW_SELECTED_OVERLINE   = 1 << 6,
    ELI_TAB_BAR_FITTING_POLICY_RESIZE_DOWN = 1 << 7,
    ELI_TAB_BAR_FITTING_POLICY_SCROLL    = 1 << 8,
};
```

### Tab Item Flags

```c
typedef int eli_tab_item_flags;
enum eli_tab_item_flags_ {
    ELI_TAB_ITEM_NONE                    = 0,
    ELI_TAB_ITEM_UNSAVED_DOCUMENT        = 1 << 0,
    ELI_TAB_ITEM_SET_SELECTED            = 1 << 1,
    ELI_TAB_ITEM_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 2,
    ELI_TAB_ITEM_NO_PUSH_ID              = 1 << 3,
    ELI_TAB_ITEM_NO_TOOLTIP              = 1 << 4,
    ELI_TAB_ITEM_NO_REORDER              = 1 << 5,
    ELI_TAB_ITEM_LEADING                 = 1 << 6,
    ELI_TAB_ITEM_TRAILING                = 1 << 7,
    ELI_TAB_ITEM_NO_ASSUMED_CLOSURE      = 1 << 8,
};
```

### Table Flags

```c
typedef int eli_table_flags;
enum eli_table_flags_ {
    ELI_TABLE_NONE                       = 0,
    // Features
    ELI_TABLE_RESIZABLE                  = 1 << 0,
    ELI_TABLE_REORDERABLE                = 1 << 1,
    ELI_TABLE_HIDEABLE                   = 1 << 2,
    ELI_TABLE_SORTABLE                   = 1 << 3,
    ELI_TABLE_NO_SAVED_SETTINGS          = 1 << 4,
    ELI_TABLE_CONTEXT_MENU_IN_BODY       = 1 << 5,
    // Decorations
    ELI_TABLE_ROW_BG                     = 1 << 6,
    ELI_TABLE_BORDERS_INNER_H            = 1 << 7,
    ELI_TABLE_BORDERS_OUTER_H            = 1 << 8,
    ELI_TABLE_BORDERS_INNER_V            = 1 << 9,
    ELI_TABLE_BORDERS_OUTER_V            = 1 << 10,
    ELI_TABLE_BORDERS_H                  = ELI_TABLE_BORDERS_INNER_H | ELI_TABLE_BORDERS_OUTER_H,
    ELI_TABLE_BORDERS_V                  = ELI_TABLE_BORDERS_INNER_V | ELI_TABLE_BORDERS_OUTER_V,
    ELI_TABLE_BORDERS_INNER              = ELI_TABLE_BORDERS_INNER_H | ELI_TABLE_BORDERS_INNER_V,
    ELI_TABLE_BORDERS_OUTER              = ELI_TABLE_BORDERS_OUTER_H | ELI_TABLE_BORDERS_OUTER_V,
    ELI_TABLE_BORDERS                    = ELI_TABLE_BORDERS_INNER | ELI_TABLE_BORDERS_OUTER,
    ELI_TABLE_NO_BORDERS_IN_BODY         = 1 << 11,
    ELI_TABLE_NO_BORDERS_IN_BODY_UNTIL_RESIZE = 1 << 12,
    // Sizing
    ELI_TABLE_SIZING_FIXED_FIT           = 1 << 13,
    ELI_TABLE_SIZING_FIXED_SAME          = 1 << 14,
    ELI_TABLE_SIZING_STRETCH_PROP        = 1 << 15,
    ELI_TABLE_SIZING_STRETCH_SAME        = 1 << 16,
    // Sizing policies
    ELI_TABLE_NO_HOST_EXTEND_X           = 1 << 17,
    ELI_TABLE_NO_HOST_EXTEND_Y           = 1 << 18,
    ELI_TABLE_NO_KEEP_COLUMNS_VISIBLE    = 1 << 19,
    ELI_TABLE_PRECISE_WIDTHS             = 1 << 20,
    // Clipping
    ELI_TABLE_NO_CLIP                    = 1 << 21,
    // Padding
    ELI_TABLE_PAD_OUTER_X                = 1 << 22,
    ELI_TABLE_NO_PAD_OUTER_X             = 1 << 23,
    ELI_TABLE_NO_PAD_INNER_X             = 1 << 24,
    // Scrolling
    ELI_TABLE_SCROLL_X                   = 1 << 25,
    ELI_TABLE_SCROLL_Y                   = 1 << 26,
    // Sorting
    ELI_TABLE_SORT_MULTI                 = 1 << 27,
    ELI_TABLE_SORT_TRISTATE              = 1 << 28,
    ELI_TABLE_HIGHLIGHT_HOVERED_COLUMN   = 1 << 29,
};
```

### Table Column Flags

```c
typedef int eli_table_column_flags;
enum eli_table_column_flags_ {
    ELI_TABLE_COLUMN_NONE                = 0,
    ELI_TABLE_COLUMN_DISABLED            = 1 << 0,
    ELI_TABLE_COLUMN_DEFAULT_HIDE        = 1 << 1,
    ELI_TABLE_COLUMN_DEFAULT_SORT        = 1 << 2,
    ELI_TABLE_COLUMN_WIDTH_STRETCH       = 1 << 3,
    ELI_TABLE_COLUMN_WIDTH_FIXED         = 1 << 4,
    ELI_TABLE_COLUMN_NO_RESIZE           = 1 << 5,
    ELI_TABLE_COLUMN_NO_REORDER          = 1 << 6,
    ELI_TABLE_COLUMN_NO_HIDE             = 1 << 7,
    ELI_TABLE_COLUMN_NO_CLIP             = 1 << 8,
    ELI_TABLE_COLUMN_NO_SORT             = 1 << 9,
    ELI_TABLE_COLUMN_NO_SORT_ASCENDING   = 1 << 10,
    ELI_TABLE_COLUMN_NO_SORT_DESCENDING  = 1 << 11,
    ELI_TABLE_COLUMN_NO_HEADER_LABEL     = 1 << 12,
    ELI_TABLE_COLUMN_NO_HEADER_WIDTH     = 1 << 13,
    ELI_TABLE_COLUMN_PREFER_SORT_ASCENDING = 1 << 14,
    ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING = 1 << 15,
    ELI_TABLE_COLUMN_INDENT_ENABLE       = 1 << 16,
    ELI_TABLE_COLUMN_INDENT_DISABLE      = 1 << 17,
    ELI_TABLE_COLUMN_ANGLED_HEADER       = 1 << 18,
    // Output flags
    ELI_TABLE_COLUMN_IS_ENABLED          = 1 << 24,
    ELI_TABLE_COLUMN_IS_VISIBLE          = 1 << 25,
    ELI_TABLE_COLUMN_IS_SORTED           = 1 << 26,
    ELI_TABLE_COLUMN_IS_HOVERED          = 1 << 27,
};
```

### Table Row Flags

```c
typedef int eli_table_row_flags;
enum eli_table_row_flags_ {
    ELI_TABLE_ROW_NONE                   = 0,
    ELI_TABLE_ROW_HEADERS                = 1 << 0,
};
```

### Table Bg Target

```c
typedef int eli_table_bg_target;
enum eli_table_bg_target_ {
    ELI_TABLE_BG_TARGET_NONE             = 0,
    ELI_TABLE_BG_TARGET_ROW_BG0          = 1,
    ELI_TABLE_BG_TARGET_ROW_BG1          = 2,
    ELI_TABLE_BG_TARGET_CELL_BG          = 3,
};
```

### Focused Flags

```c
typedef int eli_focused_flags;
enum eli_focused_flags_ {
    ELI_FOCUSED_NONE                     = 0,
    ELI_FOCUSED_CHILD_WINDOWS            = 1 << 0,
    ELI_FOCUSED_ROOT_WINDOW              = 1 << 1,
    ELI_FOCUSED_ANY_WINDOW               = 1 << 2,
    ELI_FOCUSED_NO_POPUP_HIERARCHY       = 1 << 3,
    ELI_FOCUSED_ROOT_AND_CHILD_WINDOWS   = ELI_FOCUSED_ROOT_WINDOW | ELI_FOCUSED_CHILD_WINDOWS,
};
```

### Hovered Flags

```c
typedef int eli_hovered_flags;
enum eli_hovered_flags_ {
    ELI_HOVERED_NONE                     = 0,
    ELI_HOVERED_CHILD_WINDOWS            = 1 << 0,
    ELI_HOVERED_ROOT_WINDOW              = 1 << 1,
    ELI_HOVERED_ANY_WINDOW               = 1 << 2,
    ELI_HOVERED_NO_POPUP_HIERARCHY       = 1 << 3,
    ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP = 1 << 5,
    ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM = 1 << 7,
    ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_ITEM = 1 << 8,
    ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_WINDOW = 1 << 9,
    ELI_HOVERED_ALLOW_WHEN_DISABLED      = 1 << 10,
    ELI_HOVERED_NO_NAV_OVERRIDE          = 1 << 11,
    ELI_HOVERED_RECT_ONLY                = ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP | ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM | ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_ITEM,
    ELI_HOVERED_ROOT_AND_CHILD_WINDOWS   = ELI_HOVERED_ROOT_WINDOW | ELI_HOVERED_CHILD_WINDOWS,
    // Tooltips
    ELI_HOVERED_FOR_TOOLTIP              = 1 << 12,
    ELI_HOVERED_STATIONARY               = 1 << 13,
    ELI_HOVERED_DELAY_NONE               = 1 << 14,
    ELI_HOVERED_DELAY_SHORT              = 1 << 15,
    ELI_HOVERED_DELAY_NORMAL             = 1 << 16,
    ELI_HOVERED_NO_SHARED_DELAY          = 1 << 17,
};
```

### Drag Drop Flags

```c
typedef int eli_drag_drop_flags;
enum eli_drag_drop_flags_ {
    ELI_DRAG_DROP_NONE                   = 0,
    // Source flags
    ELI_DRAG_DROP_SOURCE_NO_PREVIEW_TOOLTIP = 1 << 0,
    ELI_DRAG_DROP_SOURCE_NO_DISABLE_HOVER = 1 << 1,
    ELI_DRAG_DROP_SOURCE_NO_HOLD_TO_OPEN_OTHERS = 1 << 2,
    ELI_DRAG_DROP_SOURCE_ALLOW_NULL_ID   = 1 << 3,
    ELI_DRAG_DROP_SOURCE_EXTERN          = 1 << 4,
    ELI_DRAG_DROP_SOURCE_AUTO_EXPIRE_PAYLOAD = 1 << 5,
    // Target flags
    ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY = 1 << 10,
    ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT = 1 << 11,
    ELI_DRAG_DROP_ACCEPT_NO_PREVIEW_TOOLTIP = 1 << 12,
    ELI_DRAG_DROP_ACCEPT_PEEK_ONLY       = ELI_DRAG_DROP_ACCEPT_BEFORE_DELIVERY | ELI_DRAG_DROP_ACCEPT_NO_DRAW_DEFAULT_RECT,
};
```

### Data Type

```c
typedef int eli_data_type;
enum eli_data_type_ {
    ELI_DATA_TYPE_S8,
    ELI_DATA_TYPE_U8,
    ELI_DATA_TYPE_S16,
    ELI_DATA_TYPE_U16,
    ELI_DATA_TYPE_S32,
    ELI_DATA_TYPE_U32,
    ELI_DATA_TYPE_S64,
    ELI_DATA_TYPE_U64,
    ELI_DATA_TYPE_FLOAT,
    ELI_DATA_TYPE_DOUBLE,
    ELI_DATA_TYPE_COUNT
};
```

### Direction

```c
typedef int eli_dir;
enum eli_dir_ {
    ELI_DIR_NONE  = -1,
    ELI_DIR_LEFT  = 0,
    ELI_DIR_RIGHT = 1,
    ELI_DIR_UP    = 2,
    ELI_DIR_DOWN  = 3,
    ELI_DIR_COUNT
};
```

### Sort Direction

```c
typedef int eli_sort_direction;
enum eli_sort_direction_ {
    ELI_SORT_NONE       = 0,
    ELI_SORT_ASCENDING  = 1,
    ELI_SORT_DESCENDING = 2,
};
```

### Mouse Button

```c
typedef int eli_mouse_button;
enum eli_mouse_button_ {
    ELI_MOUSE_BUTTON_LEFT   = 0,
    ELI_MOUSE_BUTTON_RIGHT  = 1,
    ELI_MOUSE_BUTTON_MIDDLE = 2,
    ELI_MOUSE_BUTTON_COUNT  = 5
};
```

### Mouse Cursor

```c
typedef int eli_mouse_cursor;
enum eli_mouse_cursor_ {
    ELI_MOUSE_CURSOR_NONE = -1,
    ELI_MOUSE_CURSOR_ARROW = 0,
    ELI_MOUSE_CURSOR_TEXT_INPUT,
    ELI_MOUSE_CURSOR_RESIZE_ALL,
    ELI_MOUSE_CURSOR_RESIZE_NS,
    ELI_MOUSE_CURSOR_RESIZE_EW,
    ELI_MOUSE_CURSOR_RESIZE_NESW,
    ELI_MOUSE_CURSOR_RESIZE_NWSE,
    ELI_MOUSE_CURSOR_HAND,
    ELI_MOUSE_CURSOR_NOT_ALLOWED,
    ELI_MOUSE_CURSOR_COUNT
};
```

### Condition

```c
typedef int eli_cond;
enum eli_cond_ {
    ELI_COND_NONE            = 0,
    ELI_COND_ALWAYS          = 1 << 0,
    ELI_COND_ONCE            = 1 << 1,
    ELI_COND_FIRST_USE_EVER  = 1 << 2,
    ELI_COND_APPEARING       = 1 << 3,
};
```

### Slider Flags

```c
typedef int eli_slider_flags;
enum eli_slider_flags_ {
    ELI_SLIDER_NONE          = 0,
    ELI_SLIDER_LOGARITHMIC   = 1 << 5,
    ELI_SLIDER_NO_ROUND_TO_FORMAT = 1 << 6,
    ELI_SLIDER_NO_INPUT      = 1 << 7,
    ELI_SLIDER_WRAP_AROUND   = 1 << 8,
    ELI_SLIDER_CLAMP_ON_INPUT = 1 << 9,
    ELI_SLIDER_CLAMP_ZERO_RANGE = 1 << 10,
    ELI_SLIDER_ALWAYS_CLAMP  = ELI_SLIDER_CLAMP_ON_INPUT | ELI_SLIDER_CLAMP_ZERO_RANGE,
};
```

### Color Edit Flags

```c
typedef int eli_color_edit_flags;
enum eli_color_edit_flags_ {
    ELI_COLOR_EDIT_NONE              = 0,
    ELI_COLOR_EDIT_NO_ALPHA          = 1 << 1,
    ELI_COLOR_EDIT_NO_PICKER         = 1 << 2,
    ELI_COLOR_EDIT_NO_OPTIONS        = 1 << 3,
    ELI_COLOR_EDIT_NO_SMALL_PREVIEW  = 1 << 4,
    ELI_COLOR_EDIT_NO_INPUTS         = 1 << 5,
    ELI_COLOR_EDIT_NO_TOOLTIP        = 1 << 6,
    ELI_COLOR_EDIT_NO_LABEL          = 1 << 7,
    ELI_COLOR_EDIT_NO_SIDE_PREVIEW   = 1 << 8,
    ELI_COLOR_EDIT_NO_DRAG_DROP      = 1 << 9,
    ELI_COLOR_EDIT_NO_BORDER         = 1 << 10,
    ELI_COLOR_EDIT_ALPHA_BAR         = 1 << 16,
    ELI_COLOR_EDIT_ALPHA_PREVIEW     = 1 << 17,
    ELI_COLOR_EDIT_ALPHA_PREVIEW_HALF = 1 << 18,
    ELI_COLOR_EDIT_HDR               = 1 << 19,
    ELI_COLOR_EDIT_DISPLAY_RGB       = 1 << 20,
    ELI_COLOR_EDIT_DISPLAY_HSV       = 1 << 21,
    ELI_COLOR_EDIT_DISPLAY_HEX       = 1 << 22,
    ELI_COLOR_EDIT_UINT8             = 1 << 23,
    ELI_COLOR_EDIT_FLOAT             = 1 << 24,
    ELI_COLOR_EDIT_PICKER_HUE_BAR    = 1 << 25,
    ELI_COLOR_EDIT_PICKER_HUE_WHEEL  = 1 << 26,
    ELI_COLOR_EDIT_INPUT_RGB         = 1 << 27,
    ELI_COLOR_EDIT_INPUT_HSV         = 1 << 28,
};
```

### Button Flags

```c
typedef int eli_button_flags;
enum eli_button_flags_ {
    ELI_BUTTON_NONE                  = 0,
    ELI_BUTTON_MOUSE_BUTTON_LEFT     = 1 << 0,
    ELI_BUTTON_MOUSE_BUTTON_RIGHT    = 1 << 1,
    ELI_BUTTON_MOUSE_BUTTON_MIDDLE   = 1 << 2,
    ELI_BUTTON_ENABLE_NAV            = 1 << 3,
};
```

### Draw List Flags

```c
typedef int eli_draw_list_flags;
enum eli_draw_list_flags_ {
    ELI_DRAW_LIST_NONE               = 0,
    ELI_DRAW_LIST_ANTI_ALIASED_LINES = 1 << 0,
    ELI_DRAW_LIST_ANTI_ALIASED_LINES_USE_TEX = 1 << 1,
    ELI_DRAW_LIST_ANTI_ALIASED_FILL  = 1 << 2,
    ELI_DRAW_LIST_ALLOW_VTX_OFFSET   = 1 << 3,
};
```

### Key Codes

```c
typedef int eli_key;
enum eli_key_ {
    ELI_KEY_NONE = 0,
    // Keyboard
    ELI_KEY_TAB, ELI_KEY_LEFT_ARROW, ELI_KEY_RIGHT_ARROW, ELI_KEY_UP_ARROW, ELI_KEY_DOWN_ARROW,
    ELI_KEY_PAGE_UP, ELI_KEY_PAGE_DOWN, ELI_KEY_HOME, ELI_KEY_END,
    ELI_KEY_INSERT, ELI_KEY_DELETE, ELI_KEY_BACKSPACE, ELI_KEY_SPACE, ELI_KEY_ENTER,
    ELI_KEY_ESCAPE, ELI_KEY_APOSTROPHE, ELI_KEY_COMMA, ELI_KEY_MINUS, ELI_KEY_PERIOD,
    ELI_KEY_SLASH, ELI_KEY_SEMICOLON, ELI_KEY_EQUAL, ELI_KEY_LEFT_BRACKET, ELI_KEY_BACKSLASH,
    ELI_KEY_RIGHT_BRACKET, ELI_KEY_GRAVE_ACCENT, ELI_KEY_CAPS_LOCK, ELI_KEY_SCROLL_LOCK, ELI_KEY_NUM_LOCK,
    ELI_KEY_PRINT_SCREEN, ELI_KEY_PAUSE,
    ELI_KEY_KEYPAD_0, ELI_KEY_KEYPAD_1, ELI_KEY_KEYPAD_2, ELI_KEY_KEYPAD_3, ELI_KEY_KEYPAD_4,
    ELI_KEY_KEYPAD_5, ELI_KEY_KEYPAD_6, ELI_KEY_KEYPAD_7, ELI_KEY_KEYPAD_8, ELI_KEY_KEYPAD_9,
    ELI_KEY_KEYPAD_DECIMAL, ELI_KEY_KEYPAD_DIVIDE, ELI_KEY_KEYPAD_MULTIPLY, ELI_KEY_KEYPAD_SUBTRACT,
    ELI_KEY_KEYPAD_ADD, ELI_KEY_KEYPAD_ENTER, ELI_KEY_KEYPAD_EQUAL,
    // Modifiers
    ELI_KEY_LEFT_CTRL, ELI_KEY_LEFT_SHIFT, ELI_KEY_LEFT_ALT, ELI_KEY_LEFT_SUPER,
    ELI_KEY_RIGHT_CTRL, ELI_KEY_RIGHT_SHIFT, ELI_KEY_RIGHT_ALT, ELI_KEY_RIGHT_SUPER,
    ELI_KEY_MENU,
    // Letters
    ELI_KEY_0, ELI_KEY_1, ELI_KEY_2, ELI_KEY_3, ELI_KEY_4, ELI_KEY_5, ELI_KEY_6, ELI_KEY_7, ELI_KEY_8, ELI_KEY_9,
    ELI_KEY_A, ELI_KEY_B, ELI_KEY_C, ELI_KEY_D, ELI_KEY_E, ELI_KEY_F, ELI_KEY_G, ELI_KEY_H, ELI_KEY_I, ELI_KEY_J,
    ELI_KEY_K, ELI_KEY_L, ELI_KEY_M, ELI_KEY_N, ELI_KEY_O, ELI_KEY_P, ELI_KEY_Q, ELI_KEY_R, ELI_KEY_S, ELI_KEY_T,
    ELI_KEY_U, ELI_KEY_V, ELI_KEY_W, ELI_KEY_X, ELI_KEY_Y, ELI_KEY_Z,
    // Function keys
    ELI_KEY_F1, ELI_KEY_F2, ELI_KEY_F3, ELI_KEY_F4, ELI_KEY_F5, ELI_KEY_F6,
    ELI_KEY_F7, ELI_KEY_F8, ELI_KEY_F9, ELI_KEY_F10, ELI_KEY_F11, ELI_KEY_F12,
    ELI_KEY_F13, ELI_KEY_F14, ELI_KEY_F15, ELI_KEY_F16, ELI_KEY_F17, ELI_KEY_F18,
    ELI_KEY_F19, ELI_KEY_F20, ELI_KEY_F21, ELI_KEY_F22, ELI_KEY_F23, ELI_KEY_F24,
    // Modifiers (as keys)
    ELI_KEY_MOD_CTRL, ELI_KEY_MOD_SHIFT, ELI_KEY_MOD_ALT, ELI_KEY_MOD_SUPER,
    ELI_KEY_COUNT,
    // Mouse buttons as keys (for unified input)
    ELI_KEY_MOUSE_LEFT, ELI_KEY_MOUSE_RIGHT, ELI_KEY_MOUSE_MIDDLE, ELI_KEY_MOUSE_X1, ELI_KEY_MOUSE_X2,
    ELI_KEY_MOUSE_WHEEL_X, ELI_KEY_MOUSE_WHEEL_Y,
};
```

### Color Indices (51 colors)

```c
typedef int eli_col;
enum eli_col_ {
    ELI_COL_TEXT,
    ELI_COL_TEXT_DISABLED,
    ELI_COL_WINDOW_BG,
    ELI_COL_CHILD_BG,
    ELI_COL_POPUP_BG,
    ELI_COL_BORDER,
    ELI_COL_BORDER_SHADOW,
    ELI_COL_FRAME_BG,
    ELI_COL_FRAME_BG_HOVERED,
    ELI_COL_FRAME_BG_ACTIVE,
    ELI_COL_TITLE_BG,
    ELI_COL_TITLE_BG_ACTIVE,
    ELI_COL_TITLE_BG_COLLAPSED,
    ELI_COL_MENU_BAR_BG,
    ELI_COL_SCROLLBAR_BG,
    ELI_COL_SCROLLBAR_GRAB,
    ELI_COL_SCROLLBAR_GRAB_HOVERED,
    ELI_COL_SCROLLBAR_GRAB_ACTIVE,
    ELI_COL_CHECK_MARK,
    ELI_COL_SLIDER_GRAB,
    ELI_COL_SLIDER_GRAB_ACTIVE,
    ELI_COL_BUTTON,
    ELI_COL_BUTTON_HOVERED,
    ELI_COL_BUTTON_ACTIVE,
    ELI_COL_HEADER,
    ELI_COL_HEADER_HOVERED,
    ELI_COL_HEADER_ACTIVE,
    ELI_COL_SEPARATOR,
    ELI_COL_SEPARATOR_HOVERED,
    ELI_COL_SEPARATOR_ACTIVE,
    ELI_COL_RESIZE_GRIP,
    ELI_COL_RESIZE_GRIP_HOVERED,
    ELI_COL_RESIZE_GRIP_ACTIVE,
    ELI_COL_TAB_HOVERED,
    ELI_COL_TAB,
    ELI_COL_TAB_SELECTED,
    ELI_COL_TAB_SELECTED_OVERLINE,
    ELI_COL_TAB_DIMMED,
    ELI_COL_TAB_DIMMED_SELECTED,
    ELI_COL_TAB_DIMMED_SELECTED_OVERLINE,
    ELI_COL_PLOT_LINES,
    ELI_COL_PLOT_LINES_HOVERED,
    ELI_COL_PLOT_HISTOGRAM,
    ELI_COL_PLOT_HISTOGRAM_HOVERED,
    ELI_COL_TABLE_HEADER_BG,
    ELI_COL_TABLE_BORDER_STRONG,
    ELI_COL_TABLE_BORDER_LIGHT,
    ELI_COL_TABLE_ROW_BG,
    ELI_COL_TABLE_ROW_BG_ALT,
    ELI_COL_TEXT_LINK,
    ELI_COL_TEXT_SELECTED_BG,
    ELI_COL_DRAG_DROP_TARGET,
    ELI_COL_NAV_CURSOR,
    ELI_COL_NAV_WINDOWING_HIGHLIGHT,
    ELI_COL_NAV_WINDOWING_DIM_BG,
    ELI_COL_MODAL_WINDOW_DIM_BG,
    ELI_COL_COUNT
};
```

### Style Variables

```c
typedef int eli_style_var;
enum eli_style_var_ {
    ELI_STYLE_VAR_ALPHA,
    ELI_STYLE_VAR_DISABLED_ALPHA,
    ELI_STYLE_VAR_WINDOW_PADDING,
    ELI_STYLE_VAR_WINDOW_ROUNDING,
    ELI_STYLE_VAR_WINDOW_BORDER_SIZE,
    ELI_STYLE_VAR_WINDOW_MIN_SIZE,
    ELI_STYLE_VAR_WINDOW_TITLE_ALIGN,
    ELI_STYLE_VAR_CHILD_ROUNDING,
    ELI_STYLE_VAR_CHILD_BORDER_SIZE,
    ELI_STYLE_VAR_POPUP_ROUNDING,
    ELI_STYLE_VAR_POPUP_BORDER_SIZE,
    ELI_STYLE_VAR_FRAME_PADDING,
    ELI_STYLE_VAR_FRAME_ROUNDING,
    ELI_STYLE_VAR_FRAME_BORDER_SIZE,
    ELI_STYLE_VAR_ITEM_SPACING,
    ELI_STYLE_VAR_ITEM_INNER_SPACING,
    ELI_STYLE_VAR_INDENT_SPACING,
    ELI_STYLE_VAR_CELL_PADDING,
    ELI_STYLE_VAR_SCROLLBAR_SIZE,
    ELI_STYLE_VAR_SCROLLBAR_ROUNDING,
    ELI_STYLE_VAR_GRAB_MIN_SIZE,
    ELI_STYLE_VAR_GRAB_ROUNDING,
    ELI_STYLE_VAR_TAB_ROUNDING,
    ELI_STYLE_VAR_TAB_BORDER_SIZE,
    ELI_STYLE_VAR_TAB_BAR_BORDER_SIZE,
    ELI_STYLE_VAR_TAB_BAR_OVERLINE_SIZE,
    ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_ANGLE,
    ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_TEXT_ALIGN,
    ELI_STYLE_VAR_BUTTON_TEXT_ALIGN,
    ELI_STYLE_VAR_SELECTABLE_TEXT_ALIGN,
    ELI_STYLE_VAR_SEPARATOR_TEXT_BORDER_SIZE,
    ELI_STYLE_VAR_SEPARATOR_TEXT_ALIGN,
    ELI_STYLE_VAR_SEPARATOR_TEXT_PADDING,
    ELI_STYLE_VAR_COUNT
};
```

---

## Core Types

### Vectors and Rects

```c
typedef struct { float x, y; } eli_vec2;
typedef struct { float x, y, z, w; } eli_vec4;
typedef struct { float x, y, w, h; } eli_rect;
```

### Colors

```c
typedef uint32_t eli_col32;

#define ELI_COL32(r, g, b, a) (((uint32_t)(a)<<24) | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(r))
#define ELI_COL32_WHITE     0xFFFFFFFF
#define ELI_COL32_BLACK     0x000000FF
#define ELI_COL32_BLACK_TRANS 0x00000000
```

### ID Type

```c
typedef uint32_t eli_id;
```

### Draw Types

```c
typedef struct {
    eli_rect clip_rect;
    uint32_t texture_id;
    uint32_t vtx_offset;
    uint32_t idx_offset;
    uint32_t elem_count;
    void* user_callback;
    void* user_callback_data;
} eli_draw_cmd;

typedef struct {
    float x, y;
    float u, v;
    eli_col32 col;
} eli_draw_vert;

typedef uint16_t eli_draw_idx;
```

### Draw List

```c
typedef struct {
    eli_draw_cmd* cmds;
    uint32_t cmd_count;
    uint32_t cmd_capacity;

    eli_draw_vert* vtx;
    uint32_t vtx_count;
    uint32_t vtx_capacity;

    eli_draw_idx* idx;
    uint32_t idx_count;
    uint32_t idx_capacity;

    eli_draw_list_flags flags;

    // Path building
    eli_vec2* path;
    int path_count;
    int path_capacity;

    // Clip rect stack
    eli_rect* clip_rect_stack;
    int clip_rect_stack_count;

    // Texture stack
    uint32_t* texture_stack;
    int texture_stack_count;
} eli_draw_list;
```

### Draw Data

```c
typedef struct {
    bool valid;
    int cmd_lists_count;
    eli_draw_list** cmd_lists;
    int total_idx_count;
    int total_vtx_count;
    eli_vec2 display_pos;
    eli_vec2 display_size;
    eli_vec2 framebuffer_scale;
} eli_draw_data;
```

### IO Structure

```c
typedef struct {
    // Configuration
    eli_vec2 display_size;
    eli_vec2 display_framebuffer_scale;
    float delta_time;
    float ini_saving_rate;
    const char* ini_filename;
    const char* log_filename;
    void* user_data;
    eli_font_atlas* fonts;
    float font_global_scale;
    bool font_allow_user_scaling;
    eli_font* font_default;

    // Configuration flags
    eli_config_flags config_flags;
    eli_backend_flags backend_flags;

    // Mouse input
    eli_vec2 mouse_pos;
    bool mouse_down[5];
    float mouse_wheel;
    float mouse_wheel_h;
    eli_mouse_cursor mouse_draw_cursor;

    // Keyboard input
    bool key_ctrl;
    bool key_shift;
    bool key_alt;
    bool key_super;
    bool keys_down[ELI_KEY_COUNT];

    // Text input
    uint16_t input_queue_characters[16];
    int input_queue_characters_count;

    // Output
    bool want_capture_mouse;
    bool want_capture_keyboard;
    bool want_text_input;
    bool want_set_mouse_pos;
    bool want_save_ini_settings;
    bool nav_active;
    bool nav_visible;
    float framerate;
    int metrics_render_vertices;
    int metrics_render_indices;
    int metrics_render_windows;
    int metrics_active_windows;

    // Mouse cursor position (set by backend)
    eli_vec2 mouse_pos_prev;
    eli_vec2 mouse_delta;
    bool mouse_clicked[5];
    bool mouse_double_clicked[5];
    uint16_t mouse_clicked_count[5];
    bool mouse_released[5];
    bool mouse_down_owned[5];
    float mouse_down_duration[5];
    float mouse_down_duration_prev[5];
    float mouse_drag_max_distance_sqr[5];

    // Keyboard
    float key_down_duration[ELI_KEY_COUNT];
    float key_down_duration_prev[ELI_KEY_COUNT];

    // Platform functions (callbacks)
    const char* (*get_clipboard_text_fn)(void* user_data);
    void (*set_clipboard_text_fn)(void* user_data, const char* text);
    void* clipboard_user_data;
} eli_io;
```

### Style Structure

```c
typedef struct {
    // Main
    float alpha;
    float disabled_alpha;
    eli_vec2 window_padding;
    float window_rounding;
    float window_border_size;
    eli_vec2 window_min_size;
    eli_vec2 window_title_align;
    eli_dir window_menu_button_position;
    float child_rounding;
    float child_border_size;
    float popup_rounding;
    float popup_border_size;
    eli_vec2 frame_padding;
    float frame_rounding;
    float frame_border_size;
    eli_vec2 item_spacing;
    eli_vec2 item_inner_spacing;
    eli_vec2 cell_padding;
    eli_vec2 touch_extra_padding;
    float indent_spacing;
    float columns_min_spacing;
    float scrollbar_size;
    float scrollbar_rounding;
    float grab_min_size;
    float grab_rounding;
    float log_slider_deadzone;
    float tab_rounding;
    float tab_border_size;
    float tab_min_width_for_close_button;
    float tab_bar_border_size;
    float tab_bar_overline_size;
    float table_angled_headers_angle;
    eli_vec2 table_angled_headers_text_align;
    eli_dir color_button_position;
    eli_vec2 button_text_align;
    eli_vec2 selectable_text_align;
    float separator_text_border_size;
    eli_vec2 separator_text_align;
    eli_vec2 separator_text_padding;
    eli_vec2 display_window_padding;
    eli_vec2 display_safe_area_padding;
    float docking_separator_size;
    float mouse_cursor_scale;
    bool anti_aliased_lines;
    bool anti_aliased_lines_use_tex;
    bool anti_aliased_fill;
    float curve_tessellation_tol;
    float circle_tessellation_max_error;
    eli_col32 colors[ELI_COL_COUNT];

    // Hover delay settings
    float hover_stationary_delay;
    float hover_delay_short;
    float hover_delay_normal;
    eli_hovered_flags hover_flags_for_tooltip_mouse;
    eli_hovered_flags hover_flags_for_tooltip_nav;
} eli_style;
```

### Payload (Drag & Drop)

```c
typedef struct {
    void* data;
    int data_size;
    eli_id source_id;
    eli_id source_parent_id;
    int data_frame_count;
    char data_type[32 + 1];
    bool preview;
    bool delivery;
} eli_payload;
```

### Table Sort Specs

```c
typedef struct {
    eli_id column_user_id;
    int16_t column_index;
    int16_t sort_order;
    eli_sort_direction sort_direction;
} eli_table_column_sort_specs;

typedef struct {
    const eli_table_column_sort_specs* specs;
    int specs_count;
    bool specs_dirty;
} eli_table_sort_specs;
```

### List Clipper

```c
typedef struct {
    eli_context* ctx;
    int display_start;
    int display_end;
    int items_count;
    float items_height;
    float start_pos_y;
    double start_seek_offset_y;
    void* temp_data;
} eli_list_clipper;
```

### Input Text Callback Data

```c
typedef struct {
    eli_context* ctx;
    eli_input_text_flags event_flag;
    eli_input_text_flags flags;
    void* user_data;

    // CharFilter event
    uint16_t event_char;

    // Completion/History/Always
    eli_key event_key;
    char* buf;
    int buf_text_len;
    int buf_size;
    bool buf_dirty;
    int cursor_pos;
    int selection_start;
    int selection_end;
} eli_input_text_callback_data;

typedef int (*eli_input_text_callback)(eli_input_text_callback_data* data);
```

### Storage

```c
typedef struct {
    eli_id key;
    union { int val_i; float val_f; void* val_p; };
} eli_storage_pair;

typedef struct {
    eli_storage_pair* data;
    int size;
    int capacity;
} eli_storage;
```

### Text Filter

```c
typedef struct {
    char input_buf[256];
    // ... filter implementation
    int count_grep;
} eli_text_filter;
```

---

## Context & Lifecycle API

```c
// Context
eli_context* eli_create_context(void);
void         eli_destroy_context(eli_context* ctx);
eli_context* eli_get_current_context(void);
void         eli_set_current_context(eli_context* ctx);

// Frame
void         eli_new_frame(void);
void         eli_end_frame(void);
void         eli_render(void);
eli_draw_data* eli_get_draw_data(void);

// Demo/Debug
void         eli_show_demo_window(bool* p_open);
void         eli_show_metrics_window(bool* p_open);
void         eli_show_debug_log_window(bool* p_open);
void         eli_show_id_stack_tool_window(bool* p_open);
void         eli_show_about_window(bool* p_open);
void         eli_show_style_editor(eli_style* ref);
bool         eli_show_style_selector(const char* label);
void         eli_show_font_selector(const char* label);
void         eli_show_user_guide(void);
const char*  eli_get_version(void);
```

---

## Windows API

```c
// Main windows
bool eli_begin(const char* name, bool* p_open, eli_window_flags flags);
void eli_end(void);

// Child windows
bool eli_begin_child(const char* str_id, eli_vec2 size, eli_child_flags child_flags, eli_window_flags window_flags);
bool eli_begin_child_id(eli_id id, eli_vec2 size, eli_child_flags child_flags, eli_window_flags window_flags);
void eli_end_child(void);

// Window state
bool eli_is_window_appearing(void);
bool eli_is_window_collapsed(void);
bool eli_is_window_focused(eli_focused_flags flags);
bool eli_is_window_hovered(eli_hovered_flags flags);
eli_draw_list* eli_get_window_draw_list(void);
eli_vec2 eli_get_window_pos(void);
eli_vec2 eli_get_window_size(void);
float eli_get_window_width(void);
float eli_get_window_height(void);

// Window manipulation
void eli_set_next_window_pos(eli_vec2 pos, eli_cond cond, eli_vec2 pivot);
void eli_set_next_window_size(eli_vec2 size, eli_cond cond);
void eli_set_next_window_size_constraints(eli_vec2 size_min, eli_vec2 size_max);
void eli_set_next_window_content_size(eli_vec2 size);
void eli_set_next_window_collapsed(bool collapsed, eli_cond cond);
void eli_set_next_window_focus(void);
void eli_set_next_window_scroll(eli_vec2 scroll);
void eli_set_next_window_bg_alpha(float alpha);
void eli_set_window_pos(eli_vec2 pos, eli_cond cond);
void eli_set_window_size(eli_vec2 size, eli_cond cond);
void eli_set_window_collapsed(bool collapsed, eli_cond cond);
void eli_set_window_focus(void);
void eli_set_window_font_scale(float scale);
void eli_set_window_pos_str(const char* name, eli_vec2 pos, eli_cond cond);
void eli_set_window_size_str(const char* name, eli_vec2 size, eli_cond cond);
void eli_set_window_collapsed_str(const char* name, bool collapsed, eli_cond cond);
void eli_set_window_focus_str(const char* name);

// Scrolling
float eli_get_scroll_x(void);
float eli_get_scroll_y(void);
void  eli_set_scroll_x(float scroll_x);
void  eli_set_scroll_y(float scroll_y);
float eli_get_scroll_max_x(void);
float eli_get_scroll_max_y(void);
void  eli_set_scroll_here_x(float center_x_ratio);
void  eli_set_scroll_here_y(float center_y_ratio);
void  eli_set_scroll_from_pos_x(float local_x, float center_x_ratio);
void  eli_set_scroll_from_pos_y(float local_y, float center_y_ratio);
```

---

## Layout API

```c
// Cursor / Layout
void eli_separator(void);
void eli_same_line(float offset_from_start_x, float spacing);
void eli_new_line(void);
void eli_spacing(void);
void eli_dummy(eli_vec2 size);
void eli_indent(float indent_w);
void eli_unindent(float indent_w);
void eli_begin_group(void);
void eli_end_group(void);
eli_vec2 eli_get_cursor_pos(void);
float eli_get_cursor_pos_x(void);
float eli_get_cursor_pos_y(void);
void eli_set_cursor_pos(eli_vec2 local_pos);
void eli_set_cursor_pos_x(float local_x);
void eli_set_cursor_pos_y(float local_y);
eli_vec2 eli_get_cursor_start_pos(void);
eli_vec2 eli_get_cursor_screen_pos(void);
void eli_set_cursor_screen_pos(eli_vec2 pos);
void eli_align_text_to_frame_padding(void);
float eli_get_text_line_height(void);
float eli_get_text_line_height_with_spacing(void);
float eli_get_frame_height(void);
float eli_get_frame_height_with_spacing(void);

// Content region
eli_vec2 eli_get_content_region_avail(void);
eli_vec2 eli_get_content_region_max(void);
eli_vec2 eli_get_window_content_region_min(void);
eli_vec2 eli_get_window_content_region_max(void);

// Item width
void eli_push_item_width(float item_width);
void eli_pop_item_width(void);
void eli_set_next_item_width(float item_width);
float eli_calc_item_width(void);

// Text wrapping
void eli_push_text_wrap_pos(float wrap_local_pos_x);
void eli_pop_text_wrap_pos(void);
```

---

## ID Stack API

```c
void eli_push_id(const char* str_id);
void eli_push_id_str(const char* str_id_begin, const char* str_id_end);
void eli_push_id_ptr(const void* ptr_id);
void eli_push_id_int(int int_id);
void eli_pop_id(void);
eli_id eli_get_id(const char* str_id);
eli_id eli_get_id_str(const char* str_id_begin, const char* str_id_end);
eli_id eli_get_id_ptr(const void* ptr_id);
eli_id eli_get_id_int(int int_id);
```

---

## Widgets - Text

```c
void eli_text_unformatted(const char* text, const char* text_end);
void eli_text(const char* fmt, ...);
void eli_text_v(const char* fmt, va_list args);
void eli_text_colored(eli_vec4 col, const char* fmt, ...);
void eli_text_colored_v(eli_vec4 col, const char* fmt, va_list args);
void eli_text_disabled(const char* fmt, ...);
void eli_text_disabled_v(const char* fmt, va_list args);
void eli_text_wrapped(const char* fmt, ...);
void eli_text_wrapped_v(const char* fmt, va_list args);
void eli_label_text(const char* label, const char* fmt, ...);
void eli_label_text_v(const char* label, const char* fmt, va_list args);
void eli_bullet_text(const char* fmt, ...);
void eli_bullet_text_v(const char* fmt, va_list args);
void eli_separator_text(const char* label);
```

---

## Widgets - Buttons

```c
bool eli_button(const char* label);
bool eli_button_ex(const char* label, eli_vec2 size, eli_button_flags flags);
bool eli_small_button(const char* label);
bool eli_invisible_button(const char* str_id, eli_vec2 size, eli_button_flags flags);
bool eli_arrow_button(const char* str_id, eli_dir dir);
bool eli_checkbox(const char* label, bool* v);
bool eli_checkbox_flags_int(const char* label, int* flags, int flags_value);
bool eli_checkbox_flags_uint(const char* label, unsigned int* flags, unsigned int flags_value);
bool eli_radio_button(const char* label, bool active);
bool eli_radio_button_int(const char* label, int* v, int v_button);
void eli_progress_bar(float fraction, eli_vec2 size_arg, const char* overlay);
void eli_bullet(void);
bool eli_text_link(const char* label);
void eli_text_link_open_url(const char* label, const char* url);
```

---

## Widgets - Images

```c
void eli_image(uint32_t user_texture_id, eli_vec2 image_size, eli_vec2 uv0, eli_vec2 uv1, eli_vec4 tint_col, eli_vec4 border_col);
bool eli_image_button(const char* str_id, uint32_t user_texture_id, eli_vec2 image_size, eli_vec2 uv0, eli_vec2 uv1, eli_vec4 bg_col, eli_vec4 tint_col);
```

---

## Widgets - Combo

```c
bool eli_begin_combo(const char* label, const char* preview_value, eli_combo_flags flags);
void eli_end_combo(void);
bool eli_combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items);
bool eli_combo_str(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items);
bool eli_combo_fn(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items);
```

---

## Widgets - Drag

```c
bool eli_drag_float(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_drag_float2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_drag_float3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_drag_float4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_drag_float_range2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, eli_slider_flags flags);
bool eli_drag_int(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_drag_int2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_drag_int3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_drag_int4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_drag_int_range2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max, eli_slider_flags flags);
bool eli_drag_scalar(const char* label, eli_data_type data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, eli_slider_flags flags);
bool eli_drag_scalar_n(const char* label, eli_data_type data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, eli_slider_flags flags);
```

---

## Widgets - Slider

```c
bool eli_slider_float(const char* label, float* v, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_slider_float2(const char* label, float v[2], float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_slider_float3(const char* label, float v[3], float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_slider_float4(const char* label, float v[4], float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_slider_angle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, eli_slider_flags flags);
bool eli_slider_int(const char* label, int* v, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_slider_int2(const char* label, int v[2], int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_slider_int3(const char* label, int v[3], int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_slider_int4(const char* label, int v[4], int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_slider_scalar(const char* label, eli_data_type data_type, void* p_data, const void* p_min, const void* p_max, const char* format, eli_slider_flags flags);
bool eli_slider_scalar_n(const char* label, eli_data_type data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, eli_slider_flags flags);
bool eli_v_slider_float(const char* label, eli_vec2 size, float* v, float v_min, float v_max, const char* format, eli_slider_flags flags);
bool eli_v_slider_int(const char* label, eli_vec2 size, int* v, int v_min, int v_max, const char* format, eli_slider_flags flags);
bool eli_v_slider_scalar(const char* label, eli_vec2 size, eli_data_type data_type, void* p_data, const void* p_min, const void* p_max, const char* format, eli_slider_flags flags);
```

---

## Widgets - Input

```c
bool eli_input_text(const char* label, char* buf, size_t buf_size, eli_input_text_flags flags, eli_input_text_callback callback, void* user_data);
bool eli_input_text_multiline(const char* label, char* buf, size_t buf_size, eli_vec2 size, eli_input_text_flags flags, eli_input_text_callback callback, void* user_data);
bool eli_input_text_with_hint(const char* label, const char* hint, char* buf, size_t buf_size, eli_input_text_flags flags, eli_input_text_callback callback, void* user_data);
bool eli_input_float(const char* label, float* v, float step, float step_fast, const char* format, eli_input_text_flags flags);
bool eli_input_float2(const char* label, float v[2], const char* format, eli_input_text_flags flags);
bool eli_input_float3(const char* label, float v[3], const char* format, eli_input_text_flags flags);
bool eli_input_float4(const char* label, float v[4], const char* format, eli_input_text_flags flags);
bool eli_input_int(const char* label, int* v, int step, int step_fast, eli_input_text_flags flags);
bool eli_input_int2(const char* label, int v[2], eli_input_text_flags flags);
bool eli_input_int3(const char* label, int v[3], eli_input_text_flags flags);
bool eli_input_int4(const char* label, int v[4], eli_input_text_flags flags);
bool eli_input_double(const char* label, double* v, double step, double step_fast, const char* format, eli_input_text_flags flags);
bool eli_input_scalar(const char* label, eli_data_type data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, eli_input_text_flags flags);
bool eli_input_scalar_n(const char* label, eli_data_type data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, eli_input_text_flags flags);
```

---

## Widgets - Color

```c
bool eli_color_edit3(const char* label, float col[3], eli_color_edit_flags flags);
bool eli_color_edit4(const char* label, float col[4], eli_color_edit_flags flags);
bool eli_color_picker3(const char* label, float col[3], eli_color_edit_flags flags);
bool eli_color_picker4(const char* label, float col[4], eli_color_edit_flags flags, const float* ref_col);
bool eli_color_button(const char* desc_id, eli_vec4 col, eli_color_edit_flags flags, eli_vec2 size);
void eli_set_color_edit_options(eli_color_edit_flags flags);
```

---

## Widgets - Trees

```c
bool eli_tree_node(const char* label);
bool eli_tree_node_str(const char* str_id, const char* fmt, ...);
bool eli_tree_node_ptr(const void* ptr_id, const char* fmt, ...);
bool eli_tree_node_v(const char* str_id, const char* fmt, va_list args);
bool eli_tree_node_ex(const char* label, eli_tree_node_flags flags);
bool eli_tree_node_ex_str(const char* str_id, eli_tree_node_flags flags, const char* fmt, ...);
bool eli_tree_node_ex_ptr(const void* ptr_id, eli_tree_node_flags flags, const char* fmt, ...);
bool eli_tree_node_ex_v(const char* str_id, eli_tree_node_flags flags, const char* fmt, va_list args);
void eli_tree_push(const char* str_id);
void eli_tree_push_ptr(const void* ptr_id);
void eli_tree_pop(void);
float eli_get_tree_node_to_label_spacing(void);
bool eli_collapsing_header(const char* label, eli_tree_node_flags flags);
bool eli_collapsing_header_bool(const char* label, bool* p_visible, eli_tree_node_flags flags);
void eli_set_next_item_open(bool is_open, eli_cond cond);
void eli_set_next_item_storage_id(eli_id storage_id);
```

---

## Widgets - Selectables

```c
bool eli_selectable(const char* label, bool selected, eli_selectable_flags flags, eli_vec2 size);
bool eli_selectable_bool(const char* label, bool* p_selected, eli_selectable_flags flags, eli_vec2 size);
```

---

## Widgets - List Boxes

```c
bool eli_begin_list_box(const char* label, eli_vec2 size);
void eli_end_list_box(void);
bool eli_list_box(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items);
bool eli_list_box_fn(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int height_in_items);
```

---

## Widgets - Data Plotting

```c
void eli_plot_lines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, eli_vec2 graph_size, int stride);
void eli_plot_lines_fn(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, eli_vec2 graph_size);
void eli_plot_histogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, eli_vec2 graph_size, int stride);
void eli_plot_histogram_fn(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, eli_vec2 graph_size);
```

---

## Widgets - Value Display

```c
void eli_value_bool(const char* prefix, bool b);
void eli_value_int(const char* prefix, int v);
void eli_value_uint(const char* prefix, unsigned int v);
void eli_value_float(const char* prefix, float v, const char* float_format);
```

---

## Widgets - Menus

```c
bool eli_begin_menu_bar(void);
void eli_end_menu_bar(void);
bool eli_begin_main_menu_bar(void);
void eli_end_main_menu_bar(void);
bool eli_begin_menu(const char* label, bool enabled);
void eli_end_menu(void);
bool eli_menu_item(const char* label, const char* shortcut, bool selected, bool enabled);
bool eli_menu_item_bool(const char* label, const char* shortcut, bool* p_selected, bool enabled);
```

---

## Tooltips

```c
bool eli_begin_tooltip(void);
void eli_end_tooltip(void);
void eli_set_tooltip(const char* fmt, ...);
void eli_set_tooltip_v(const char* fmt, va_list args);
bool eli_begin_item_tooltip(void);
void eli_set_item_tooltip(const char* fmt, ...);
void eli_set_item_tooltip_v(const char* fmt, va_list args);
```

---

## Popups & Modals

```c
bool eli_begin_popup(const char* str_id, eli_window_flags flags);
bool eli_begin_popup_modal(const char* name, bool* p_open, eli_window_flags flags);
void eli_end_popup(void);
void eli_open_popup(const char* str_id, eli_popup_flags popup_flags);
void eli_open_popup_id(eli_id id, eli_popup_flags popup_flags);
void eli_open_popup_on_item_click(const char* str_id, eli_popup_flags popup_flags);
void eli_close_current_popup(void);
bool eli_begin_popup_context_item(const char* str_id, eli_popup_flags popup_flags);
bool eli_begin_popup_context_window(const char* str_id, eli_popup_flags popup_flags);
bool eli_begin_popup_context_void(const char* str_id, eli_popup_flags popup_flags);
bool eli_is_popup_open(const char* str_id, eli_popup_flags flags);
```

---

## Tables

```c
bool eli_begin_table(const char* str_id, int columns, eli_table_flags flags, eli_vec2 outer_size, float inner_width);
void eli_end_table(void);
void eli_table_next_row(eli_table_row_flags row_flags, float min_row_height);
bool eli_table_next_column(void);
bool eli_table_set_column_index(int column_n);

// Headers
void eli_table_setup_column(const char* label, eli_table_column_flags flags, float init_width_or_weight, eli_id user_id);
void eli_table_setup_scroll_freeze(int cols, int rows);
void eli_table_header(const char* label);
void eli_table_headers_row(void);
void eli_table_angled_headers_row(void);

// Sorting & Info
eli_table_sort_specs* eli_table_get_sort_specs(void);
int eli_table_get_column_count(void);
int eli_table_get_column_index(void);
int eli_table_get_row_index(void);
const char* eli_table_get_column_name(int column_n);
eli_table_column_flags eli_table_get_column_flags(int column_n);
void eli_table_set_column_enabled(int column_n, bool v);
int eli_table_get_hovered_column(void);
void eli_table_set_bg_color(eli_table_bg_target target, eli_col32 color, int column_n);
```

---

## Tab Bars

```c
bool eli_begin_tab_bar(const char* str_id, eli_tab_bar_flags flags);
void eli_end_tab_bar(void);
bool eli_begin_tab_item(const char* label, bool* p_open, eli_tab_item_flags flags);
void eli_end_tab_item(void);
bool eli_tab_item_button(const char* label, eli_tab_item_flags flags);
void eli_set_tab_item_closed(const char* tab_or_docked_window_label);
```

---

## Drag & Drop

```c
bool eli_begin_drag_drop_source(eli_drag_drop_flags flags);
bool eli_set_drag_drop_payload(const char* type, const void* data, size_t sz, eli_cond cond);
void eli_end_drag_drop_source(void);
bool eli_begin_drag_drop_target(void);
const eli_payload* eli_accept_drag_drop_payload(const char* type, eli_drag_drop_flags flags);
void eli_end_drag_drop_target(void);
const eli_payload* eli_get_drag_drop_payload(void);
```

---

## Disabling

```c
void eli_begin_disabled(bool disabled);
void eli_end_disabled(void);
```

---

## Clipping

```c
void eli_push_clip_rect(eli_vec2 clip_rect_min, eli_vec2 clip_rect_max, bool intersect_with_current_clip_rect);
void eli_pop_clip_rect(void);
```

---

## Focus & Activation

```c
void eli_set_item_default_focus(void);
void eli_set_keyboard_focus_here(int offset);
```

---

## Item Status Queries

```c
bool eli_is_item_hovered(eli_hovered_flags flags);
bool eli_is_item_active(void);
bool eli_is_item_focused(void);
bool eli_is_item_clicked(eli_mouse_button mouse_button);
bool eli_is_item_visible(void);
bool eli_is_item_edited(void);
bool eli_is_item_activated(void);
bool eli_is_item_deactivated(void);
bool eli_is_item_deactivated_after_edit(void);
bool eli_is_item_toggled_open(void);
bool eli_is_any_item_hovered(void);
bool eli_is_any_item_active(void);
bool eli_is_any_item_focused(void);
eli_id eli_get_item_id(void);
eli_vec2 eli_get_item_rect_min(void);
eli_vec2 eli_get_item_rect_max(void);
eli_vec2 eli_get_item_rect_size(void);
```

---

## Viewports

```c
eli_viewport* eli_get_main_viewport(void);
```

---

## Background/Foreground Draw Lists

```c
eli_draw_list* eli_get_background_draw_list(void);
eli_draw_list* eli_get_foreground_draw_list(void);
```

---

## Misc Utilities

```c
bool eli_is_rect_visible(eli_vec2 size);
bool eli_is_rect_visible_vec2(eli_vec2 rect_min, eli_vec2 rect_max);
double eli_get_time(void);
int eli_get_frame_count(void);
const char* eli_get_style_color_name(eli_col idx);
void eli_set_state_storage(eli_storage* storage);
eli_storage* eli_get_state_storage(void);
eli_vec2 eli_calc_text_size(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width);
```

---

## Color Utilities

```c
eli_vec4 eli_color_convert_u32_to_float4(eli_col32 in);
eli_col32 eli_color_convert_float4_to_u32(eli_vec4 in);
void eli_color_convert_rgb_to_hsv(float r, float g, float b, float* out_h, float* out_s, float* out_v);
void eli_color_convert_hsv_to_rgb(float h, float s, float v, float* out_r, float* out_g, float* out_b);
```

---

## Input Utilities - Keyboard

```c
bool eli_is_key_down(eli_key key);
bool eli_is_key_pressed(eli_key key, bool repeat);
bool eli_is_key_released(eli_key key);
bool eli_is_key_chord_pressed(int key_chord);
int eli_get_key_pressed_amount(eli_key key, float repeat_delay, float rate);
const char* eli_get_key_name(eli_key key);
void eli_set_next_frame_want_capture_keyboard(bool want_capture_keyboard);
```

---

## Input Utilities - Shortcuts

```c
bool eli_shortcut(int key_chord, eli_input_flags flags);
void eli_set_next_item_shortcut(int key_chord, eli_input_flags flags);
```

---

## Input Utilities - Mouse

```c
bool eli_is_mouse_down(eli_mouse_button button);
bool eli_is_mouse_clicked(eli_mouse_button button, bool repeat);
bool eli_is_mouse_released(eli_mouse_button button);
bool eli_is_mouse_double_clicked(eli_mouse_button button);
int eli_get_mouse_clicked_count(eli_mouse_button button);
bool eli_is_mouse_hovering_rect(eli_vec2 r_min, eli_vec2 r_max, bool clip);
bool eli_is_mouse_pos_valid(const eli_vec2* mouse_pos);
bool eli_is_any_mouse_down(void);
eli_vec2 eli_get_mouse_pos(void);
eli_vec2 eli_get_mouse_pos_on_opening_current_popup(void);
bool eli_is_mouse_dragging(eli_mouse_button button, float lock_threshold);
eli_vec2 eli_get_mouse_drag_delta(eli_mouse_button button, float lock_threshold);
void eli_reset_mouse_drag_delta(eli_mouse_button button);
eli_mouse_cursor eli_get_mouse_cursor(void);
void eli_set_mouse_cursor(eli_mouse_cursor cursor_type);
void eli_set_next_frame_want_capture_mouse(bool want_capture_mouse);
```

---

## Clipboard

```c
const char* eli_get_clipboard_text(void);
void eli_set_clipboard_text(const char* text);
```

---

## Settings / INI

```c
void eli_load_ini_settings_from_disk(const char* ini_filename);
void eli_load_ini_settings_from_memory(const char* ini_data, size_t ini_size);
void eli_save_ini_settings_to_disk(const char* ini_filename);
const char* eli_save_ini_settings_to_memory(size_t* out_ini_size);
```

---

## Logging

```c
void eli_log_to_tty(int auto_open_depth);
void eli_log_to_file(int auto_open_depth, const char* filename);
void eli_log_to_clipboard(int auto_open_depth);
void eli_log_finish(void);
void eli_log_buttons(void);
void eli_log_text(const char* fmt, ...);
void eli_log_text_v(const char* fmt, va_list args);
```

---

## Memory

```c
void eli_set_allocator_functions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data);
void eli_get_allocator_functions(void* (**alloc_func)(size_t sz, void* user_data), void (**free_func)(void* ptr, void* user_data), void** user_data);
void* eli_mem_alloc(size_t size);
void eli_mem_free(void* ptr);
```

---

## Style

```c
eli_style* eli_get_style(void);
void eli_style_colors_dark(eli_style* dst);
void eli_style_colors_light(eli_style* dst);
void eli_style_colors_classic(eli_style* dst);
void eli_push_style_color(eli_col idx, eli_col32 col);
void eli_push_style_color_vec4(eli_col idx, eli_vec4 col);
void eli_pop_style_color(int count);
void eli_push_style_var(eli_style_var idx, float val);
void eli_push_style_var_vec2(eli_style_var idx, eli_vec2 val);
void eli_pop_style_var(int count);
void eli_push_item_flag(eli_item_flags option, bool enabled);
void eli_pop_item_flag(void);
eli_col32 eli_get_color_u32(eli_col idx, float alpha_mul);
eli_col32 eli_get_color_u32_vec4(eli_vec4 col);
eli_col32 eli_get_color_u32_col32(eli_col32 col, float alpha_mul);
eli_vec4 eli_get_style_color_vec4(eli_col idx);
```

---

## Fonts

```c
void eli_push_font(eli_font* font);
void eli_pop_font(void);
eli_font* eli_get_font(void);
float eli_get_font_size(void);
eli_vec2 eli_get_font_tex_uv_white_pixel(void);
```

---

## Draw List API

```c
// Primitives
void eli_draw_list_add_line(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_col32 col, float thickness);
void eli_draw_list_add_rect(eli_draw_list* list, eli_vec2 p_min, eli_vec2 p_max, eli_col32 col, float rounding, eli_draw_flags flags, float thickness);
void eli_draw_list_add_rect_filled(eli_draw_list* list, eli_vec2 p_min, eli_vec2 p_max, eli_col32 col, float rounding, eli_draw_flags flags);
void eli_draw_list_add_rect_filled_multi_color(eli_draw_list* list, eli_vec2 p_min, eli_vec2 p_max, eli_col32 col_upr_left, eli_col32 col_upr_right, eli_col32 col_bot_right, eli_col32 col_bot_left);
void eli_draw_list_add_quad(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, eli_col32 col, float thickness);
void eli_draw_list_add_quad_filled(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, eli_col32 col);
void eli_draw_list_add_triangle(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_col32 col, float thickness);
void eli_draw_list_add_triangle_filled(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_col32 col);
void eli_draw_list_add_circle(eli_draw_list* list, eli_vec2 center, float radius, eli_col32 col, int num_segments, float thickness);
void eli_draw_list_add_circle_filled(eli_draw_list* list, eli_vec2 center, float radius, eli_col32 col, int num_segments);
void eli_draw_list_add_ngon(eli_draw_list* list, eli_vec2 center, float radius, eli_col32 col, int num_segments, float thickness);
void eli_draw_list_add_ngon_filled(eli_draw_list* list, eli_vec2 center, float radius, eli_col32 col, int num_segments);
void eli_draw_list_add_ellipse(eli_draw_list* list, eli_vec2 center, eli_vec2 radius, eli_col32 col, float rot, int num_segments, float thickness);
void eli_draw_list_add_ellipse_filled(eli_draw_list* list, eli_vec2 center, eli_vec2 radius, eli_col32 col, float rot, int num_segments);
void eli_draw_list_add_text(eli_draw_list* list, eli_vec2 pos, eli_col32 col, const char* text_begin, const char* text_end);
void eli_draw_list_add_text_ex(eli_draw_list* list, eli_font* font, float font_size, eli_vec2 pos, eli_col32 col, const char* text_begin, const char* text_end, float wrap_width, const eli_vec4* cpu_fine_clip_rect);
void eli_draw_list_add_polyline(eli_draw_list* list, const eli_vec2* points, int num_points, eli_col32 col, eli_draw_flags flags, float thickness);
void eli_draw_list_add_convex_poly_filled(eli_draw_list* list, const eli_vec2* points, int num_points, eli_col32 col);
void eli_draw_list_add_bezier_cubic(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, eli_col32 col, float thickness, int num_segments);
void eli_draw_list_add_bezier_quadratic(eli_draw_list* list, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_col32 col, float thickness, int num_segments);

// Images
void eli_draw_list_add_image(eli_draw_list* list, uint32_t user_texture_id, eli_vec2 p_min, eli_vec2 p_max, eli_vec2 uv_min, eli_vec2 uv_max, eli_col32 col);
void eli_draw_list_add_image_quad(eli_draw_list* list, uint32_t user_texture_id, eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, eli_vec2 uv1, eli_vec2 uv2, eli_vec2 uv3, eli_vec2 uv4, eli_col32 col);
void eli_draw_list_add_image_rounded(eli_draw_list* list, uint32_t user_texture_id, eli_vec2 p_min, eli_vec2 p_max, eli_vec2 uv_min, eli_vec2 uv_max, eli_col32 col, float rounding, eli_draw_flags flags);

// Stateful Path API
void eli_draw_list_path_clear(eli_draw_list* list);
void eli_draw_list_path_line_to(eli_draw_list* list, eli_vec2 pos);
void eli_draw_list_path_line_to_merge_duplicate(eli_draw_list* list, eli_vec2 pos);
void eli_draw_list_path_fill_convex(eli_draw_list* list, eli_col32 col);
void eli_draw_list_path_stroke(eli_draw_list* list, eli_col32 col, eli_draw_flags flags, float thickness);
void eli_draw_list_path_arc_to(eli_draw_list* list, eli_vec2 center, float radius, float a_min, float a_max, int num_segments);
void eli_draw_list_path_arc_to_fast(eli_draw_list* list, eli_vec2 center, float radius, int a_min_of_12, int a_max_of_12);
void eli_draw_list_path_elliptical_arc_to(eli_draw_list* list, eli_vec2 center, eli_vec2 radius, float rot, float a_min, float a_max, int num_segments);
void eli_draw_list_path_bezier_cubic_curve_to(eli_draw_list* list, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4, int num_segments);
void eli_draw_list_path_bezier_quadratic_curve_to(eli_draw_list* list, eli_vec2 p2, eli_vec2 p3, int num_segments);
void eli_draw_list_path_rect(eli_draw_list* list, eli_vec2 rect_min, eli_vec2 rect_max, float rounding, eli_draw_flags flags);

// Advanced
void eli_draw_list_add_callback(eli_draw_list* list, void (*callback)(const eli_draw_list* parent_list, const eli_draw_cmd* cmd), void* callback_data);
void eli_draw_list_add_draw_cmd(eli_draw_list* list);
eli_draw_list* eli_draw_list_clone_output(eli_draw_list* list);

// Channels (for draw list splitting)
void eli_draw_list_channels_split(eli_draw_list* list, int count);
void eli_draw_list_channels_merge(eli_draw_list* list);
void eli_draw_list_channels_set_current(eli_draw_list* list, int n);

// Clip/Texture stack
void eli_draw_list_push_clip_rect(eli_draw_list* list, eli_vec2 clip_rect_min, eli_vec2 clip_rect_max, bool intersect_with_current_clip_rect);
void eli_draw_list_push_clip_rect_full_screen(eli_draw_list* list);
void eli_draw_list_pop_clip_rect(eli_draw_list* list);
void eli_draw_list_push_texture_id(eli_draw_list* list, uint32_t texture_id);
void eli_draw_list_pop_texture_id(eli_draw_list* list);
eli_vec2 eli_draw_list_get_clip_rect_min(eli_draw_list* list);
eli_vec2 eli_draw_list_get_clip_rect_max(eli_draw_list* list);

// Primitives reservation
void eli_draw_list_prim_reserve(eli_draw_list* list, int idx_count, int vtx_count);
void eli_draw_list_prim_unreserve(eli_draw_list* list, int idx_count, int vtx_count);
void eli_draw_list_prim_rect(eli_draw_list* list, eli_vec2 a, eli_vec2 b, eli_col32 col);
void eli_draw_list_prim_rect_uv(eli_draw_list* list, eli_vec2 a, eli_vec2 b, eli_vec2 uv_a, eli_vec2 uv_b, eli_col32 col);
void eli_draw_list_prim_quad_uv(eli_draw_list* list, eli_vec2 a, eli_vec2 b, eli_vec2 c, eli_vec2 d, eli_vec2 uv_a, eli_vec2 uv_b, eli_vec2 uv_c, eli_vec2 uv_d, eli_col32 col);
void eli_draw_list_prim_write_vtx(eli_draw_list* list, eli_vec2 pos, eli_vec2 uv, eli_col32 col);
void eli_draw_list_prim_write_idx(eli_draw_list* list, eli_draw_idx idx);
void eli_draw_list_prim_vtx(eli_draw_list* list, eli_vec2 pos, eli_vec2 uv, eli_col32 col);
```

---

## Font Atlas API

```c
eli_font* eli_font_atlas_add_font(eli_font_atlas* atlas, const eli_font_config* font_cfg);
eli_font* eli_font_atlas_add_font_default(eli_font_atlas* atlas, const eli_font_config* font_cfg);
eli_font* eli_font_atlas_add_font_from_file_ttf(eli_font_atlas* atlas, const char* filename, float size_pixels, const eli_font_config* font_cfg, const uint16_t* glyph_ranges);
eli_font* eli_font_atlas_add_font_from_memory_ttf(eli_font_atlas* atlas, void* font_data, int font_data_size, float size_pixels, const eli_font_config* font_cfg, const uint16_t* glyph_ranges);
eli_font* eli_font_atlas_add_font_from_memory_compressed_ttf(eli_font_atlas* atlas, const void* compressed_font_data, int compressed_font_data_size, float size_pixels, const eli_font_config* font_cfg, const uint16_t* glyph_ranges);
eli_font* eli_font_atlas_add_font_from_memory_compressed_base85_ttf(eli_font_atlas* atlas, const char* compressed_font_data_base85, float size_pixels, const eli_font_config* font_cfg, const uint16_t* glyph_ranges);
void eli_font_atlas_clear_input_data(eli_font_atlas* atlas);
void eli_font_atlas_clear_tex_data(eli_font_atlas* atlas);
void eli_font_atlas_clear_fonts(eli_font_atlas* atlas);
void eli_font_atlas_clear(eli_font_atlas* atlas);
bool eli_font_atlas_build(eli_font_atlas* atlas);
void eli_font_atlas_get_tex_data_as_alpha8(eli_font_atlas* atlas, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
void eli_font_atlas_get_tex_data_as_rgba32(eli_font_atlas* atlas, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
bool eli_font_atlas_is_built(eli_font_atlas* atlas);
void eli_font_atlas_set_tex_id(eli_font_atlas* atlas, uint32_t id);

// Glyph ranges
const uint16_t* eli_font_atlas_get_glyph_ranges_default(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_greek(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_korean(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_japanese(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_chinese_full(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_chinese_simplified_common(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_cyrillic(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_thai(eli_font_atlas* atlas);
const uint16_t* eli_font_atlas_get_glyph_ranges_vietnamese(eli_font_atlas* atlas);
```

---

## List Clipper API

```c
void eli_list_clipper_begin(eli_list_clipper* clipper, int items_count, float items_height);
void eli_list_clipper_end(eli_list_clipper* clipper);
bool eli_list_clipper_step(eli_list_clipper* clipper);
void eli_list_clipper_include_item_by_index(eli_list_clipper* clipper, int item_index);
void eli_list_clipper_include_items_by_index(eli_list_clipper* clipper, int item_begin, int item_end);
void eli_list_clipper_seek_cursor_for_item(eli_list_clipper* clipper, int item_index);
```

---

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

---

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

    eli_slider_float("Slider", &slider_value, 0.0f, 1.0f, "%.3f", 0);
    eli_checkbox("Checkbox", &checkbox_value);
    eli_input_text("Text", text_buf, sizeof(text_buf), 0, NULL, NULL);

    // Tables
    if (eli_begin_table("my_table", 3, ELI_TABLE_BORDERS, (eli_vec2){0, 0}, 0.0f)) {
        eli_table_setup_column("One", 0, 0.0f, 0);
        eli_table_setup_column("Two", 0, 0.0f, 0);
        eli_table_setup_column("Three", 0, 0.0f, 0);
        eli_table_headers_row();

        for (int row = 0; row < 4; row++) {
            eli_table_next_row(0, 0.0f);
            for (int col = 0; col < 3; col++) {
                eli_table_set_column_index(col);
                eli_text("Cell %d,%d", row, col);
            }
        }
        eli_end_table();
    }

    eli_end();

    eli_end_frame();
    eli_render();

    // Get draw data and render with your backend
    eli_draw_data* data = eli_get_draw_data();
    my_render_function(data);
}
```

---

## Memory Model

elimgui uses a simple arena/bump allocator model:

```c
// User provides memory
void eli_set_allocator_functions(
    void* (*alloc_func)(size_t sz, void* user_data),
    void (*free_func)(void* ptr, void* user_data),
    void* user_data
);

// Or use default (JAClibc malloc)
eli_context* eli_create_context(void);
void eli_destroy_context(eli_context* ctx);
```
