/**
 * @file eli_window_settings.h
 * @brief Window manipulation API: the set_next_window_* family that seeds the
 *        next eli_begin, the set_window_* family that mutates the current window,
 *        and per-window font scaling.
 *
 * @status Phase 7 window settings in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_SETTINGS_H
#define ELI_WINDOW_ELI_WINDOW_SETTINGS_H

#include "eli_window_internal.h"

/* ---------------------------------------------------------------------------
 * Next-window settings (consumed by the following eli_begin)
 * ------------------------------------------------------------------------- */

/**
 * Set the position of the next window.
 *
 * @param pos    Top-left position in screen space.
 * @param cond   Condition gating the write (0 == always).
 * @param pivot  Pivot within the window (0,0 = top-left, 0.5,0.5 = center).
 */
static inline void eli_set_next_window_pos(eli_vec2 pos, eli_cond cond, eli_vec2 pivot)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_pos = true;
    d->pos_val = pos;
    d->pos_pivot = pivot;
    d->pos_cond = cond;
}

/**
 * Set the size of the next window. A zero or negative axis means auto-size.
 *
 * @param size  Desired size in pixels.
 * @param cond  Condition gating the write (0 == always).
 */
static inline void eli_set_next_window_size(eli_vec2 size, eli_cond cond)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_size = true;
    d->size_val = size;
    d->size_cond = cond;
}

/**
 * Constrain the next window's size to a [min, max] box. Use a negative axis in
 * max to leave that axis unconstrained.
 *
 * @param size_min  Minimum size.
 * @param size_max  Maximum size (negative axis = unbounded).
 */
static inline void eli_set_next_window_size_constraints(eli_vec2 size_min, eli_vec2 size_max)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_size_constraint = true;
    d->size_constraint_min = size_min;
    d->size_constraint_max = size_max;
}

/**
 * Set the explicit content size of the next window (drives the scroll range).
 *
 * @param size  Content size in pixels.
 */
static inline void eli_set_next_window_content_size(eli_vec2 size)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_content_size = true;
    d->content_size_val = size;
}

/**
 * Set the collapsed state of the next window.
 *
 * @param collapsed  true to collapse.
 * @param cond       Condition gating the write (0 == always).
 */
static inline void eli_set_next_window_collapsed(bool collapsed, eli_cond cond)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_collapsed = true;
    d->collapsed_val = collapsed;
    d->collapsed_cond = cond;
}

/** Focus the next window (bring it to front). */
static inline void eli_set_next_window_focus(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d != NULL)
        d->has_focus = true;
}

/**
 * Set the next window's scroll offsets. A negative axis leaves it unchanged.
 *
 * @param scroll  Desired scroll offsets.
 */
static inline void eli_set_next_window_scroll(eli_vec2 scroll)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d != NULL) {
        d->has_scroll = true;
        d->scroll_val = scroll;
    }
}

/**
 * Set the background alpha of the next window (0 transparent .. 1 opaque).
 *
 * @param alpha  Background alpha multiplier.
 */
static inline void eli_set_next_window_bg_alpha(float alpha)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d != NULL) {
        d->has_bg_alpha = true;
        d->bg_alpha_val = alpha;
    }
}

/* ---------------------------------------------------------------------------
 * Current-window mutation
 * ------------------------------------------------------------------------- */

/** Set the current window's position immediately. */
static inline void eli_set_window_pos(eli_vec2 pos, eli_cond cond)
{
    (void)cond;
    eli_window *win = eli_get_current_window();
    if (win != NULL)
        win->pos = pos;
}

/** Set the current window's size immediately (updates size_full). */
static inline void eli_set_window_size(eli_vec2 size, eli_cond cond)
{
    (void)cond;
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    if (size.x > 0.0f)
        win->size_full.x = size.x;
    if (size.y > 0.0f)
        win->size_full.y = size.y;
}

/** Set the current window's collapsed state immediately. */
static inline void eli_set_window_collapsed(bool collapsed, eli_cond cond)
{
    (void)cond;
    eli_window *win = eli_get_current_window();
    if (win != NULL)
        win->collapsed = collapsed;
}

/** Focus (bring to front) the current window. */
static inline void eli_set_window_focus(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx != NULL && ctx->current_window != NULL)
        eli_window_focus(ctx, ctx->current_window);
}

/**
 * Set the per-window font scale multiplier applied to the current window's text.
 *
 * @param scale  Font scale (1.0 = default).
 */
static inline void eli_set_window_font_scale(float scale)
{
    eli_window *win = eli_get_current_window();
    if (win != NULL)
        win->font_window_scale = scale;
}

#endif /* ELI_WINDOW_ELI_WINDOW_SETTINGS_H */
