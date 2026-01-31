/*
 * elimgui.h - Pure C Immediate Mode GUI Library
 *
 * A ground-up reimplementation of Dear ImGui in pure C11,
 * targeting WebAssembly via JAClibc.
 */

#ifndef ELIMGUI_H
#define ELIMGUI_H

#include <jaclibc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version */
#define ELI_VERSION       "0.1.0"
#define ELI_VERSION_NUM   001

/*============================================================================
 * FORWARD DECLARATIONS
 *===========================================================================*/

typedef struct eli_context eli_context;
typedef struct eli_io eli_io;
typedef struct eli_style eli_style;
typedef struct eli_draw_list eli_draw_list;
typedef struct eli_draw_data eli_draw_data;
typedef struct eli_font eli_font;
typedef struct eli_font_atlas eli_font_atlas;

/*============================================================================
 * CORE TYPES
 *===========================================================================*/

/* Unique widget identifier */
typedef uint32_t eli_id;

/* 2D vector for positions, sizes, etc. */
typedef struct eli_vec2 {
    float x, y;
} eli_vec2;

/* 4D vector for colors, rectangles, etc. */
typedef struct eli_vec4 {
    float x, y, z, w;
} eli_vec4;

/* Axis-aligned bounding box */
typedef struct eli_rect {
    eli_vec2 min;
    eli_vec2 max;
} eli_rect;

/*============================================================================
 * COLOR MACROS
 *===========================================================================*/

#define ELI_COL32_R_SHIFT    0
#define ELI_COL32_G_SHIFT    8
#define ELI_COL32_B_SHIFT    16
#define ELI_COL32_A_SHIFT    24
#define ELI_COL32_A_MASK     0xFF000000

#define ELI_COL32(R, G, B, A) \
    (((uint32_t)(A) << ELI_COL32_A_SHIFT) | \
     ((uint32_t)(B) << ELI_COL32_B_SHIFT) | \
     ((uint32_t)(G) << ELI_COL32_G_SHIFT) | \
     ((uint32_t)(R) << ELI_COL32_R_SHIFT))

#define ELI_COL32_WHITE       ELI_COL32(255, 255, 255, 255)
#define ELI_COL32_BLACK       ELI_COL32(0, 0, 0, 255)
#define ELI_COL32_BLACK_TRANS ELI_COL32(0, 0, 0, 0)

/*============================================================================
 * ENUMS: Direction, Condition, Data Type
 *===========================================================================*/

typedef enum eli_dir {
    ELI_DIR_NONE  = -1,
    ELI_DIR_LEFT  = 0,
    ELI_DIR_RIGHT = 1,
    ELI_DIR_UP    = 2,
    ELI_DIR_DOWN  = 3,
    ELI_DIR_COUNT
} eli_dir;

typedef enum eli_cond {
    ELI_COND_NONE          = 0,
    ELI_COND_ALWAYS        = 1 << 0,
    ELI_COND_ONCE          = 1 << 1,
    ELI_COND_FIRST_USE_EVER = 1 << 2,
    ELI_COND_APPEARING     = 1 << 3
} eli_cond;

typedef enum eli_data_type {
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
    ELI_DATA_TYPE_BOOL,
    ELI_DATA_TYPE_STRING,
    ELI_DATA_TYPE_COUNT
} eli_data_type;

/*============================================================================
 * ENUMS: Window Flags
 *===========================================================================*/

typedef enum eli_window_flags {
    ELI_WINDOW_FLAGS_NONE                      = 0,
    ELI_WINDOW_FLAGS_NO_TITLE_BAR              = 1 << 0,
    ELI_WINDOW_FLAGS_NO_RESIZE                 = 1 << 1,
    ELI_WINDOW_FLAGS_NO_MOVE                   = 1 << 2,
    ELI_WINDOW_FLAGS_NO_SCROLLBAR              = 1 << 3,
    ELI_WINDOW_FLAGS_NO_SCROLL_WITH_MOUSE      = 1 << 4,
    ELI_WINDOW_FLAGS_NO_COLLAPSE               = 1 << 5,
    ELI_WINDOW_FLAGS_ALWAYS_AUTO_RESIZE        = 1 << 6,
    ELI_WINDOW_FLAGS_NO_BACKGROUND             = 1 << 7,
    ELI_WINDOW_FLAGS_NO_SAVED_SETTINGS         = 1 << 8,
    ELI_WINDOW_FLAGS_NO_MOUSE_INPUTS           = 1 << 9,
    ELI_WINDOW_FLAGS_MENU_BAR                  = 1 << 10,
    ELI_WINDOW_FLAGS_HORIZONTAL_SCROLLBAR      = 1 << 11,
    ELI_WINDOW_FLAGS_NO_FOCUS_ON_APPEARING     = 1 << 12,
    ELI_WINDOW_FLAGS_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 13,
    ELI_WINDOW_FLAGS_ALWAYS_VERTICAL_SCROLLBAR = 1 << 14,
    ELI_WINDOW_FLAGS_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
    ELI_WINDOW_FLAGS_NO_NAV_INPUTS             = 1 << 16,
    ELI_WINDOW_FLAGS_NO_NAV_FOCUS              = 1 << 17,
    ELI_WINDOW_FLAGS_UNSAVED_DOCUMENT          = 1 << 18,

    /* Composite flags */
    ELI_WINDOW_FLAGS_NO_NAV = ELI_WINDOW_FLAGS_NO_NAV_INPUTS | ELI_WINDOW_FLAGS_NO_NAV_FOCUS,
    ELI_WINDOW_FLAGS_NO_DECORATION = ELI_WINDOW_FLAGS_NO_TITLE_BAR | ELI_WINDOW_FLAGS_NO_RESIZE |
                                     ELI_WINDOW_FLAGS_NO_SCROLLBAR | ELI_WINDOW_FLAGS_NO_COLLAPSE,
    ELI_WINDOW_FLAGS_NO_INPUTS = ELI_WINDOW_FLAGS_NO_MOUSE_INPUTS | ELI_WINDOW_FLAGS_NO_NAV_INPUTS |
                                 ELI_WINDOW_FLAGS_NO_NAV_FOCUS,

    /* Internal flags */
    ELI_WINDOW_FLAGS_CHILD_WINDOW              = 1 << 24,
    ELI_WINDOW_FLAGS_TOOLTIP                   = 1 << 25,
    ELI_WINDOW_FLAGS_POPUP                     = 1 << 26,
    ELI_WINDOW_FLAGS_MODAL                     = 1 << 27
} eli_window_flags;

/*============================================================================
 * ENUMS: Child Window Flags
 *===========================================================================*/

typedef enum eli_child_flags {
    ELI_CHILD_FLAGS_NONE                       = 0,
    ELI_CHILD_FLAGS_BORDERS                    = 1 << 0,
    ELI_CHILD_FLAGS_ALWAYS_USE_WINDOW_PADDING  = 1 << 1,
    ELI_CHILD_FLAGS_RESIZE_X                   = 1 << 2,
    ELI_CHILD_FLAGS_RESIZE_Y                   = 1 << 3,
    ELI_CHILD_FLAGS_AUTO_RESIZE_X              = 1 << 4,
    ELI_CHILD_FLAGS_AUTO_RESIZE_Y              = 1 << 5,
    ELI_CHILD_FLAGS_ALWAYS_AUTO_RESIZE         = 1 << 6,
    ELI_CHILD_FLAGS_FRAME_STYLE                = 1 << 7,
    ELI_CHILD_FLAGS_NAV_FLATTEN                = 1 << 8
} eli_child_flags;

/*============================================================================
 * ENUMS: Item Flags
 *===========================================================================*/

