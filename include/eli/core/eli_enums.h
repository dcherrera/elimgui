/**
 * @file eli_enums.h
 * @brief Core enumerations and flag sets for elimgui, using Dear ImGui-compatible
 *        values: directions, conditions, data types, keys, mouse, color indices,
 *        style variables, and the window/child/item/config/backend/hovered flags.
 *
 * @status Phase 1 foundation enums in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_CORE_ELI_ENUMS_H
#define ELI_CORE_ELI_ENUMS_H

#include "eli_platform.h"

/* ---------------------------------------------------------------------------
 * Direction
 * ------------------------------------------------------------------------- */

typedef int eli_dir;
enum eli_dir_ {
    ELI_DIR_NONE  = -1,
    ELI_DIR_LEFT  = 0,
    ELI_DIR_RIGHT = 1,
    ELI_DIR_UP    = 2,
    ELI_DIR_DOWN  = 3,
    ELI_DIR_COUNT = 4
};

/* ---------------------------------------------------------------------------
 * Condition (for set-next-* style APIs)
 * ------------------------------------------------------------------------- */

typedef int eli_cond;
enum eli_cond_ {
    ELI_COND_NONE           = 0,
    ELI_COND_ALWAYS         = 1 << 0,
    ELI_COND_ONCE           = 1 << 1,
    ELI_COND_FIRST_USE_EVER = 1 << 2,
    ELI_COND_APPEARING      = 1 << 3
};

/* ---------------------------------------------------------------------------
 * Data type (for scalar widgets)
 * ------------------------------------------------------------------------- */

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

/* ---------------------------------------------------------------------------
 * Slider / drag flags (shared by eli_slider_* and eli_drag_* widgets)
 *
 * The public subset matches Dear ImGui's ImGuiSliderFlags. Private axis/read-only
 * bits used internally by the slider/drag behaviors live in eli_slider_behavior.h.
 * ------------------------------------------------------------------------- */

typedef int eli_slider_flags;
enum eli_slider_flags_ {
    ELI_SLIDER_NONE             = 0,
    ELI_SLIDER_LOGARITHMIC      = 1 << 5,  /* map value logarithmically along the track */
    ELI_SLIDER_NO_ROUND_TO_FORMAT = 1 << 6, /* do not round the value to the format precision */
    ELI_SLIDER_NO_INPUT         = 1 << 7,  /* disable Ctrl+Click / tab-to-type text input */
    ELI_SLIDER_WRAP_AROUND      = 1 << 8,  /* wrap past the bounds (drags only) */
    ELI_SLIDER_CLAMP_ON_INPUT   = 1 << 9,  /* clamp typed input to [min,max] */
    ELI_SLIDER_CLAMP_ZERO_RANGE = 1 << 10, /* clamp even when min == max == 0 */
    ELI_SLIDER_ALWAYS_CLAMP     = ELI_SLIDER_CLAMP_ON_INPUT | ELI_SLIDER_CLAMP_ZERO_RANGE
};

/* ---------------------------------------------------------------------------
 * Mouse buttons and cursors
 * ------------------------------------------------------------------------- */

typedef int eli_mouse_button;
enum eli_mouse_button_ {
    ELI_MOUSE_BUTTON_LEFT   = 0,
    ELI_MOUSE_BUTTON_RIGHT  = 1,
    ELI_MOUSE_BUTTON_MIDDLE = 2,
    ELI_MOUSE_BUTTON_COUNT  = 5
};

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

/* ---------------------------------------------------------------------------
 * Keys — full keyboard, keypad, modifiers, function keys, plus mouse/wheel
 * codes for unified input. ELI_KEY_COUNT marks the end of the "named key"
 * range; the mouse/wheel codes that follow are addressed separately.
 * ------------------------------------------------------------------------- */

