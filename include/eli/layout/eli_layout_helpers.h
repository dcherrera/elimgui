/**
 * @file eli_layout_helpers.h
 * @brief Layout-flow helpers built on the item-layout core: eli_separator,
 *        eli_same_line, eli_new_line, eli_spacing, eli_dummy, eli_indent /
 *        eli_unindent, eli_align_text_to_frame_padding, and the group primitives
 *        eli_begin_group / eli_end_group.
 *
 * @status Phase 8 layout helpers in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_HELPERS_H
#define ELI_LAYOUT_ELI_LAYOUT_HELPERS_H

#include "eli_layout_item.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../core/eli_types.h"

/* Draw thickness of a horizontal separator line, in pixels. */
#define ELI_SEPARATOR_THICKNESS 1.0f

/* ---------------------------------------------------------------------------
 * Separator
 * ------------------------------------------------------------------------- */

/**
 * Emit a full-width horizontal separator line at the cursor, advancing the layout
 * by the line thickness.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_separator(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return;

    const eli_style *style = &ctx->style;
    float x1 = win->pos.x + style->window_padding.x;
    float x2 = win->pos.x + win->size.x - style->window_padding.x;
    float y = win->cursor_pos.y;
    eli_rect bb = eli_make_rect(x1, y, x2 - x1, ELI_SEPARATOR_THICKNESS);

    eli_item_size(eli_make_vec2(0.0f, ELI_SEPARATOR_THICKNESS), -1.0f);
    if (!eli_item_add(0u, bb, 0))
        return;

    eli_col32 col = eli_get_color_u32(ELI_COL_SEPARATOR, 1.0f);
    eli_draw_list_add_line(&win->draw_list, eli_make_vec2(x1, y), eli_make_vec2(x2, y), col,
                           ELI_SEPARATOR_THICKNESS);
}

/* ---------------------------------------------------------------------------
 * Same line / new line / spacing / dummy
 * ------------------------------------------------------------------------- */

/**
 * Place the next item on the current line rather than starting a new one.
 *
 * @param offset_from_start_x  If non-zero, absolute window x (from the window's
 *                             left content origin) for the next item; if zero, the
 *                             next item follows the previous one.
 * @param spacing              Horizontal gap in pixels; < 0 uses the style default
 *                             (item spacing x for the follow case, 0 otherwise).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_same_line(float offset_from_start_x, float spacing)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return;

    const eli_style *style = &ctx->style;
    if (offset_from_start_x != 0.0f) {
        float s = (spacing < 0.0f) ? 0.0f : spacing;
        win->cursor_pos.x = win->pos.x - win->scroll.x + offset_from_start_x + s + win->group_offset;
        win->cursor_pos.y = win->cursor_pos_prev_line.y;
    } else {
        float s = (spacing < 0.0f) ? style->item_spacing.x : spacing;
        win->cursor_pos.x = win->cursor_pos_prev_line.x + s;
        win->cursor_pos.y = win->cursor_pos_prev_line.y;
    }
    win->curr_line_size = win->prev_line_size;
    win->curr_line_text_baseline_offset = win->prev_line_text_baseline_offset;
    win->is_same_line = true;
}

/**
 * Terminate the current line and move to the next, preserving the current line's
 * height when it holds short items (else advancing by one text line).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_new_line(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return;

    win->is_same_line = false;
    if (win->curr_line_size.y > 0.0f)
        eli_item_size(eli_make_vec2(0.0f, 0.0f), -1.0f);
    else
        eli_item_size(eli_make_vec2(0.0f, ctx->font_size), -1.0f);
}

/**
 * Advance the cursor by one empty item (adds a row of vertical item spacing).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_spacing(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL || win->skip_items)
        return;
    eli_item_size(eli_make_vec2(0.0f, 0.0f), -1.0f);
}

/**
 * Add an empty, non-interactive item of the given size, advancing the layout.
 *
 * @param size  Item size in pixels.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_dummy(eli_vec2 size)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL || win->skip_items)
        return;
    eli_rect bb = eli_make_rect(win->cursor_pos.x, win->cursor_pos.y, size.x, size.y);
    eli_item_size(size, -1.0f);
    eli_item_add(0u, bb, 0);
}

/* ---------------------------------------------------------------------------
 * Indent / unindent / text alignment
 * ------------------------------------------------------------------------- */