typedef enum eli_item_flags {
    ELI_ITEM_FLAGS_NONE                        = 0,
    ELI_ITEM_FLAGS_NO_TAB_STOP                 = 1 << 0,
    ELI_ITEM_FLAGS_NO_NAV                      = 1 << 1,
    ELI_ITEM_FLAGS_NO_NAV_DEFAULT_FOCUS        = 1 << 2,
    ELI_ITEM_FLAGS_BUTTON_REPEAT               = 1 << 3,
    ELI_ITEM_FLAGS_AUTO_CLOSE_POPUPS           = 1 << 4
} eli_item_flags;

/*============================================================================
 * ENUMS: Input Text Flags
 *===========================================================================*/

typedef enum eli_input_text_flags {
    ELI_INPUT_TEXT_FLAGS_NONE                  = 0,
    ELI_INPUT_TEXT_FLAGS_CHARS_DECIMAL         = 1 << 0,
    ELI_INPUT_TEXT_FLAGS_CHARS_HEXADECIMAL     = 1 << 1,
    ELI_INPUT_TEXT_FLAGS_CHARS_SCIENTIFIC      = 1 << 2,
    ELI_INPUT_TEXT_FLAGS_CHARS_UPPERCASE       = 1 << 3,
    ELI_INPUT_TEXT_FLAGS_CHARS_NO_BLANK        = 1 << 4,
    ELI_INPUT_TEXT_FLAGS_ALLOW_TAB_INPUT       = 1 << 5,
    ELI_INPUT_TEXT_FLAGS_ENTER_RETURNS_TRUE    = 1 << 6,
    ELI_INPUT_TEXT_FLAGS_ESCAPE_CLEARS_ALL     = 1 << 7,
    ELI_INPUT_TEXT_FLAGS_CTRL_ENTER_FOR_NEW_LINE = 1 << 8,
    ELI_INPUT_TEXT_FLAGS_READ_ONLY             = 1 << 9,
    ELI_INPUT_TEXT_FLAGS_PASSWORD              = 1 << 10,
    ELI_INPUT_TEXT_FLAGS_ALWAYS_OVERWRITE      = 1 << 11,
    ELI_INPUT_TEXT_FLAGS_AUTO_SELECT_ALL       = 1 << 12,
    ELI_INPUT_TEXT_FLAGS_PARSE_EMPTY_REF_VAL   = 1 << 13,
    ELI_INPUT_TEXT_FLAGS_DISPLAY_EMPTY_REF_VAL = 1 << 14,
    ELI_INPUT_TEXT_FLAGS_NO_HORIZONTAL_SCROLL  = 1 << 15,
    ELI_INPUT_TEXT_FLAGS_NO_UNDO_REDO          = 1 << 16,
    ELI_INPUT_TEXT_FLAGS_ELIDE_LEFT            = 1 << 17,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_COMPLETION   = 1 << 18,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_HISTORY      = 1 << 19,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_ALWAYS       = 1 << 20,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_CHAR_FILTER  = 1 << 21,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_RESIZE       = 1 << 22,
    ELI_INPUT_TEXT_FLAGS_CALLBACK_EDIT         = 1 << 23,
    ELI_INPUT_TEXT_FLAGS_WORD_WRAP             = 1 << 24
} eli_input_text_flags;

/*============================================================================
 * ENUMS: Tree Node Flags
 *===========================================================================*/

typedef enum eli_tree_node_flags {
    ELI_TREE_NODE_FLAGS_NONE                   = 0,
    ELI_TREE_NODE_FLAGS_SELECTED               = 1 << 0,
    ELI_TREE_NODE_FLAGS_FRAMED                 = 1 << 1,
    ELI_TREE_NODE_FLAGS_ALLOW_OVERLAP          = 1 << 2,
    ELI_TREE_NODE_FLAGS_NO_TREE_PUSH_ON_OPEN   = 1 << 3,
    ELI_TREE_NODE_FLAGS_NO_AUTO_OPEN_ON_LOG    = 1 << 4,
    ELI_TREE_NODE_FLAGS_DEFAULT_OPEN           = 1 << 5,
    ELI_TREE_NODE_FLAGS_OPEN_ON_DOUBLE_CLICK   = 1 << 6,
    ELI_TREE_NODE_FLAGS_OPEN_ON_ARROW          = 1 << 7,
    ELI_TREE_NODE_FLAGS_LEAF                   = 1 << 8,
    ELI_TREE_NODE_FLAGS_BULLET                 = 1 << 9,
    ELI_TREE_NODE_FLAGS_FRAME_PADDING          = 1 << 10,
    ELI_TREE_NODE_FLAGS_SPAN_AVAIL_WIDTH       = 1 << 11,
    ELI_TREE_NODE_FLAGS_SPAN_FULL_WIDTH        = 1 << 12,
    ELI_TREE_NODE_FLAGS_SPAN_LABEL_WIDTH       = 1 << 13,
    ELI_TREE_NODE_FLAGS_SPAN_ALL_COLUMNS       = 1 << 14,
    ELI_TREE_NODE_FLAGS_LABEL_SPAN_ALL_COLUMNS = 1 << 15,
    ELI_TREE_NODE_FLAGS_NAV_LEFT_JUMPS_TO_PARENT = 1 << 17,
    ELI_TREE_NODE_FLAGS_DRAW_LINES_NONE        = 1 << 18,
    ELI_TREE_NODE_FLAGS_DRAW_LINES_FULL        = 1 << 19,
    ELI_TREE_NODE_FLAGS_DRAW_LINES_TO_NODES    = 1 << 20,

    /* Composite */
    ELI_TREE_NODE_FLAGS_COLLAPSING_HEADER = ELI_TREE_NODE_FLAGS_FRAMED |
                                            ELI_TREE_NODE_FLAGS_NO_TREE_PUSH_ON_OPEN |
                                            ELI_TREE_NODE_FLAGS_NO_AUTO_OPEN_ON_LOG
} eli_tree_node_flags;

/*============================================================================
 * ENUMS: Selectable Flags
 *===========================================================================*/

typedef enum eli_selectable_flags {
    ELI_SELECTABLE_FLAGS_NONE                  = 0,
    ELI_SELECTABLE_FLAGS_NO_AUTO_CLOSE_POPUPS  = 1 << 0,
    ELI_SELECTABLE_FLAGS_SPAN_ALL_COLUMNS      = 1 << 1,
    ELI_SELECTABLE_FLAGS_ALLOW_DOUBLE_CLICK    = 1 << 2,
    ELI_SELECTABLE_FLAGS_DISABLED              = 1 << 3,
    ELI_SELECTABLE_FLAGS_ALLOW_OVERLAP         = 1 << 4,
    ELI_SELECTABLE_FLAGS_HIGHLIGHT             = 1 << 5,
    ELI_SELECTABLE_FLAGS_SELECT_ON_NAV         = 1 << 6
} eli_selectable_flags;

/*============================================================================
 * ENUMS: Combo Flags
 *===========================================================================*/

typedef enum eli_combo_flags {
    ELI_COMBO_FLAGS_NONE                       = 0,
    ELI_COMBO_FLAGS_POPUP_ALIGN_LEFT           = 1 << 0,
    ELI_COMBO_FLAGS_HEIGHT_SMALL               = 1 << 1,
    ELI_COMBO_FLAGS_HEIGHT_REGULAR             = 1 << 2,
    ELI_COMBO_FLAGS_HEIGHT_LARGE               = 1 << 3,
    ELI_COMBO_FLAGS_HEIGHT_LARGEST             = 1 << 4,
    ELI_COMBO_FLAGS_NO_ARROW_BUTTON            = 1 << 5,
    ELI_COMBO_FLAGS_NO_PREVIEW                 = 1 << 6,
    ELI_COMBO_FLAGS_WIDTH_FIT_PREVIEW          = 1 << 7,

    ELI_COMBO_FLAGS_HEIGHT_MASK = ELI_COMBO_FLAGS_HEIGHT_SMALL | ELI_COMBO_FLAGS_HEIGHT_REGULAR |
                                  ELI_COMBO_FLAGS_HEIGHT_LARGE | ELI_COMBO_FLAGS_HEIGHT_LARGEST
} eli_combo_flags;

