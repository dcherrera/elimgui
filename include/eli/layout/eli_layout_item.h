/**
 * @file eli_layout_item.h
 * @brief Shared item-layout primitives every widget builds on: eli_item_size /
 *        eli_item_add (cursor advancement, item registration, clip/hover tests),
 *        the last-item record accessors, and the content-region queries.
 *
 * These mirror Dear ImGui's ItemSize/ItemAdd contract. A widget measures itself,
 * calls eli_item_size to advance the window's layout cursor by that size plus the
 * style item spacing, then eli_item_add to register the item's id/rect/status so
 * later phases (interaction, item queries) can read it back.
 *
 * @status Phase 8 item-layout core in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_ITEM_H
#define ELI_LAYOUT_ELI_LAYOUT_ITEM_H

#include "../window/eli_window.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../core/eli_types.h"
#include "../input/eli_input.h"

/* ---------------------------------------------------------------------------
 * Item status flags
 *
 * Written by eli_item_add onto the context's last-item record and consumed by the
 * item-query APIs added in a later phase. Distinct from eli_item_flags (which are
 * input-behavior flags pushed by the caller).
 * ------------------------------------------------------------------------- */

typedef int eli_item_status_flags;
enum eli_item_status_flags_ {
    ELI_ITEM_STATUS_NONE         = 0,
    ELI_ITEM_STATUS_HOVERED_RECT = 1 << 0,  /* mouse is over the item's rect */
    ELI_ITEM_STATUS_VISIBLE      = 1 << 1,  /* item rect overlaps the clip rect */
    ELI_ITEM_STATUS_EDITED       = 1 << 2,  /* value was modified this frame */
    ELI_ITEM_STATUS_TOGGLED_SELECTION = 1 << 3,
    ELI_ITEM_STATUS_TOGGLED_OPEN = 1 << 4,
    ELI_ITEM_STATUS_HAS_DEACTIVATED = 1 << 5,
    ELI_ITEM_STATUS_DEACTIVATED  = 1 << 6
};

/* ---------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------- */

/** Truncate toward zero to a pixel boundary (Dear ImGui IM_TRUNC equivalent). */
static inline float eli_layout_trunc(float v)
{
    return (float)(long)v;
}

/** @return true if two rectangles overlap (min-inclusive, max-exclusive). */
static inline bool eli_rect_overlaps(eli_rect a, eli_rect b)
{
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}

/** @return the current window, or NULL when no window scope is active. */
static inline eli_window *eli_layout_current_window(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->current_window : NULL;
}

/* ---------------------------------------------------------------------------
 * Item size — cursor advancement
 * ------------------------------------------------------------------------- */

/**
 * Advance the current window's layout cursor to account for an item of the given
 * size, then move the cursor to the start of the next line (adding item spacing).
 * Updates the current/previous line heights, the content max extent, and the
 * text-baseline bookkeeping used to vertically align mixed-height rows.
 *
 * @param size             Item size in pixels (width, height).
 * @param text_baseline_y  Item's text baseline offset from its top, or < 0 if the
 *                         item has no text baseline to align.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_item_size(eli_vec2 size, float text_baseline_y)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return;

    const eli_style *style = &ctx->style;

    /* Grow the line to keep this item's baseline aligned with the row's. */
    float offset_to_baseline = (text_baseline_y >= 0.0f)
        ? eli_max_f(0.0f, win->curr_line_text_baseline_offset - text_baseline_y)
        : 0.0f;

    float line_y1 = win->is_same_line ? win->cursor_pos_prev_line.y : win->cursor_pos.y;
    float line_height = eli_max_f(win->curr_line_size.y,
                                  win->cursor_pos.y - line_y1 + size.y + offset_to_baseline);

    win->cursor_pos_prev_line.x = win->cursor_pos.x + size.x;
    win->cursor_pos_prev_line.y = line_y1;
    win->cursor_pos.x = eli_layout_trunc(win->pos.x + win->indent);
    win->cursor_pos.y = eli_layout_trunc(line_y1 + line_height + style->item_spacing.y);

    win->cursor_max_pos.x = eli_max_f(win->cursor_max_pos.x, win->cursor_pos_prev_line.x);
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y,
                                      win->cursor_pos.y - style->item_spacing.y);

    win->prev_line_size.y = line_height;
    win->curr_line_size.y = 0.0f;
    win->prev_line_text_baseline_offset =
        eli_max_f(win->curr_line_text_baseline_offset, text_baseline_y);
    win->curr_line_text_baseline_offset = 0.0f;

    win->is_same_line = false;
    win->is_set_pos = false;
}

