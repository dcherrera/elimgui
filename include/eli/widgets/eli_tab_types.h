/**
 * @file eli_tab_types.h
 * @brief Data types for the Phase 20 tab-bar widget: the eli_tab_bar_flags and
 *        eli_tab_item_flags enums plus the persistent eli_tab_item / eli_tab_bar
 *        records the pool stores across frames.
 *
 * A tab bar keeps an ordered, persistent list of tab items so it can lay them out
 * (widths, offsets), track the selected/visible tab across frames, scroll when the
 * tabs overflow, and reorder them by drag. Mirrors Dear ImGui's ImGuiTabBar /
 * ImGuiTabItem storage, trimmed to a single (central) tab section.
 *
 * @status Phase 20 tab-bar types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WIDGETS_ELI_TAB_TYPES_H
#define ELI_WIDGETS_ELI_TAB_TYPES_H

#include "../core/eli_types.h"

/* ---------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------- */

/** Flags for eli_begin_tab_bar (see enum eli_tab_bar_flags_). */
typedef int eli_tab_bar_flags;
enum eli_tab_bar_flags_ {
    ELI_TAB_BAR_NONE                          = 0,
    ELI_TAB_BAR_REORDERABLE                   = 1 << 0,  /* drag tabs to re-order */
    ELI_TAB_BAR_AUTO_SELECT_NEW_TABS          = 1 << 1,  /* select tabs as they appear */
    ELI_TAB_BAR_NO_CLOSE_WITH_MIDDLE_MOUSE    = 1 << 3,  /* disable middle-click close */
    ELI_TAB_BAR_NO_TAB_LIST_SCROLLING_BUTTONS = 1 << 4,  /* (no-op: buttons deferred) */
    ELI_TAB_BAR_NO_TOOLTIP                    = 1 << 5,  /* (no-op: tooltips deferred) */
    ELI_TAB_BAR_DRAW_SELECTED_OVERLINE        = 1 << 6,  /* overline over selected tab */

    /* Fitting policy: how tabs behave when they don't fit the bar width. */
    ELI_TAB_BAR_FITTING_POLICY_SHRINK         = 1 << 8,  /* shrink tabs to fit */
    ELI_TAB_BAR_FITTING_POLICY_SCROLL         = 1 << 9,  /* scroll when tabs overflow */
    ELI_TAB_BAR_FITTING_POLICY_MASK           = ELI_TAB_BAR_FITTING_POLICY_SHRINK |
                                                ELI_TAB_BAR_FITTING_POLICY_SCROLL,
    ELI_TAB_BAR_FITTING_POLICY_DEFAULT        = ELI_TAB_BAR_FITTING_POLICY_SHRINK
};

/** Flags for eli_begin_tab_item / eli_tab_item_button (see enum eli_tab_item_flags_). */
typedef int eli_tab_item_flags;
enum eli_tab_item_flags_ {
    ELI_TAB_ITEM_NONE                         = 0,
    ELI_TAB_ITEM_UNSAVED_DOCUMENT             = 1 << 0,  /* show an unsaved dot marker */
    ELI_TAB_ITEM_SET_SELECTED                 = 1 << 1,  /* select this tab this frame */
    ELI_TAB_ITEM_NO_CLOSE_WITH_MIDDLE_MOUSE   = 1 << 2,  /* disable middle-click close */
    ELI_TAB_ITEM_NO_PUSH_ID                   = 1 << 3,  /* skip push/pop id on begin/end */
    ELI_TAB_ITEM_NO_TOOLTIP                   = 1 << 4,  /* (no-op: tooltips deferred) */
    ELI_TAB_ITEM_NO_REORDER                   = 1 << 5,  /* pin this tab against reorder */

    /* Private (elimgui) bits. */
    ELI_TAB_ITEM_NO_CLOSE_BUTTON              = 1 << 20, /* hide the close button */
    ELI_TAB_ITEM_BUTTON                       = 1 << 21  /* pressable button, never selected */
};

/* ---------------------------------------------------------------------------
 * Persistent records
 * ------------------------------------------------------------------------- */

/** Width capping: a tab never grows wider than this multiple of the font size. */
#define ELI_TAB_MAX_WIDTH_FONT_MULT 20.0f

/**
 * One tab inside a tab bar. Persisted in the bar's tab array across frames so the
 * bar can retain order, selection recency, and the last-visible frame used to
 * garbage-collect tabs that stop being submitted.
 */
typedef struct eli_tab_item {
    eli_id             id;
    eli_tab_item_flags flags;
    int                last_frame_visible;   /* frame this tab was last submitted */
    int                last_frame_selected;  /* frame this tab was last selected */
    float              offset;               /* x offset from the bar start */
    float              width;                /* displayed width (post-shrink) */
    float              content_width;        /* measured ideal width */
    int                begin_order;          /* submission order this frame */
    bool               want_close;           /* set by eli_set_tab_item_closed */
} eli_tab_item;

/**
 * A tab bar's persistent state. Lives in the module-private pool keyed by id;
 * released by eli_tab_shutdown. The tab array is heap-grown as tabs are added and
 * compacted during layout.
 */
typedef struct eli_tab_bar {
    eli_id            id;
    eli_tab_bar_flags flags;

    eli_tab_item     *tabs;
    int               tab_count;
    int               tab_capacity;

    eli_id            selected_tab_id;       /* current selection (locked at layout) */
    eli_id            next_selected_tab_id;  /* queued selection applied next layout */
    eli_id            visible_tab_id;        /* contents-visible tab (== selected here) */

    int               curr_frame_visible;
    int               prev_frame_visible;

    eli_rect          bar_rect;              /* the tab strip rect in screen space */
    float             bar_rect_prev_width;
    float             width_all_tabs;        /* laid-out total width of all tabs */
    float             width_all_tabs_ideal;  /* unshrunk total width */

    float             scrolling_anim;        /* applied horizontal scroll */
    float             scrolling_target;      /* desired horizontal scroll */

    eli_id            reorder_request_tab_id;
    int               reorder_request_offset;

    int               tabs_active_count;     /* tabs submitted this frame */
    int               last_tab_item_idx;     /* index of the last begin_tab_item tab */

    eli_vec2          frame_padding;         /* style frame padding locked at begin */
    float             item_spacing_y;
    eli_vec2          backup_cursor_pos;     /* window cursor saved at begin */
    float             curr_tabs_contents_height;
    float             prev_tabs_contents_height;

    int               begin_count;           /* begin_tab_bar calls this frame */
    bool              want_layout;           /* layout pending before first item */
    bool              visible_tab_was_submitted;
    bool              tabs_added_new;         /* a new tab appeared last frame */
} eli_tab_bar;

#endif /* ELI_WIDGETS_ELI_TAB_TYPES_H */