/**
 * Increase the left indent of subsequent lines.
 *
 * @param indent_w  Indent in pixels; 0 uses the style indent spacing.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_indent(float indent_w)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL)
        return;
    win->indent += (indent_w != 0.0f) ? indent_w : ctx->style.indent_spacing;
    win->cursor_pos.x = win->pos.x + win->indent;
}

/**
 * Decrease the left indent of subsequent lines.
 *
 * @param indent_w  Indent in pixels; 0 uses the style indent spacing.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_unindent(float indent_w)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL)
        return;
    win->indent -= (indent_w != 0.0f) ? indent_w : ctx->style.indent_spacing;
    win->cursor_pos.x = win->pos.x + win->indent;
}

/**
 * Vertically align the following text to match a framed widget's text baseline, so
 * text placed with eli_same_line next to a framed widget lines up.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_align_text_to_frame_padding(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || win->skip_items)
        return;
    const eli_style *style = &ctx->style;
    win->curr_line_size.y = eli_max_f(win->curr_line_size.y,
                                      ctx->font_size + style->frame_padding.y * 2.0f);
    win->curr_line_text_baseline_offset =
        eli_max_f(win->curr_line_text_baseline_offset, style->frame_padding.y);
}

/* ---------------------------------------------------------------------------
 * Groups
 * ------------------------------------------------------------------------- */

/**
 * Begin a layout group: subsequent items are measured together so the group can be
 * treated as one item (its bounding box) by the following eli_end_group.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_begin_group(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL || ctx->group_stack_size >= ELI_GROUP_STACK_MAX)
        return;

    eli_group_data *g = &ctx->group_stack[ctx->group_stack_size++];
    g->backup_cursor_pos = win->cursor_pos;
    g->backup_cursor_max_pos = win->cursor_max_pos;
    g->backup_curr_line_size = win->curr_line_size;
    g->backup_curr_line_text_baseline_offset = win->curr_line_text_baseline_offset;
    g->backup_indent = win->indent;
    g->backup_group_offset = win->group_offset;

    win->group_offset = win->cursor_pos.x - win->pos.x;
    win->indent = win->group_offset;
    win->cursor_max_pos = win->cursor_pos;
    win->curr_line_size = eli_make_vec2(0.0f, 0.0f);
}

/**
 * End the group opened by eli_begin_group: restore the pre-group layout state and
 * register the enclosing bounding box as the last item (its size equals the extent
 * of the enclosed items). The group's rect is readable via the last-item accessors.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_end_group(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->group_stack_size <= 0)
        return;
    eli_window *win = ctx->current_window;
    if (win == NULL)
        return;

    eli_group_data *g = &ctx->group_stack[--ctx->group_stack_size];

    eli_vec2 group_min = g->backup_cursor_pos;
    eli_vec2 group_max = eli_make_vec2(eli_max_f(win->cursor_max_pos.x, group_min.x),
                                       eli_max_f(win->cursor_max_pos.y, group_min.y));
    eli_rect group_bb = eli_make_rect(group_min.x, group_min.y,
                                      group_max.x - group_min.x, group_max.y - group_min.y);

    win->cursor_pos = g->backup_cursor_pos;
    win->cursor_max_pos = eli_make_vec2(eli_max_f(g->backup_cursor_max_pos.x, win->cursor_max_pos.x),
                                        eli_max_f(g->backup_cursor_max_pos.y, win->cursor_max_pos.y));
    win->indent = g->backup_indent;
    win->group_offset = g->backup_group_offset;
    win->curr_line_size = g->backup_curr_line_size;
    win->curr_line_text_baseline_offset = g->backup_curr_line_text_baseline_offset;
    win->is_same_line = false;

    eli_item_size(eli_rect_size(group_bb), -1.0f);
    eli_item_add(0u, group_bb, 0);
}

#endif /* ELI_LAYOUT_ELI_LAYOUT_HELPERS_H */