/*============================================================================
 * ENUMS: Tab Bar Flags
 *===========================================================================*/

typedef enum eli_tab_bar_flags {
    ELI_TAB_BAR_FLAGS_NONE                          = 0,
    ELI_TAB_BAR_FLAGS_REORDERABLE                   = 1 << 0,
    ELI_TAB_BAR_FLAGS_AUTO_SELECT_NEW_TABS          = 1 << 1,
    ELI_TAB_BAR_FLAGS_TAB_LIST_POPUP_BUTTON         = 1 << 2,
    ELI_TAB_BAR_FLAGS_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 3,
    ELI_TAB_BAR_FLAGS_NO_TAB_LIST_SCROLLING_BUTTONS = 1 << 4,
    ELI_TAB_BAR_FLAGS_NO_TOOLTIP                    = 1 << 5,
    ELI_TAB_BAR_FLAGS_DRAW_SELECTED_OVERLINE        = 1 << 6,
    ELI_TAB_BAR_FLAGS_FITTING_POLICY_MIXED          = 1 << 7,
    ELI_TAB_BAR_FLAGS_FITTING_POLICY_SHRINK         = 1 << 8,
    ELI_TAB_BAR_FLAGS_FITTING_POLICY_SCROLL         = 1 << 9,

    ELI_TAB_BAR_FLAGS_FITTING_POLICY_MASK = ELI_TAB_BAR_FLAGS_FITTING_POLICY_MIXED |
                                            ELI_TAB_BAR_FLAGS_FITTING_POLICY_SHRINK |
                                            ELI_TAB_BAR_FLAGS_FITTING_POLICY_SCROLL,
    ELI_TAB_BAR_FLAGS_FITTING_POLICY_DEFAULT = ELI_TAB_BAR_FLAGS_FITTING_POLICY_MIXED
} eli_tab_bar_flags;

/*============================================================================
 * ENUMS: Tab Item Flags
 *===========================================================================*/

typedef enum eli_tab_item_flags {
    ELI_TAB_ITEM_FLAGS_NONE                          = 0,
    ELI_TAB_ITEM_FLAGS_UNSAVED_DOCUMENT              = 1 << 0,
    ELI_TAB_ITEM_FLAGS_SET_SELECTED                  = 1 << 1,
    ELI_TAB_ITEM_FLAGS_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 2,
    ELI_TAB_ITEM_FLAGS_NO_PUSH_ID                    = 1 << 3,
    ELI_TAB_ITEM_FLAGS_NO_TOOLTIP                    = 1 << 4,
    ELI_TAB_ITEM_FLAGS_NO_REORDER                    = 1 << 5,
    ELI_TAB_ITEM_FLAGS_LEADING                       = 1 << 6,
    ELI_TAB_ITEM_FLAGS_TRAILING                      = 1 << 7,
    ELI_TAB_ITEM_FLAGS_NO_ASSUMED_CLOSURE            = 1 << 8
} eli_tab_item_flags;

/*============================================================================
 * ENUMS: Table Flags
 *===========================================================================*/

typedef enum eli_table_flags {
    /* Features */
    ELI_TABLE_FLAGS_NONE                       = 0,
    ELI_TABLE_FLAGS_RESIZABLE                  = 1 << 0,
    ELI_TABLE_FLAGS_REORDERABLE                = 1 << 1,
    ELI_TABLE_FLAGS_HIDEABLE                   = 1 << 2,
    ELI_TABLE_FLAGS_SORTABLE                   = 1 << 3,
    ELI_TABLE_FLAGS_NO_SAVED_SETTINGS          = 1 << 4,
    ELI_TABLE_FLAGS_CONTEXT_MENU_IN_BODY       = 1 << 5,

    /* Decorations */
    ELI_TABLE_FLAGS_ROW_BG                     = 1 << 6,
    ELI_TABLE_FLAGS_BORDERS_INNER_H            = 1 << 7,
    ELI_TABLE_FLAGS_BORDERS_OUTER_H            = 1 << 8,
    ELI_TABLE_FLAGS_BORDERS_INNER_V            = 1 << 9,
    ELI_TABLE_FLAGS_BORDERS_OUTER_V            = 1 << 10,
    ELI_TABLE_FLAGS_BORDERS_H = ELI_TABLE_FLAGS_BORDERS_INNER_H | ELI_TABLE_FLAGS_BORDERS_OUTER_H,
    ELI_TABLE_FLAGS_BORDERS_V = ELI_TABLE_FLAGS_BORDERS_INNER_V | ELI_TABLE_FLAGS_BORDERS_OUTER_V,
    ELI_TABLE_FLAGS_BORDERS_INNER = ELI_TABLE_FLAGS_BORDERS_INNER_V | ELI_TABLE_FLAGS_BORDERS_INNER_H,
    ELI_TABLE_FLAGS_BORDERS_OUTER = ELI_TABLE_FLAGS_BORDERS_OUTER_V | ELI_TABLE_FLAGS_BORDERS_OUTER_H,
    ELI_TABLE_FLAGS_BORDERS = ELI_TABLE_FLAGS_BORDERS_INNER | ELI_TABLE_FLAGS_BORDERS_OUTER,
    ELI_TABLE_FLAGS_NO_BORDERS_IN_BODY         = 1 << 11,
    ELI_TABLE_FLAGS_NO_BORDERS_IN_BODY_UNTIL_RESIZE = 1 << 12,

    /* Sizing Policy */
    ELI_TABLE_FLAGS_SIZING_FIXED_FIT           = 1 << 13,
    ELI_TABLE_FLAGS_SIZING_FIXED_SAME          = 2 << 13,
    ELI_TABLE_FLAGS_SIZING_STRETCH_PROP        = 3 << 13,
    ELI_TABLE_FLAGS_SIZING_STRETCH_SAME        = 4 << 13,

    /* Sizing Extra */
    ELI_TABLE_FLAGS_NO_HOST_EXTEND_X           = 1 << 16,
    ELI_TABLE_FLAGS_NO_HOST_EXTEND_Y           = 1 << 17,
    ELI_TABLE_FLAGS_NO_KEEP_COLUMNS_VISIBLE    = 1 << 18,
    ELI_TABLE_FLAGS_PRECISE_WIDTHS             = 1 << 19,

    /* Clipping */
    ELI_TABLE_FLAGS_NO_CLIP                    = 1 << 20,

    /* Padding */
    ELI_TABLE_FLAGS_PAD_OUTER_X                = 1 << 21,
    ELI_TABLE_FLAGS_NO_PAD_OUTER_X             = 1 << 22,
    ELI_TABLE_FLAGS_NO_PAD_INNER_X             = 1 << 23,

    /* Scrolling */
    ELI_TABLE_FLAGS_SCROLL_X                   = 1 << 24,
    ELI_TABLE_FLAGS_SCROLL_Y                   = 1 << 25,

    /* Sorting */
    ELI_TABLE_FLAGS_SORT_MULTI                 = 1 << 26,
    ELI_TABLE_FLAGS_SORT_TRISTATE              = 1 << 27,

    /* Misc */
    ELI_TABLE_FLAGS_HIGHLIGHT_HOVERED_COLUMN   = 1 << 28,

    ELI_TABLE_FLAGS_SIZING_MASK = ELI_TABLE_FLAGS_SIZING_FIXED_FIT | ELI_TABLE_FLAGS_SIZING_FIXED_SAME |
                                  ELI_TABLE_FLAGS_SIZING_STRETCH_PROP | ELI_TABLE_FLAGS_SIZING_STRETCH_SAME
} eli_table_flags;