typedef int eli_key;
enum eli_key_ {
    ELI_KEY_NONE = 0,
    /* Control / navigation / editing */
    ELI_KEY_TAB, ELI_KEY_LEFT_ARROW, ELI_KEY_RIGHT_ARROW, ELI_KEY_UP_ARROW, ELI_KEY_DOWN_ARROW,
    ELI_KEY_PAGE_UP, ELI_KEY_PAGE_DOWN, ELI_KEY_HOME, ELI_KEY_END,
    ELI_KEY_INSERT, ELI_KEY_DELETE, ELI_KEY_BACKSPACE, ELI_KEY_SPACE, ELI_KEY_ENTER,
    ELI_KEY_ESCAPE, ELI_KEY_APOSTROPHE, ELI_KEY_COMMA, ELI_KEY_MINUS, ELI_KEY_PERIOD,
    ELI_KEY_SLASH, ELI_KEY_SEMICOLON, ELI_KEY_EQUAL, ELI_KEY_LEFT_BRACKET, ELI_KEY_BACKSLASH,
    ELI_KEY_RIGHT_BRACKET, ELI_KEY_GRAVE_ACCENT, ELI_KEY_CAPS_LOCK, ELI_KEY_SCROLL_LOCK, ELI_KEY_NUM_LOCK,
    ELI_KEY_PRINT_SCREEN, ELI_KEY_PAUSE,
    /* Keypad */
    ELI_KEY_KEYPAD_0, ELI_KEY_KEYPAD_1, ELI_KEY_KEYPAD_2, ELI_KEY_KEYPAD_3, ELI_KEY_KEYPAD_4,
    ELI_KEY_KEYPAD_5, ELI_KEY_KEYPAD_6, ELI_KEY_KEYPAD_7, ELI_KEY_KEYPAD_8, ELI_KEY_KEYPAD_9,
    ELI_KEY_KEYPAD_DECIMAL, ELI_KEY_KEYPAD_DIVIDE, ELI_KEY_KEYPAD_MULTIPLY, ELI_KEY_KEYPAD_SUBTRACT,
    ELI_KEY_KEYPAD_ADD, ELI_KEY_KEYPAD_ENTER, ELI_KEY_KEYPAD_EQUAL,
    /* Modifiers (physical left/right) */
    ELI_KEY_LEFT_CTRL, ELI_KEY_LEFT_SHIFT, ELI_KEY_LEFT_ALT, ELI_KEY_LEFT_SUPER,
    ELI_KEY_RIGHT_CTRL, ELI_KEY_RIGHT_SHIFT, ELI_KEY_RIGHT_ALT, ELI_KEY_RIGHT_SUPER,
    ELI_KEY_MENU,
    /* Digits and letters */
    ELI_KEY_0, ELI_KEY_1, ELI_KEY_2, ELI_KEY_3, ELI_KEY_4, ELI_KEY_5, ELI_KEY_6, ELI_KEY_7, ELI_KEY_8, ELI_KEY_9,
    ELI_KEY_A, ELI_KEY_B, ELI_KEY_C, ELI_KEY_D, ELI_KEY_E, ELI_KEY_F, ELI_KEY_G, ELI_KEY_H, ELI_KEY_I, ELI_KEY_J,
    ELI_KEY_K, ELI_KEY_L, ELI_KEY_M, ELI_KEY_N, ELI_KEY_O, ELI_KEY_P, ELI_KEY_Q, ELI_KEY_R, ELI_KEY_S, ELI_KEY_T,
    ELI_KEY_U, ELI_KEY_V, ELI_KEY_W, ELI_KEY_X, ELI_KEY_Y, ELI_KEY_Z,
    /* Function keys */
    ELI_KEY_F1, ELI_KEY_F2, ELI_KEY_F3, ELI_KEY_F4, ELI_KEY_F5, ELI_KEY_F6,
    ELI_KEY_F7, ELI_KEY_F8, ELI_KEY_F9, ELI_KEY_F10, ELI_KEY_F11, ELI_KEY_F12,
    ELI_KEY_F13, ELI_KEY_F14, ELI_KEY_F15, ELI_KEY_F16, ELI_KEY_F17, ELI_KEY_F18,
    ELI_KEY_F19, ELI_KEY_F20, ELI_KEY_F21, ELI_KEY_F22, ELI_KEY_F23, ELI_KEY_F24,
    /* Combined modifier keys (chord-friendly) */
    ELI_KEY_MOD_CTRL, ELI_KEY_MOD_SHIFT, ELI_KEY_MOD_ALT, ELI_KEY_MOD_SUPER,
    ELI_KEY_COUNT,
    /* Mouse buttons and wheels expressed as keys for unified input handling. */
    ELI_KEY_MOUSE_LEFT, ELI_KEY_MOUSE_RIGHT, ELI_KEY_MOUSE_MIDDLE, ELI_KEY_MOUSE_X1, ELI_KEY_MOUSE_X2,
    ELI_KEY_MOUSE_WHEEL_X, ELI_KEY_MOUSE_WHEEL_Y
};