/**
 * Convenience wrapper advancing the cursor by a bounding box's size.
 *
 * @param bb               Item bounding box.
 * @param text_baseline_y  Baseline offset, or < 0 for none.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_item_size_rect(eli_rect bb, float text_baseline_y)
{
    eli_item_size(eli_rect_size(bb), text_baseline_y);
}

/* ---------------------------------------------------------------------------
 * Item add — registration + clip/hover test
 * ------------------------------------------------------------------------- */

/**
 * Register an item with the layout: record its id, rect, and status flags on the
 * context (readable via the last-item accessors), run the clip-rect visibility
 * test, and consume any one-shot next-item width. Call after eli_item_size.
 *
 * @param id           Item id (0 for non-interactive items).
 * @param bb           Item bounding box in screen space.
 * @param extra_flags  Reserved for item-behavior flags (currently unused).
 * @return             true if the item is visible/interactable this frame.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline bool eli_item_add(eli_id id, eli_rect bb, int extra_flags)
{
    (void)extra_flags;
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;

    ctx->last_item_id = id;
    ctx->last_item_rect = bb;
    ctx->last_item_status_flags = ELI_ITEM_STATUS_NONE;

    /* A next-item width applies to a single item and is cleared once consumed. */
    ctx->has_next_item_width = false;

    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return false;

    bool visible = eli_rect_overlaps(bb, win->clip_rect);
    if (visible)
        ctx->last_item_status_flags |= ELI_ITEM_STATUS_VISIBLE;

    if (eli_is_mouse_hovering_rect(eli_rect_min(bb), eli_rect_max(bb), true))
        ctx->last_item_status_flags |= ELI_ITEM_STATUS_HOVERED_RECT;

    return visible;
}

/* ---------------------------------------------------------------------------
 * Last-item accessors (consumed by later item-query phases)
 * ------------------------------------------------------------------------- */

/** @return the id of the most recently added item (0 if none). */
static inline eli_id eli_get_item_id(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->last_item_id : 0u;
}

/** @return the bounding rect of the most recently added item. */
static inline eli_rect eli_get_item_rect(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->last_item_rect : eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
}

/** @return the top-left corner of the most recently added item's rect. */
static inline eli_vec2 eli_get_item_rect_min(void)
{
    return eli_rect_min(eli_get_item_rect());
}

/** @return the bottom-right corner of the most recently added item's rect. */
static inline eli_vec2 eli_get_item_rect_max(void)
{
    return eli_rect_max(eli_get_item_rect());
}

/** @return the size of the most recently added item's rect. */
static inline eli_vec2 eli_get_item_rect_size(void)
{
    return eli_rect_size(eli_get_item_rect());
}

/** @return the eli_item_status_flags of the most recently added item. */
static inline int eli_get_item_status_flags(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->last_item_status_flags : ELI_ITEM_STATUS_NONE;
}

/* ---------------------------------------------------------------------------
 * Content region queries
 * ------------------------------------------------------------------------- */

/** @return absolute (screen-space) bottom-right of the window work area. */
static inline eli_vec2 eli_layout_content_region_max_abs(const eli_window *win)
{
    return eli_make_vec2(win->content_region_rect.x + win->content_region_rect.w,
                         win->content_region_rect.y + win->content_region_rect.h);
}

/**
 * @return space remaining from the cursor to the work-area edge (screen units).
 *         Shrinks as the layout cursor advances. {0,0} without a current window.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_content_region_avail(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    eli_vec2 mx = eli_layout_content_region_max_abs(win);
    return eli_vec2_sub(mx, win->cursor_pos);
}

/**
 * @return work-area bottom-right in window-local coordinates (relative to pos).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_content_region_max(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    eli_vec2 mx = eli_layout_content_region_max_abs(win);
    return eli_vec2_sub(mx, win->pos);
}

/**
 * @return work-area top-left in window-local coordinates (relative to pos).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_window_content_region_min(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    return eli_make_vec2(win->content_region_rect.x - win->pos.x,
                         win->content_region_rect.y - win->pos.y);
}

/**
 * @return work-area bottom-right in window-local coordinates (relative to pos).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_window_content_region_max(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    eli_vec2 mx = eli_layout_content_region_max_abs(win);
    return eli_vec2_sub(mx, win->pos);
}

#endif /* ELI_LAYOUT_ELI_LAYOUT_ITEM_H */