/*============================================================================
 * ENUMS: Table Column Flags
 *===========================================================================*/

typedef enum eli_table_column_flags {
    /* Input configuration flags */
    ELI_TABLE_COLUMN_FLAGS_NONE                = 0,
    ELI_TABLE_COLUMN_FLAGS_DISABLED            = 1 << 0,
    ELI_TABLE_COLUMN_FLAGS_DEFAULT_HIDE        = 1 << 1,
    ELI_TABLE_COLUMN_FLAGS_DEFAULT_SORT        = 1 << 2,
    ELI_TABLE_COLUMN_FLAGS_WIDTH_STRETCH       = 1 << 3,
    ELI_TABLE_COLUMN_FLAGS_WIDTH_FIXED         = 1 << 4,
    ELI_TABLE_COLUMN_FLAGS_NO_RESIZE           = 1 << 5,
    ELI_TABLE_COLUMN_FLAGS_NO_REORDER          = 1 << 6,
    ELI_TABLE_COLUMN_FLAGS_NO_HIDE             = 1 << 7,
    ELI_TABLE_COLUMN_FLAGS_NO_CLIP             = 1 << 8,
    ELI_TABLE_COLUMN_FLAGS_NO_SORT             = 1 << 9,
    ELI_TABLE_COLUMN_FLAGS_NO_SORT_ASCENDING   = 1 << 10,
    ELI_TABLE_COLUMN_FLAGS_NO_SORT_DESCENDING  = 1 << 11,
    ELI_TABLE_COLUMN_FLAGS_NO_HEADER_LABEL     = 1 << 12,
    ELI_TABLE_COLUMN_FLAGS_NO_HEADER_WIDTH     = 1 << 13,
    ELI_TABLE_COLUMN_FLAGS_PREFER_SORT_ASCENDING = 1 << 14,
    ELI_TABLE_COLUMN_FLAGS_PREFER_SORT_DESCENDING = 1 << 15,
    ELI_TABLE_COLUMN_FLAGS_INDENT_ENABLE       = 1 << 16,
    ELI_TABLE_COLUMN_FLAGS_INDENT_DISABLE      = 1 << 17,
    ELI_TABLE_COLUMN_FLAGS_ANGLED_HEADER       = 1 << 18,

    /* Output status flags */
    ELI_TABLE_COLUMN_FLAGS_IS_ENABLED          = 1 << 24,
    ELI_TABLE_COLUMN_FLAGS_IS_VISIBLE          = 1 << 25,
    ELI_TABLE_COLUMN_FLAGS_IS_SORTED           = 1 << 26,
    ELI_TABLE_COLUMN_FLAGS_IS_HOVERED          = 1 << 27,

    /* Masks */
    ELI_TABLE_COLUMN_FLAGS_WIDTH_MASK = ELI_TABLE_COLUMN_FLAGS_WIDTH_STRETCH | ELI_TABLE_COLUMN_FLAGS_WIDTH_FIXED,
    ELI_TABLE_COLUMN_FLAGS_INDENT_MASK = ELI_TABLE_COLUMN_FLAGS_INDENT_ENABLE | ELI_TABLE_COLUMN_FLAGS_INDENT_DISABLE,
    ELI_TABLE_COLUMN_FLAGS_STATUS_MASK = ELI_TABLE_COLUMN_FLAGS_IS_ENABLED | ELI_TABLE_COLUMN_FLAGS_IS_VISIBLE |
                                         ELI_TABLE_COLUMN_FLAGS_IS_SORTED | ELI_TABLE_COLUMN_FLAGS_IS_HOVERED,
    ELI_TABLE_COLUMN_FLAGS_NO_DIRECT_RESIZE    = 1 << 30
} eli_table_column_flags;

/*============================================================================
 * ENUMS: Popup Flags
 *===========================================================================*/

typedef enum eli_popup_flags {
    ELI_POPUP_FLAGS_NONE                       = 0,
    ELI_POPUP_FLAGS_MOUSE_BUTTON_LEFT          = 0,
    ELI_POPUP_FLAGS_MOUSE_BUTTON_RIGHT         = 1,
    ELI_POPUP_FLAGS_MOUSE_BUTTON_MIDDLE        = 2,
    ELI_POPUP_FLAGS_NO_REOPEN                  = 1 << 5,
    ELI_POPUP_FLAGS_NO_OPEN_OVER_EXISTING_POPUP = 1 << 7,
    ELI_POPUP_FLAGS_NO_OPEN_OVER_ITEMS         = 1 << 8,
    ELI_POPUP_FLAGS_ANY_POPUP_ID               = 1 << 10,
    ELI_POPUP_FLAGS_ANY_POPUP_LEVEL            = 1 << 11,
    ELI_POPUP_FLAGS_ANY_POPUP = ELI_POPUP_FLAGS_ANY_POPUP_ID | ELI_POPUP_FLAGS_ANY_POPUP_LEVEL
} eli_popup_flags;

/*============================================================================
 * ENUMS: Hovered Flags
 *===========================================================================*/

typedef enum eli_hovered_flags {
    ELI_HOVERED_FLAGS_NONE                           = 0,
    ELI_HOVERED_FLAGS_CHILD_WINDOWS                  = 1 << 0,
    ELI_HOVERED_FLAGS_ROOT_WINDOW                    = 1 << 1,
    ELI_HOVERED_FLAGS_ANY_WINDOW                     = 1 << 2,
    ELI_HOVERED_FLAGS_NO_POPUP_HIERARCHY             = 1 << 3,
    ELI_HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_POPUP    = 1 << 5,
    ELI_HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM = 1 << 7,
    ELI_HOVERED_FLAGS_ALLOW_WHEN_OVERLAPPED_BY_ITEM  = 1 << 8,
    ELI_HOVERED_FLAGS_ALLOW_WHEN_OVERLAPPED_BY_WINDOW = 1 << 9,
    ELI_HOVERED_FLAGS_ALLOW_WHEN_DISABLED            = 1 << 10,
    ELI_HOVERED_FLAGS_NO_NAV_OVERRIDE                = 1 << 11,
    ELI_HOVERED_FLAGS_RECT_ONLY = ELI_HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_POPUP |
                                  ELI_HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM |
                                  ELI_HOVERED_FLAGS_ALLOW_WHEN_OVERLAPPED_BY_ITEM |
                                  ELI_HOVERED_FLAGS_ALLOW_WHEN_OVERLAPPED_BY_WINDOW,
    ELI_HOVERED_FLAGS_ROOT_AND_CHILD_WINDOWS = ELI_HOVERED_FLAGS_ROOT_WINDOW | ELI_HOVERED_FLAGS_CHILD_WINDOWS,

    /* Tooltips */
    ELI_HOVERED_FLAGS_FOR_TOOLTIP                    = 1 << 12,
    ELI_HOVERED_FLAGS_STATIONARY                     = 1 << 13,
    ELI_HOVERED_FLAGS_DELAY_NONE                     = 1 << 14,
    ELI_HOVERED_FLAGS_DELAY_SHORT                    = 1 << 15,
    ELI_HOVERED_FLAGS_DELAY_NORMAL                   = 1 << 16,
    ELI_HOVERED_FLAGS_NO_SHARED_DELAY                = 1 << 17
} eli_hovered_flags;

/*============================================================================
 * ENUMS: Focused Flags
 *===========================================================================*/