/* ---------------------------------------------------------------------------
 * Color indices — one entry per themeable color slot. ELI_COL_COUNT sizes the
 * style color array.
 * ------------------------------------------------------------------------- */

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

/* ---------------------------------------------------------------------------
 * Style variables — one entry per pushable style value. ELI_STYLE_VAR_COUNT
 * marks the end of the range.
 * ------------------------------------------------------------------------- */

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

/* ---------------------------------------------------------------------------
 * Window / child / item flags (Begin, BeginChild, PushItemFlag)
 * ------------------------------------------------------------------------- */

typedef int eli_window_flags;
enum eli_window_flags_ {
    ELI_WINDOW_NONE                        = 0,
    ELI_WINDOW_NO_TITLEBAR                 = 1 << 0,
    ELI_WINDOW_NO_RESIZE                   = 1 << 1,
    ELI_WINDOW_NO_MOVE                     = 1 << 2,
    ELI_WINDOW_NO_SCROLLBAR                = 1 << 3,
    ELI_WINDOW_NO_SCROLL_WITH_MOUSE        = 1 << 4,
    ELI_WINDOW_NO_COLLAPSE                 = 1 << 5,
    ELI_WINDOW_AUTO_RESIZE                 = 1 << 6,
    ELI_WINDOW_NO_BACKGROUND               = 1 << 7,
    ELI_WINDOW_NO_SAVED_SETTINGS           = 1 << 8,
    ELI_WINDOW_NO_MOUSE_INPUTS             = 1 << 9,
    ELI_WINDOW_MENU_BAR                    = 1 << 10,
    ELI_WINDOW_HORIZONTAL_SCROLLBAR        = 1 << 11,
    ELI_WINDOW_NO_FOCUS_ON_APPEARING       = 1 << 12,
    ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS  = 1 << 13,
    ELI_WINDOW_ALWAYS_VERTICAL_SCROLLBAR   = 1 << 14,
    ELI_WINDOW_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
    ELI_WINDOW_NO_NAV_INPUTS               = 1 << 16,
    ELI_WINDOW_NO_NAV_FOCUS                = 1 << 17,
    ELI_WINDOW_UNSAVED_DOCUMENT            = 1 << 18,
    ELI_WINDOW_NO_DOCKING                  = 1 << 19,  /* window refuses to dock (Phase 34) */
    ELI_WINDOW_NO_NAV                      = ELI_WINDOW_NO_NAV_INPUTS | ELI_WINDOW_NO_NAV_FOCUS,
    ELI_WINDOW_NO_DECORATION               = ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_RESIZE |
                                             ELI_WINDOW_NO_SCROLLBAR | ELI_WINDOW_NO_COLLAPSE,
    ELI_WINDOW_NO_INPUTS                   = ELI_WINDOW_NO_MOUSE_INPUTS | ELI_WINDOW_NO_NAV_INPUTS |
                                             ELI_WINDOW_NO_NAV_FOCUS
};

typedef int eli_child_flags;
enum eli_child_flags_ {
    ELI_CHILD_NONE                      = 0,
    ELI_CHILD_BORDERS                   = 1 << 0,
    ELI_CHILD_ALWAYS_USE_WINDOW_PADDING = 1 << 1,
    ELI_CHILD_RESIZE_X                  = 1 << 2,
    ELI_CHILD_RESIZE_Y                  = 1 << 3,
    ELI_CHILD_AUTO_RESIZE_X             = 1 << 4,
    ELI_CHILD_AUTO_RESIZE_Y             = 1 << 5,
    ELI_CHILD_ALWAYS_AUTO_RESIZE        = 1 << 6,
    ELI_CHILD_FRAME_STYLE               = 1 << 7,
    ELI_CHILD_NAV_FLATTENED             = 1 << 8
};

