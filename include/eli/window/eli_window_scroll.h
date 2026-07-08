/**
 * @file eli_window_scroll.h
 * @brief Window scrolling: current-window scroll get/set on both axes, scroll
 *        range queries, position-based and cursor-based scroll targeting, and
 *        rendering of the vertical and horizontal scrollbars.
 *
 * Scroll setters apply immediately, clamped against the window's current scroll
 * range (computed at eli_begin), so a get_scroll_* right after a set_scroll_*
 * reads the clamped result within the same frame.
 *
 * @status Phase 7 window scrolling in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_SCROLL_H
#define ELI_WINDOW_ELI_WINDOW_SCROLL_H

#include "eli_window_internal.h"

/* ---------------------------------------------------------------------------
 * Scroll get/set (current window)
 * ------------------------------------------------------------------------- */

/** @return the current window's horizontal scroll offset (0 if no window). */
static inline float eli_get_scroll_x(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->scroll.x : 0.0f;
}

/** @return the current window's vertical scroll offset (0 if no window). */
static inline float eli_get_scroll_y(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->scroll.y : 0.0f;
}

/** @return the maximum horizontal scroll offset (0 if no window). */
static inline float eli_get_scroll_max_x(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->scroll_max.x : 0.0f;
}

/** @return the maximum vertical scroll offset (0 if no window). */
static inline float eli_get_scroll_max_y(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->scroll_max.y : 0.0f;
}

/**
 * Set the current window's horizontal scroll (clamped to [0, scroll_max.x]).
 *
 * @param scroll_x  Desired horizontal offset in pixels.
 */
static inline void eli_set_scroll_x(float scroll_x)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    win->scroll.x = eli_clamp_f(scroll_x, 0.0f, win->scroll_max.x);
}

/**
 * Set the current window's vertical scroll (clamped to [0, scroll_max.y]).
 *
 * @param scroll_y  Desired vertical offset in pixels.
 */
static inline void eli_set_scroll_y(float scroll_y)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    win->scroll.y = eli_clamp_f(scroll_y, 0.0f, win->scroll_max.y);
}

/* ---------------------------------------------------------------------------
 * Position/cursor-based scroll targeting (current window)
 * ------------------------------------------------------------------------- */

/**
 * Scroll so a window-relative X position lands at a horizontal ratio of the work
 * area (0 = left edge, 0.5 = center, 1 = right edge).
 *
 * @param local_x         X position relative to the window's top-left corner.
 * @param center_x_ratio  Target ratio within the visible work area.
 */
static inline void eli_set_scroll_from_pos_x(float local_x, float center_x_ratio)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    float deco_left = win->content_region_rect.x - win->pos.x;
    float content_x = local_x - deco_left + win->scroll.x;
    float target = content_x - center_x_ratio * win->content_region_rect.w;
    win->scroll.x = eli_clamp_f(target, 0.0f, win->scroll_max.x);
}

/**
 * Scroll so a window-relative Y position lands at a vertical ratio of the work
 * area (0 = top edge, 0.5 = center, 1 = bottom edge).
 *
 * @param local_y         Y position relative to the window's top-left corner.
 * @param center_y_ratio  Target ratio within the visible work area.
 */
static inline void eli_set_scroll_from_pos_y(float local_y, float center_y_ratio)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    float deco_top = win->content_region_rect.y - win->pos.y;
    float content_y = local_y - deco_top + win->scroll.y;
    float target = content_y - center_y_ratio * win->content_region_rect.h;
    win->scroll.y = eli_clamp_f(target, 0.0f, win->scroll_max.y);
}

/**
 * Scroll so the current layout cursor's X is at the given ratio of the work
 * area. With no widgets emitted the cursor sits at the work-area origin.
 *
 * @param center_x_ratio  Target ratio (default 0.5 centers).
 */
static inline void eli_set_scroll_here_x(float center_x_ratio)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    float local_x = win->cursor_pos.x - win->pos.x;
    eli_set_scroll_from_pos_x(local_x, center_x_ratio);
}

/**
 * Scroll so the current layout cursor's Y is at the given ratio of the work
 * area. With no widgets emitted the cursor sits at the work-area origin.
 *
 * @param center_y_ratio  Target ratio (default 0.5 centers).
 */
static inline void eli_set_scroll_here_y(float center_y_ratio)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    float local_y = win->cursor_pos.y - win->pos.y;
    eli_set_scroll_from_pos_y(local_y, center_y_ratio);
}

/* ---------------------------------------------------------------------------
 * Scrollbar rendering
 * ------------------------------------------------------------------------- */

/**
 * Render a window's active scrollbars into its draw list, sizing the grab to the
 * visible/content ratio and positioning it by the current scroll offset. A
 * no-op for axes without an active scrollbar.
 *
 * @param win    Window to draw scrollbars for (non-NULL).
 * @param style  Active style (non-NULL).
 */
static inline void eli_window_render_scrollbars(eli_window *win, const eli_style *style)
{
    eli_draw_list *dl = &win->draw_list;
    float sz = style->scrollbar_size;
    float rounding = style->scrollbar_rounding;
    eli_col32 bg = eli_get_color_u32(ELI_COL_SCROLLBAR_BG, 1.0f);
    eli_col32 grab = eli_get_color_u32(ELI_COL_SCROLLBAR_GRAB, 1.0f);

    if (win->has_scrollbar_y) {
        float track_x0 = win->inner_rect.x + win->inner_rect.w - sz;
        float track_y0 = win->inner_rect.y;
        float track_y1 = win->inner_rect.y + win->inner_rect.h - (win->has_scrollbar_x ? sz : 0.0f);
        float track_h = eli_max_f(1.0f, track_y1 - track_y0);
        float content_h = win->content_size.y > 0.0f ? win->content_size.y : track_h;
        float visible_ratio = eli_clamp_f(win->content_region_rect.h / content_h, 0.0f, 1.0f);
        float grab_h = eli_max_f(style->grab_min_size, track_h * visible_ratio);
        float scroll_ratio = win->scroll_max.y > 0.0f ? win->scroll.y / win->scroll_max.y : 0.0f;
        float grab_y0 = track_y0 + scroll_ratio * (track_h - grab_h);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(track_x0, track_y0),
                                      eli_make_vec2(track_x0 + sz, track_y1), bg, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(track_x0, grab_y0),
                                      eli_make_vec2(track_x0 + sz, grab_y0 + grab_h), grab, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
    }

    if (win->has_scrollbar_x) {
        float track_y0 = win->inner_rect.y + win->inner_rect.h - sz;
        float track_x0 = win->inner_rect.x;
        float track_x1 = win->inner_rect.x + win->inner_rect.w - (win->has_scrollbar_y ? sz : 0.0f);
        float track_w = eli_max_f(1.0f, track_x1 - track_x0);
        float content_w = win->content_size.x > 0.0f ? win->content_size.x : track_w;
        float visible_ratio = eli_clamp_f(win->content_region_rect.w / content_w, 0.0f, 1.0f);
        float grab_w = eli_max_f(style->grab_min_size, track_w * visible_ratio);
        float scroll_ratio = win->scroll_max.x > 0.0f ? win->scroll.x / win->scroll_max.x : 0.0f;
        float grab_x0 = track_x0 + scroll_ratio * (track_w - grab_w);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(track_x0, track_y0),
                                      eli_make_vec2(track_x1, track_y0 + sz), bg, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(grab_x0, track_y0),
                                      eli_make_vec2(grab_x0 + grab_w, track_y0 + sz), grab, rounding,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
    }
}

#endif /* ELI_WINDOW_ELI_WINDOW_SCROLL_H */