typedef enum eli_focused_flags {
    ELI_FOCUSED_FLAGS_NONE                     = 0,
    ELI_FOCUSED_FLAGS_CHILD_WINDOWS            = 1 << 0,
    ELI_FOCUSED_FLAGS_ROOT_WINDOW              = 1 << 1,
    ELI_FOCUSED_FLAGS_ANY_WINDOW               = 1 << 2,
    ELI_FOCUSED_FLAGS_NO_POPUP_HIERARCHY       = 1 << 3,
    ELI_FOCUSED_FLAGS_ROOT_AND_CHILD_WINDOWS = ELI_FOCUSED_FLAGS_ROOT_WINDOW | ELI_FOCUSED_FLAGS_CHILD_WINDOWS
} eli_focused_flags;

/*============================================================================
 * ENUMS: Slider Flags
 *===========================================================================*/

typedef enum eli_slider_flags {
    ELI_SLIDER_FLAGS_NONE                      = 0,
    ELI_SLIDER_FLAGS_LOGARITHMIC               = 1 << 5,
    ELI_SLIDER_FLAGS_NO_ROUND_TO_FORMAT        = 1 << 6,
    ELI_SLIDER_FLAGS_NO_INPUT                  = 1 << 7,
    ELI_SLIDER_FLAGS_WRAP_AROUND               = 1 << 8,
    ELI_SLIDER_FLAGS_CLAMP_ON_INPUT            = 1 << 9,
    ELI_SLIDER_FLAGS_CLAMP_ZERO_RANGE          = 1 << 10,
    ELI_SLIDER_FLAGS_ALWAYS_CLAMP = ELI_SLIDER_FLAGS_CLAMP_ON_INPUT | ELI_SLIDER_FLAGS_CLAMP_ZERO_RANGE
} eli_slider_flags;

/*============================================================================
 * ENUMS: Color Edit Flags
 *===========================================================================*/

typedef enum eli_color_edit_flags {
    ELI_COLOR_EDIT_FLAGS_NONE                  = 0,
    ELI_COLOR_EDIT_FLAGS_NO_ALPHA              = 1 << 1,
    ELI_COLOR_EDIT_FLAGS_NO_PICKER             = 1 << 2,
    ELI_COLOR_EDIT_FLAGS_NO_OPTIONS            = 1 << 3,
    ELI_COLOR_EDIT_FLAGS_NO_SMALL_PREVIEW      = 1 << 4,
    ELI_COLOR_EDIT_FLAGS_NO_INPUTS             = 1 << 5,
    ELI_COLOR_EDIT_FLAGS_NO_TOOLTIP            = 1 << 6,
    ELI_COLOR_EDIT_FLAGS_NO_LABEL              = 1 << 7,
    ELI_COLOR_EDIT_FLAGS_NO_SIDE_PREVIEW       = 1 << 8,
    ELI_COLOR_EDIT_FLAGS_NO_DRAG_DROP          = 1 << 9,
    ELI_COLOR_EDIT_FLAGS_NO_BORDER             = 1 << 10,
    ELI_COLOR_EDIT_FLAGS_ALPHA_BAR             = 1 << 16,
    ELI_COLOR_EDIT_FLAGS_ALPHA_PREVIEW         = 1 << 17,
    ELI_COLOR_EDIT_FLAGS_ALPHA_PREVIEW_HALF    = 1 << 18,
    ELI_COLOR_EDIT_FLAGS_HDR                   = 1 << 19,
    ELI_COLOR_EDIT_FLAGS_DISPLAY_RGB           = 1 << 20,
    ELI_COLOR_EDIT_FLAGS_DISPLAY_HSV           = 1 << 21,
    ELI_COLOR_EDIT_FLAGS_DISPLAY_HEX           = 1 << 22,
    ELI_COLOR_EDIT_FLAGS_UINT8                 = 1 << 23,
    ELI_COLOR_EDIT_FLAGS_FLOAT                 = 1 << 24,
    ELI_COLOR_EDIT_FLAGS_PICKER_HUE_BAR        = 1 << 25,
    ELI_COLOR_EDIT_FLAGS_PICKER_HUE_WHEEL      = 1 << 26,
    ELI_COLOR_EDIT_FLAGS_INPUT_RGB             = 1 << 27,
    ELI_COLOR_EDIT_FLAGS_INPUT_HSV             = 1 << 28
} eli_color_edit_flags;

/*============================================================================
 * ENUMS: Button Flags
 *===========================================================================*/

typedef enum eli_button_flags {
    ELI_BUTTON_FLAGS_NONE                      = 0,
    ELI_BUTTON_FLAGS_MOUSE_BUTTON_LEFT         = 1 << 0,
    ELI_BUTTON_FLAGS_MOUSE_BUTTON_RIGHT        = 1 << 1,
    ELI_BUTTON_FLAGS_MOUSE_BUTTON_MIDDLE       = 1 << 2
} eli_button_flags;

/*============================================================================
 * ENUMS: Drag Drop Flags
 *===========================================================================*/

typedef enum eli_drag_drop_flags {
    ELI_DRAG_DROP_FLAGS_NONE                        = 0,
    /* Source flags */
    ELI_DRAG_DROP_FLAGS_SOURCE_NO_PREVIEW_TOOLTIP   = 1 << 0,
    ELI_DRAG_DROP_FLAGS_SOURCE_NO_DISABLE_HOVER     = 1 << 1,
    ELI_DRAG_DROP_FLAGS_SOURCE_NO_HOLD_TO_OPEN_OTHERS = 1 << 2,
    ELI_DRAG_DROP_FLAGS_SOURCE_ALLOW_NULL_ID        = 1 << 3,
    ELI_DRAG_DROP_FLAGS_SOURCE_EXTERN               = 1 << 4,
    ELI_DRAG_DROP_FLAGS_PAYLOAD_AUTO_EXPIRE         = 1 << 5,
    ELI_DRAG_DROP_FLAGS_PAYLOAD_NO_CROSS_CONTEXT    = 1 << 6,
    ELI_DRAG_DROP_FLAGS_PAYLOAD_NO_CROSS_PROCESS    = 1 << 7,
    /* Target flags */
    ELI_DRAG_DROP_FLAGS_ACCEPT_BEFORE_DELIVERY      = 1 << 10,
    ELI_DRAG_DROP_FLAGS_ACCEPT_NO_DRAW_DEFAULT_RECT = 1 << 11,
    ELI_DRAG_DROP_FLAGS_ACCEPT_NO_PREVIEW_TOOLTIP   = 1 << 12,
    ELI_DRAG_DROP_FLAGS_ACCEPT_PEEK_ONLY = ELI_DRAG_DROP_FLAGS_ACCEPT_BEFORE_DELIVERY |
                                           ELI_DRAG_DROP_FLAGS_ACCEPT_NO_DRAW_DEFAULT_RECT
} eli_drag_drop_flags;

/*============================================================================
 * ENUMS: Keys
 *===========================================================================*/