typedef int eli_item_flags;
enum eli_item_flags_ {
    ELI_ITEM_NONE                 = 0,
    ELI_ITEM_NO_TAB_STOP          = 1 << 0,
    ELI_ITEM_NO_NAV               = 1 << 1,
    ELI_ITEM_NO_NAV_DEFAULT_FOCUS = 1 << 2,
    ELI_ITEM_BUTTON_REPEAT        = 1 << 3,
    ELI_ITEM_AUTO_CLOSE_POPUPS    = 1 << 4,
    ELI_ITEM_ALLOW_DUPLICATE_ID   = 1 << 5,
    ELI_ITEM_DISABLED             = 1 << 6
};

/* ---------------------------------------------------------------------------
 * Hovered flags — referenced by the style's tooltip defaults. The full set is
 * used by item/window hover queries in later phases.
 * ------------------------------------------------------------------------- */

typedef int eli_hovered_flags;
enum eli_hovered_flags_ {
    ELI_HOVERED_NONE                              = 0,
    ELI_HOVERED_CHILD_WINDOWS                     = 1 << 0,
    ELI_HOVERED_ROOT_WINDOW                       = 1 << 1,
    ELI_HOVERED_ANY_WINDOW                        = 1 << 2,
    ELI_HOVERED_NO_POPUP_HIERARCHY                = 1 << 3,
    ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP       = 1 << 5,
    ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM = 1 << 7,
    ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_ITEM     = 1 << 8,
    ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_WINDOW   = 1 << 9,
    ELI_HOVERED_ALLOW_WHEN_DISABLED               = 1 << 10,
    ELI_HOVERED_NO_NAV_OVERRIDE                    = 1 << 11,
    ELI_HOVERED_FOR_TOOLTIP                        = 1 << 12,
    ELI_HOVERED_STATIONARY                        = 1 << 13,
    ELI_HOVERED_DELAY_NONE                         = 1 << 14,
    ELI_HOVERED_DELAY_SHORT                        = 1 << 15,
    ELI_HOVERED_DELAY_NORMAL                       = 1 << 16,
    ELI_HOVERED_NO_SHARED_DELAY                    = 1 << 17,
    ELI_HOVERED_RECT_ONLY                          = ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_POPUP |
                                                     ELI_HOVERED_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM |
                                                     ELI_HOVERED_ALLOW_WHEN_OVERLAPPED_BY_ITEM,
    ELI_HOVERED_ROOT_AND_CHILD_WINDOWS            = ELI_HOVERED_ROOT_WINDOW | ELI_HOVERED_CHILD_WINDOWS
};

/* ---------------------------------------------------------------------------
 * IO configuration and backend capability flags
 * ------------------------------------------------------------------------- */

typedef int eli_config_flags;
enum eli_config_flags_ {
    ELI_CONFIG_FLAGS_NONE                  = 0,
    ELI_CONFIG_FLAGS_NAV_ENABLE_KEYBOARD   = 1 << 0,
    ELI_CONFIG_FLAGS_NAV_ENABLE_GAMEPAD    = 1 << 1,
    ELI_CONFIG_FLAGS_NO_MOUSE              = 1 << 4,
    ELI_CONFIG_FLAGS_NO_MOUSE_CURSOR_CHANGE = 1 << 5,
    ELI_CONFIG_FLAGS_NO_KEYBOARD           = 1 << 6,
    ELI_CONFIG_FLAGS_IS_SRGB               = 1 << 20,
    ELI_CONFIG_FLAGS_IS_TOUCH_SCREEN       = 1 << 21
};

typedef int eli_backend_flags;
enum eli_backend_flags_ {
    ELI_BACKEND_FLAGS_NONE                   = 0,
    ELI_BACKEND_FLAGS_HAS_GAMEPAD            = 1 << 0,
    ELI_BACKEND_FLAGS_HAS_MOUSE_CURSORS      = 1 << 1,
    ELI_BACKEND_FLAGS_HAS_SET_MOUSE_POS      = 1 << 2,
    ELI_BACKEND_FLAGS_RENDERER_HAS_VTX_OFFSET = 1 << 3,
    ELI_BACKEND_FLAGS_RENDERER_HAS_TEXTURES  = 1 << 4
};

#endif /* ELI_CORE_ELI_ENUMS_H */
