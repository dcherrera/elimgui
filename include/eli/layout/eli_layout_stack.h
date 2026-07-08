/**
 * @file eli_layout_stack.h
 * @brief Item-width and text-wrap-position stacks: eli_push_item_width /
 *        eli_pop_item_width / eli_set_next_item_width / eli_calc_item_width and
 *        eli_push_text_wrap_pos / eli_pop_text_wrap_pos.
 *
 * Item width governs the pixel width of the next framed widgets: a positive value
 * is used verbatim, 0 selects the window's default width, and a negative value is
 * measured back from the right edge of the work area. The text-wrap stack stores
 * window-local x positions at which text widgets wrap.
 *
 * @status Phase 8 item-width / text-wrap stacks in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_STACK_H
#define ELI_LAYOUT_ELI_LAYOUT_STACK_H

#include "eli_layout_item.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"

/* ---------------------------------------------------------------------------
 * Item width
 * ------------------------------------------------------------------------- */

/**
 * Push a width for subsequent framed widgets, saving the current width to restore
 * with eli_pop_item_width.
 *
 * @param item_width  Width in pixels; 0 selects the window default, < 0 measures
 *                    from the right edge of the work area.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_push_item_width(float item_width)
{
    eli_context *ctx = eli_get_current_context();
    eli_window *win = eli_layout_current_window();
    if (ctx == NULL || win == NULL || ctx->item_width_stack_size >= ELI_ITEM_WIDTH_STACK_MAX)
        return;
    ctx->item_width_stack[ctx->item_width_stack_size++] = win->item_width;
    win->item_width = (item_width == 0.0f) ? win->item_width_default : item_width;
    ctx->has_next_item_width = false;
}

/**
 * Restore the width saved by the matching eli_push_item_width.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_pop_item_width(void)
{
    eli_context *ctx = eli_get_current_context();
    eli_window *win = eli_layout_current_window();
    if (ctx == NULL || win == NULL || ctx->item_width_stack_size <= 0)
        return;
    win->item_width = ctx->item_width_stack[--ctx->item_width_stack_size];
}

/**
 * Set the width of the next single widget only (overrides the pushed width once).
 *
 * @param item_width  Width in pixels (same sign convention as eli_push_item_width).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_next_item_width(float item_width)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    ctx->next_item_width = item_width;
    ctx->has_next_item_width = true;
}

/**
 * Resolve the width the next framed widget should use, applying (in priority) the
 * one-shot next-item width, then the pushed item width, then the window default;
 * negative widths are measured back from the right edge of the work area.
 *
 * @return the resolved width in pixels (>= 1), or 0 without a current window.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline float eli_calc_item_width(void)
{
    eli_context *ctx = eli_get_current_context();
    eli_window *win = eli_layout_current_window();
    if (ctx == NULL || win == NULL)
        return 0.0f;

    float w = ctx->has_next_item_width ? ctx->next_item_width : win->item_width;
    if (w == 0.0f)
        w = win->item_width_default;
    if (w < 0.0f) {
        float region_max_x = eli_layout_content_region_max_abs(win).x;
        w = eli_max_f(1.0f, region_max_x - win->cursor_pos.x + w);
    }
    return eli_layout_trunc(w);
}

/* ---------------------------------------------------------------------------
 * Text wrap position
 * ------------------------------------------------------------------------- */

/**
 * Push a text-wrap x position for subsequent text widgets.
 *
 * @param wrap_local_pos_x  Window-local x at which text wraps; 0 wraps at the work
 *                          area's right edge, < 0 disables wrapping.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_push_text_wrap_pos(float wrap_local_pos_x)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->text_wrap_pos_stack_size >= ELI_TEXT_WRAP_POS_STACK_MAX)
        return;
    ctx->text_wrap_pos_stack[ctx->text_wrap_pos_stack_size++] = wrap_local_pos_x;
}

/**
 * Pop the text-wrap position pushed by the matching eli_push_text_wrap_pos.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_pop_text_wrap_pos(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->text_wrap_pos_stack_size <= 0)
        return;
    ctx->text_wrap_pos_stack_size--;
}

#endif /* ELI_LAYOUT_ELI_LAYOUT_STACK_H */