typedef enum eli_key {
    ELI_KEY_NONE = 0,
    ELI_KEY_NAMED_KEY_BEGIN = 512,

    ELI_KEY_TAB = 512,
    ELI_KEY_LEFT_ARROW,
    ELI_KEY_RIGHT_ARROW,
    ELI_KEY_UP_ARROW,
    ELI_KEY_DOWN_ARROW,
    ELI_KEY_PAGE_UP,
    ELI_KEY_PAGE_DOWN,
    ELI_KEY_HOME,
    ELI_KEY_END,
    ELI_KEY_INSERT,
    ELI_KEY_DELETE,
    ELI_KEY_BACKSPACE,
    ELI_KEY_SPACE,
    ELI_KEY_ENTER,
    ELI_KEY_ESCAPE,
    ELI_KEY_LEFT_CTRL,
    ELI_KEY_LEFT_SHIFT,
    ELI_KEY_LEFT_ALT,
    ELI_KEY_LEFT_SUPER,
    ELI_KEY_RIGHT_CTRL,
    ELI_KEY_RIGHT_SHIFT,
    ELI_KEY_RIGHT_ALT,
    ELI_KEY_RIGHT_SUPER,
    ELI_KEY_MENU,
    ELI_KEY_0, ELI_KEY_1, ELI_KEY_2, ELI_KEY_3, ELI_KEY_4,
    ELI_KEY_5, ELI_KEY_6, ELI_KEY_7, ELI_KEY_8, ELI_KEY_9,
    ELI_KEY_A, ELI_KEY_B, ELI_KEY_C, ELI_KEY_D, ELI_KEY_E,
    ELI_KEY_F, ELI_KEY_G, ELI_KEY_H, ELI_KEY_I, ELI_KEY_J,
    ELI_KEY_K, ELI_KEY_L, ELI_KEY_M, ELI_KEY_N, ELI_KEY_O,
    ELI_KEY_P, ELI_KEY_Q, ELI_KEY_R, ELI_KEY_S, ELI_KEY_T,
    ELI_KEY_U, ELI_KEY_V, ELI_KEY_W, ELI_KEY_X, ELI_KEY_Y,
    ELI_KEY_Z,
    ELI_KEY_F1, ELI_KEY_F2, ELI_KEY_F3, ELI_KEY_F4, ELI_KEY_F5, ELI_KEY_F6,
    ELI_KEY_F7, ELI_KEY_F8, ELI_KEY_F9, ELI_KEY_F10, ELI_KEY_F11, ELI_KEY_F12,
    ELI_KEY_F13, ELI_KEY_F14, ELI_KEY_F15, ELI_KEY_F16, ELI_KEY_F17, ELI_KEY_F18,
    ELI_KEY_F19, ELI_KEY_F20, ELI_KEY_F21, ELI_KEY_F22, ELI_KEY_F23, ELI_KEY_F24,
    ELI_KEY_APOSTROPHE,
    ELI_KEY_COMMA,
    ELI_KEY_MINUS,
    ELI_KEY_PERIOD,
    ELI_KEY_SLASH,
    ELI_KEY_SEMICOLON,
    ELI_KEY_EQUAL,
    ELI_KEY_LEFT_BRACKET,
    ELI_KEY_BACKSLASH,
    ELI_KEY_RIGHT_BRACKET,
    ELI_KEY_GRAVE_ACCENT,
    ELI_KEY_CAPS_LOCK,
    ELI_KEY_SCROLL_LOCK,
    ELI_KEY_NUM_LOCK,
    ELI_KEY_PRINT_SCREEN,
    ELI_KEY_PAUSE,
    ELI_KEY_KEYPAD_0, ELI_KEY_KEYPAD_1, ELI_KEY_KEYPAD_2, ELI_KEY_KEYPAD_3, ELI_KEY_KEYPAD_4,
    ELI_KEY_KEYPAD_5, ELI_KEY_KEYPAD_6, ELI_KEY_KEYPAD_7, ELI_KEY_KEYPAD_8, ELI_KEY_KEYPAD_9,
    ELI_KEY_KEYPAD_DECIMAL,
    ELI_KEY_KEYPAD_DIVIDE,
    ELI_KEY_KEYPAD_MULTIPLY,
    ELI_KEY_KEYPAD_SUBTRACT,
    ELI_KEY_KEYPAD_ADD,
    ELI_KEY_KEYPAD_ENTER,
    ELI_KEY_KEYPAD_EQUAL,
    ELI_KEY_APP_BACK,
    ELI_KEY_APP_FORWARD,
    ELI_KEY_OEM_102,

    /* Gamepad */
    ELI_KEY_GAMEPAD_START,
    ELI_KEY_GAMEPAD_BACK,
    ELI_KEY_GAMEPAD_FACE_LEFT,
    ELI_KEY_GAMEPAD_FACE_RIGHT,
    ELI_KEY_GAMEPAD_FACE_UP,
    ELI_KEY_GAMEPAD_FACE_DOWN,
    ELI_KEY_GAMEPAD_DPAD_LEFT,
    ELI_KEY_GAMEPAD_DPAD_RIGHT,
    ELI_KEY_GAMEPAD_DPAD_UP,
    ELI_KEY_GAMEPAD_DPAD_DOWN,
    ELI_KEY_GAMEPAD_L1,
    ELI_KEY_GAMEPAD_R1,
    ELI_KEY_GAMEPAD_L2,
    ELI_KEY_GAMEPAD_R2,
    ELI_KEY_GAMEPAD_L3,
    ELI_KEY_GAMEPAD_R3,
    ELI_KEY_GAMEPAD_L_STICK_LEFT,
    ELI_KEY_GAMEPAD_L_STICK_RIGHT,
    ELI_KEY_GAMEPAD_L_STICK_UP,
    ELI_KEY_GAMEPAD_L_STICK_DOWN,
    ELI_KEY_GAMEPAD_R_STICK_LEFT,
    ELI_KEY_GAMEPAD_R_STICK_RIGHT,
    ELI_KEY_GAMEPAD_R_STICK_UP,
    ELI_KEY_GAMEPAD_R_STICK_DOWN,

    /* Mouse buttons as keys */
    ELI_KEY_MOUSE_LEFT,
    ELI_KEY_MOUSE_RIGHT,
    ELI_KEY_MOUSE_MIDDLE,
    ELI_KEY_MOUSE_X1,
    ELI_KEY_MOUSE_X2,
    ELI_KEY_MOUSE_WHEEL_X,
    ELI_KEY_MOUSE_WHEEL_Y,

    /* Reserved for mod storage */
    ELI_KEY_RESERVED_FOR_MOD_CTRL,
    ELI_KEY_RESERVED_FOR_MOD_SHIFT,
    ELI_KEY_RESERVED_FOR_MOD_ALT,
    ELI_KEY_RESERVED_FOR_MOD_SUPER,

    ELI_KEY_NAMED_KEY_END,
    ELI_KEY_NAMED_KEY_COUNT = ELI_KEY_NAMED_KEY_END - ELI_KEY_NAMED_KEY_BEGIN,

    /* Keyboard modifiers */
    ELI_MOD_NONE  = 0,
    ELI_MOD_CTRL  = 1 << 12,
    ELI_MOD_SHIFT = 1 << 13,
    ELI_MOD_ALT   = 1 << 14,
    ELI_MOD_SUPER = 1 << 15,
    ELI_MOD_MASK  = 0xF000
} eli_key;

/*============================================================================
 * ENUMS: Mouse Button
 *===========================================================================*/

typedef enum eli_mouse_button {
    ELI_MOUSE_BUTTON_LEFT   = 0,
    ELI_MOUSE_BUTTON_RIGHT  = 1,
    ELI_MOUSE_BUTTON_MIDDLE = 2,
    ELI_MOUSE_BUTTON_COUNT  = 5
} eli_mouse_button;

/*============================================================================
 * ENUMS: Mouse Cursor
 *===========================================================================*/

typedef enum eli_mouse_cursor {
    ELI_MOUSE_CURSOR_NONE       = -1,
    ELI_MOUSE_CURSOR_ARROW      = 0,
    ELI_MOUSE_CURSOR_TEXT_INPUT,
    ELI_MOUSE_CURSOR_RESIZE_ALL,
    ELI_MOUSE_CURSOR_RESIZE_NS,
    ELI_MOUSE_CURSOR_RESIZE_EW,
    ELI_MOUSE_CURSOR_RESIZE_NESW,
    ELI_MOUSE_CURSOR_RESIZE_NWSE,
    ELI_MOUSE_CURSOR_HAND,
    ELI_MOUSE_CURSOR_WAIT,
    ELI_MOUSE_CURSOR_PROGRESS,
    ELI_MOUSE_CURSOR_NOT_ALLOWED,
    ELI_MOUSE_CURSOR_COUNT
} eli_mouse_cursor;

