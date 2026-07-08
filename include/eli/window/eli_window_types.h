/**
 * @file eli_window_types.h
 * @brief Window-system data types: the eli_window structure (identity, geometry,
 *        scroll, collapse/active state, decoration rects, and the embedded draw
 *        list) and the next-window settings block written by the set_next_window_*
 *        API and consumed by the following eli_begin.
 *
 * These are pure data definitions plus a few inline geometry helpers; they carry
 * no dependency on the context so the context can forward-declare and store them.
 *
 * @status Phase 7 window types in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_TYPES_H
#define ELI_WINDOW_ELI_WINDOW_TYPES_H

#include "../draw/eli_draw.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_enums.h"

/* Fallback font size used to size the title bar when no font is bound yet, so a
 * window's decorations have deterministic height in headless (test) contexts. */
#define ELI_WINDOW_DEFAULT_FONT_SIZE 13.0f

/* Default top-left position of a freshly created window (matches Dear ImGui). */
#define ELI_WINDOW_DEFAULT_POS_X 60.0f
#define ELI_WINDOW_DEFAULT_POS_Y 60.0f

/* Sentinel meaning "no pending scroll target on this axis". */
#define ELI_WINDOW_SCROLL_NONE (-FLT_MAX)

/* ---------------------------------------------------------------------------
 * Next-window settings
 * ------------------------------------------------------------------------- */

/**
 * One-shot settings applied to the next window opened with eli_begin. Populated
 * by the set_next_window_* family and cleared once consumed. `has_*` flags gate
 * which fields are live; the *_cond fields carry the eli_cond for the setting.
 */
typedef struct eli_next_window_data {
    bool has_pos;
    bool has_size;
    bool has_size_constraint;
    bool has_content_size;
    bool has_collapsed;
    bool has_focus;
    bool has_scroll;
    bool has_bg_alpha;

    eli_cond pos_cond;
    eli_cond size_cond;
    eli_cond collapsed_cond;

    eli_vec2 pos_val;
    eli_vec2 pos_pivot;
    eli_vec2 size_val;
    eli_vec2 size_constraint_min;
    eli_vec2 size_constraint_max;
    eli_vec2 content_size_val;
    bool     collapsed_val;
    eli_vec2 scroll_val;      /* per-axis; < 0 leaves that axis unchanged */
    float    bg_alpha_val;

    /* Docking (Phase 34): dock id the next window should attach to. */
    bool     has_dock_id;
    eli_id   dock_id_val;
    eli_cond dock_cond;
} eli_next_window_data;

/* ---------------------------------------------------------------------------
 * Window
 * ------------------------------------------------------------------------- */

/**
 * A single window's persistent + per-frame state. Identity (id/name/flags) and
 * geometry (pos/size/scroll/collapsed) persist across frames; the decoration
 * rects, clip rect, and the active/appearing/skip_items flags are recomputed at
 * eli_begin. Each window owns its draw_list (released by the window shutdown
 * hook). Child windows link to their parent and to the top-level root_window.
 */
typedef struct eli_window {
    eli_id           id;
    char            *name;              /* heap copy of the begin() name */
    eli_window_flags flags;
    eli_child_flags  child_flags;       /* child-only flags (0 for top-level) */

    /* Geometry (top-left origin). size is the collapsed-or-full size actually
     * used this frame; size_full is the size when expanded. */
    eli_vec2 pos;
    eli_vec2 size;
    eli_vec2 size_full;

    /* Content extent driving the scroll range. content_size is the value used
     * this frame; content_size_explicit is set via set_next_window_content_size;
     * content_size_measured is derived from the layout cursor (later phases). */
    eli_vec2 content_size;
    eli_vec2 content_size_explicit;
    bool     content_size_explicit_valid;
    eli_vec2 content_size_measured;

    /* Scrolling. */
    eli_vec2 scroll;
    eli_vec2 scroll_max;

    /* Layout cursor (advanced by widgets in later phases; seeded at Begin). */
    eli_vec2 cursor_pos;
    eli_vec2 cursor_start_pos;
    eli_vec2 cursor_max_pos;

    /* Layout advancement state (Phase 8). Mirrors Dear ImGui's window DrawContext:
     * per-line geometry used by eli_item_size/eli_same_line to place items, plus
     * the current indent/group offset (both measured from win->pos.x) and the
     * per-line text-baseline offsets used to vertically align mixed-height rows.
     * All reset each frame at eli_begin. */
    eli_vec2 cursor_pos_prev_line;              /* end of the previous item's line */
    eli_vec2 curr_line_size;                    /* max item size accumulated this line */
    eli_vec2 prev_line_size;                    /* size of the completed previous line */
    float    curr_line_text_baseline_offset;    /* text baseline offset this line */
    float    prev_line_text_baseline_offset;    /* text baseline offset previous line */
    float    indent;                            /* current indent from pos.x (incl. padding) */
    float    group_offset;                      /* group left offset from pos.x */
    float    item_width;                        /* current default item width (<=0 => auto) */
    float    item_width_default;                /* window's baseline default item width */
    bool     is_same_line;                      /* next item continues the current line */
    bool     is_set_pos;                        /* cursor was moved by set_cursor_pos* */

    /* State flags. */
    bool collapsed;
    bool active;                 /* begun this frame */
    bool was_active;             /* begun last frame */
    bool appearing;              /* first active frame after being (re)shown */
    bool hidden;
    bool skip_items;             /* true when contents must not be emitted */
    bool has_scrollbar_x;
    bool has_scrollbar_y;

    int   begin_count;           /* Begin calls this frame */
    int   last_frame_active;     /* frame index of the last active frame */
    float font_window_scale;     /* per-window font scale multiplier */

    /* Decoration + work rects (recomputed each Begin). */
    eli_rect outer_rect;         /* pos .. pos+size */
    eli_rect title_bar_rect;     /* top decoration strip */
    eli_rect inner_rect;         /* window minus title bar */
    eli_rect content_region_rect;/* work area: inner minus padding and scrollbars */
    eli_rect clip_rect;          /* clip applied to contents */

    eli_vec2 window_padding;
    float    title_bar_height;

    /* Hierarchy + focus. */
    struct eli_window *parent_window;
    struct eli_window *root_window;
    int   focus_order;           /* index within the context focus-order list */

    /* Docking (Phase 34). dock_id is the node this window wants to attach to
     * (0 == floating); it persists across frames. dock_node is the node resolved
     * this frame by the docking begin hook (NULL == floating). When docked,
     * dock_body_rect is the node's body region (node minus the shared tab strip)
     * the window fills, and dock_is_visible is true only for the node's selected
     * tab (the one window that emits its contents). */
    eli_id                dock_id;
    struct eli_dock_node *dock_node;
    eli_rect              dock_body_rect;
    bool                  dock_is_visible;

    eli_draw_list draw_list;     /* owned; released by the shutdown hook */
} eli_window;

/* ---------------------------------------------------------------------------
 * Inline geometry helpers (pure)
 * ------------------------------------------------------------------------- */

/** @return true if the window has a visible title bar. */
static inline bool eli_window_has_title_bar(const eli_window *win)
{
    return (win->flags & ELI_WINDOW_NO_TITLEBAR) == 0;
}

/** @return true if the window can show scrollbars at all. */
static inline bool eli_window_allows_scrollbar(const eli_window *win)
{
    return (win->flags & ELI_WINDOW_NO_SCROLLBAR) == 0;
}

#endif /* ELI_WINDOW_ELI_WINDOW_TYPES_H */