/*============================================================================
 * ENUMS: Color Indices
 *===========================================================================*/

typedef enum eli_col {
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
    ELI_COL_INPUT_TEXT_CURSOR,
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
    ELI_COL_TREE_LINES,
    ELI_COL_DRAG_DROP_TARGET,
    ELI_COL_DRAG_DROP_TARGET_BG,
    ELI_COL_UNSAVED_MARKER,
    ELI_COL_NAV_CURSOR,
    ELI_COL_NAV_WINDOWING_HIGHLIGHT,
    ELI_COL_NAV_WINDOWING_DIM_BG,
    ELI_COL_MODAL_WINDOW_DIM_BG,
    ELI_COL_COUNT
} eli_col;

/*============================================================================
 * ENUMS: Style Variables
 *===========================================================================*/

typedef enum eli_style_var {
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
    ELI_STYLE_VAR_SCROLLBAR_PADDING,
    ELI_STYLE_VAR_GRAB_MIN_SIZE,
    ELI_STYLE_VAR_GRAB_ROUNDING,
    ELI_STYLE_VAR_IMAGE_ROUNDING,
    ELI_STYLE_VAR_IMAGE_BORDER_SIZE,
    ELI_STYLE_VAR_TAB_ROUNDING,
    ELI_STYLE_VAR_TAB_BORDER_SIZE,
    ELI_STYLE_VAR_TAB_MIN_WIDTH_BASE,
    ELI_STYLE_VAR_TAB_MIN_WIDTH_SHRINK,
    ELI_STYLE_VAR_TAB_BAR_BORDER_SIZE,
    ELI_STYLE_VAR_TAB_BAR_OVERLINE_SIZE,
    ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_ANGLE,
    ELI_STYLE_VAR_TABLE_ANGLED_HEADERS_TEXT_ALIGN,
    ELI_STYLE_VAR_TREE_LINES_SIZE,
    ELI_STYLE_VAR_TREE_LINES_ROUNDING,
    ELI_STYLE_VAR_BUTTON_TEXT_ALIGN,
    ELI_STYLE_VAR_SELECTABLE_TEXT_ALIGN,
    ELI_STYLE_VAR_SEPARATOR_TEXT_BORDER_SIZE,
    ELI_STYLE_VAR_SEPARATOR_TEXT_ALIGN,
    ELI_STYLE_VAR_SEPARATOR_TEXT_PADDING,
    ELI_STYLE_VAR_COUNT
} eli_style_var;

/*============================================================================
 * STRUCTURES: IO
 *===========================================================================*/

#define ELI_KEY_DATA_SIZE 154

typedef struct eli_key_data {
    bool down;
    float down_duration;
    float down_duration_prev;
    float analog_value;
} eli_key_data;

struct eli_io {
    /* Configuration (fill once) */
    float display_size_x;
    float display_size_y;
    float delta_time;
    float ini_saving_rate;
    const char* ini_filename;
    const char* log_filename;
    float font_global_scale;
    bool font_allow_user_scaling;
    eli_font* font_default;

    /* Input state */
    eli_vec2 mouse_pos;
    bool mouse_down[5];
    float mouse_wheel;
    float mouse_wheel_h;
    bool key_ctrl;
    bool key_shift;
    bool key_alt;
    bool key_super;
    eli_key_data keys_data[ELI_KEY_DATA_SIZE];

    /* Text input */
    uint32_t input_queue_chars[16];
    int input_queue_chars_count;

    /* Output state */
    bool want_capture_mouse;
    bool want_capture_keyboard;
    bool want_text_input;
    bool want_set_mouse_pos;
    bool want_save_ini_settings;
    eli_mouse_cursor mouse_cursor;

    /* Framerate info */
    float framerate;
    int metrics_render_vertices;
    int metrics_render_indices;
    int metrics_render_windows;
    int metrics_active_windows;

    /* Mouse delta */
    eli_vec2 mouse_delta;

    /* Internal */
    eli_context* ctx;
};

/*============================================================================
 * STRUCTURES: Style
 *===========================================================================*/

struct eli_style {
    /* Main */
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
    eli_vec2 scrollbar_padding;
    float grab_min_size;
    float grab_rounding;
    float log_slider_deadzone;
    float tab_rounding;
    float tab_border_size;
    float tab_min_width_base;
    float tab_min_width_shrink;
    float tab_bar_border_size;
    float tab_bar_overline_size;
    float table_angled_headers_angle;
    eli_vec2 table_angled_headers_text_align;
    float tree_lines_size;
    float tree_lines_rounding;
    float image_rounding;
    float image_border_size;
    eli_vec2 button_text_align;
    eli_vec2 selectable_text_align;
    float separator_text_border_size;
    eli_vec2 separator_text_align;
    eli_vec2 separator_text_padding;
    eli_vec2 display_window_padding;
    eli_vec2 display_safe_area_padding;
    float mouse_cursor_scale;
    bool anti_aliased_lines;
    bool anti_aliased_lines_use_tex;
    bool anti_aliased_fill;
    float curve_tessellation_tol;
    float circle_tessellation_max_error;

    /* Colors */
    eli_vec4 colors[ELI_COL_COUNT];

    /* Hover/delay settings */
    float hover_stationary_delay;
    float hover_delay_short;
    float hover_delay_normal;
    int hover_flags_for_tooltip_mouse;
    int hover_flags_for_tooltip_nav;
};

/*============================================================================
 * STRUCTURES: Context
 *===========================================================================*/

struct eli_context {
    eli_io io;
    eli_style style;
    eli_font* font;
    float font_size;
    float font_base_size;

    /* Frame state */
    int frame_count;
    bool frame_count_ended;
    bool within_frame_scope;

    /* ID state */
    eli_id active_id;
    eli_id hot_id;

    /* Draw data */
    eli_draw_data* draw_data;
    eli_draw_list* draw_list;

    /* Windows (will be expanded later) */
    void* windows;
    int windows_count;

    /* Active state */
    bool initialized;
};

/*============================================================================
 * CONTEXT FUNCTIONS
 *===========================================================================*/

/* Create a new context */
static inline eli_context* eli_create_context(void) {
    eli_context* ctx = (eli_context*)malloc(sizeof(eli_context));
    if (!ctx) return NULL;

    /* Zero initialize */
    memset(ctx, 0, sizeof(eli_context));

    /* Set defaults */
    ctx->io.delta_time = 1.0f / 60.0f;
    ctx->io.ini_saving_rate = 5.0f;
    ctx->io.font_global_scale = 1.0f;
    ctx->io.mouse_cursor = ELI_MOUSE_CURSOR_ARROW;
    ctx->io.mouse_pos.x = -FLT_MAX;
    ctx->io.mouse_pos.y = -FLT_MAX;
    ctx->io.ctx = ctx;

    /* Initialize style with dark theme defaults */
    ctx->style.alpha = 1.0f;
    ctx->style.disabled_alpha = 0.6f;
    ctx->style.window_padding.x = 8.0f;
    ctx->style.window_padding.y = 8.0f;
    ctx->style.window_rounding = 0.0f;
    ctx->style.window_border_size = 1.0f;
    ctx->style.window_min_size.x = 32.0f;
    ctx->style.window_min_size.y = 32.0f;
    ctx->style.window_title_align.x = 0.0f;
    ctx->style.window_title_align.y = 0.5f;
    ctx->style.window_menu_button_position = ELI_DIR_LEFT;
    ctx->style.child_rounding = 0.0f;
    ctx->style.child_border_size = 1.0f;
    ctx->style.popup_rounding = 0.0f;
    ctx->style.popup_border_size = 1.0f;
    ctx->style.frame_padding.x = 4.0f;
    ctx->style.frame_padding.y = 3.0f;
    ctx->style.frame_rounding = 0.0f;
    ctx->style.frame_border_size = 0.0f;
    ctx->style.item_spacing.x = 8.0f;
    ctx->style.item_spacing.y = 4.0f;
    ctx->style.item_inner_spacing.x = 4.0f;
    ctx->style.item_inner_spacing.y = 4.0f;
    ctx->style.cell_padding.x = 4.0f;
    ctx->style.cell_padding.y = 2.0f;
    ctx->style.touch_extra_padding.x = 0.0f;
    ctx->style.touch_extra_padding.y = 0.0f;
    ctx->style.indent_spacing = 21.0f;
    ctx->style.columns_min_spacing = 6.0f;
    ctx->style.scrollbar_size = 14.0f;
    ctx->style.scrollbar_rounding = 9.0f;
    ctx->style.grab_min_size = 12.0f;
    ctx->style.grab_rounding = 0.0f;
    ctx->style.log_slider_deadzone = 4.0f;
    ctx->style.tab_rounding = 4.0f;
    ctx->style.tab_border_size = 0.0f;
    ctx->style.tab_min_width_base = 44.0f;
    ctx->style.tab_min_width_shrink = 4.0f;
    ctx->style.tab_bar_border_size = 1.0f;
    ctx->style.tab_bar_overline_size = 2.0f;
    ctx->style.table_angled_headers_angle = 35.0f * 0.0174533f; /* 35 degrees in radians */
    ctx->style.button_text_align.x = 0.5f;
    ctx->style.button_text_align.y = 0.5f;
    ctx->style.selectable_text_align.x = 0.0f;
    ctx->style.selectable_text_align.y = 0.0f;
    ctx->style.separator_text_border_size = 3.0f;
    ctx->style.separator_text_align.x = 0.0f;
    ctx->style.separator_text_align.y = 0.5f;
    ctx->style.display_window_padding.x = 19.0f;
    ctx->style.display_window_padding.y = 19.0f;
    ctx->style.display_safe_area_padding.x = 3.0f;
    ctx->style.display_safe_area_padding.y = 3.0f;
    ctx->style.mouse_cursor_scale = 1.0f;
    ctx->style.anti_aliased_lines = true;
    ctx->style.anti_aliased_lines_use_tex = true;
    ctx->style.anti_aliased_fill = true;
    ctx->style.curve_tessellation_tol = 1.25f;
    ctx->style.circle_tessellation_max_error = 0.3f;
    ctx->style.hover_stationary_delay = 0.15f;
    ctx->style.hover_delay_short = 0.15f;
    ctx->style.hover_delay_normal = 0.4f;

    ctx->initialized = true;
    return ctx;
}

/* Destroy a context */
static inline void eli_destroy_context(eli_context* ctx) {
    if (!ctx) return;
    free(ctx);
}

/* Global context pointer */
static eli_context* g_eli_context = NULL;

/* Get current context */
static inline eli_context* eli_get_current_context(void) {
    return g_eli_context;
}

/* Set current context */
static inline void eli_set_current_context(eli_context* ctx) {
    g_eli_context = ctx;
}

/* Get IO from current context */
static inline eli_io* eli_get_io(void) {
    return &g_eli_context->io;
}

/* Get style from current context */
static inline eli_style* eli_get_style(void) {
    return &g_eli_context->style;
}

/*============================================================================
 * FRAME LIFECYCLE
 *===========================================================================*/

/* Begin a new frame */
static inline void eli_new_frame(void) {
    eli_context* ctx = g_eli_context;
    if (!ctx) return;

    ctx->frame_count++;
    ctx->within_frame_scope = true;
    ctx->frame_count_ended = false;

    /* Calculate mouse delta */
    if (ctx->io.mouse_pos.x >= -FLT_MAX && ctx->io.mouse_pos.y >= -FLT_MAX) {
        /* Valid mouse position - delta calculated in input handling */
    }

    /* Reset per-frame state */
    ctx->io.want_capture_mouse = false;
    ctx->io.want_capture_keyboard = false;
    ctx->io.want_text_input = false;
}

/* End the current frame */
static inline void eli_end_frame(void) {
    eli_context* ctx = g_eli_context;
    if (!ctx) return;

    ctx->within_frame_scope = false;
    ctx->frame_count_ended = true;
}

/* Render the frame (generates draw data) */
static inline void eli_render(void) {
    eli_context* ctx = g_eli_context;
    if (!ctx) return;

    if (!ctx->frame_count_ended) {
        eli_end_frame();
    }

    /* Draw data generation will be implemented with draw system */
}

/* Get draw data for rendering */
static inline eli_draw_data* eli_get_draw_data(void) {
    eli_context* ctx = g_eli_context;
    if (!ctx) return NULL;
    return ctx->draw_data;
}

/*============================================================================
 * UTILITY HELPERS
 *===========================================================================*/

/* Create a vec2 */
static inline eli_vec2 eli_make_vec2(float x, float y) {
    eli_vec2 v = { x, y };
    return v;
}

/* Create a vec4 */
static inline eli_vec4 eli_make_vec4(float x, float y, float z, float w) {
    eli_vec4 v = { x, y, z, w };
    return v;
}

/* Create a rect from min/max points */
static inline eli_rect eli_make_rect(float min_x, float min_y, float max_x, float max_y) {
    eli_rect r;
    r.min.x = min_x;
    r.min.y = min_y;
    r.max.x = max_x;
    r.max.y = max_y;
    return r;
}

/* Check if rect contains a point */
static inline bool eli_rect_contains(eli_rect r, eli_vec2 p) {
    return p.x >= r.min.x && p.x < r.max.x && p.y >= r.min.y && p.y < r.max.y;
}

/* Get rect width */
static inline float eli_rect_width(eli_rect r) {
    return r.max.x - r.min.x;
}

/* Get rect height */
static inline float eli_rect_height(eli_rect r) {
    return r.max.y - r.min.y;
}

/* Get rect size */
static inline eli_vec2 eli_rect_size(eli_rect r) {
    return eli_make_vec2(r.max.x - r.min.x, r.max.y - r.min.y);
}

/* Color conversion: u32 to vec4 */
static inline eli_vec4 eli_color_u32_to_vec4(uint32_t col) {
    float s = 1.0f / 255.0f;
    return eli_make_vec4(
        (float)((col >> ELI_COL32_R_SHIFT) & 0xFF) * s,
        (float)((col >> ELI_COL32_G_SHIFT) & 0xFF) * s,
        (float)((col >> ELI_COL32_B_SHIFT) & 0xFF) * s,
        (float)((col >> ELI_COL32_A_SHIFT) & 0xFF) * s
    );
}

/* Color conversion: vec4 to u32 */
static inline uint32_t eli_color_vec4_to_u32(eli_vec4 col) {
    uint32_t r = (uint32_t)(col.x * 255.0f + 0.5f);
    uint32_t g = (uint32_t)(col.y * 255.0f + 0.5f);
    uint32_t b = (uint32_t)(col.z * 255.0f + 0.5f);
    uint32_t a = (uint32_t)(col.w * 255.0f + 0.5f);
    return ELI_COL32(r, g, b, a);
}

#ifdef __cplusplus
}
#endif

#endif /* ELIMGUI_H */
