/**
 * @file eli_window_child.h
 * @brief Child windows: scrollable/clipped sub-regions nested inside a parent
 *        window. eli_begin_child / eli_begin_child_id create a child sized from
 *        the parent's work area (or an explicit size); eli_end_child closes it.
 *
 * A child is a full eli_window with a synthetic name derived from its id, forced
 * into a decoration-free, non-moving configuration and linked to the parent's
 * root for focus and hover purposes.
 *
 * @status Phase 7 child windows in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_CHILD_H
#define ELI_WINDOW_ELI_WINDOW_CHILD_H

#include "eli_window.h"

/** Base window flags every child window is forced into. */
#define ELI_WINDOW_CHILD_BASE_FLAGS                                                          \
    (ELI_WINDOW_NO_TITLEBAR | ELI_WINDOW_NO_COLLAPSE | ELI_WINDOW_NO_SAVED_SETTINGS |        \
     ELI_WINDOW_NO_MOVE | ELI_WINDOW_NO_RESIZE | ELI_WINDOW_NO_FOCUS_ON_APPEARING)

/**
 * Shared child-window entry used by both public begin_child variants.
 *
 * @param id            Child window id (already hashed against the parent seed).
 * @param size_arg      Requested size; an axis <= 0 uses the parent's remaining
 *                      work area (0) or that minus |axis| (< 0).
 * @param child_flags   Child behavior flags (borders, resize, auto-resize).
 * @param window_flags  Extra window flags OR'd onto the forced child flags.
 * @return              true if the child body should be emitted.
 */
static inline bool eli_begin_child_ex(eli_id id, eli_vec2 size_arg, eli_child_flags child_flags,
                                      eli_window_flags window_flags)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return false;
    eli_window *parent = ctx->current_window;
    if (parent == NULL)
        return false;

    /* Remaining work area from the parent's layout cursor. */
    float avail_w = parent->content_region_rect.x + parent->content_region_rect.w -
                    parent->cursor_pos.x;
    float avail_h = parent->content_region_rect.y + parent->content_region_rect.h -
                    parent->cursor_pos.y;

    eli_vec2 size = size_arg;
    if (size.x == 0.0f) size.x = eli_max_f(avail_w, 4.0f);
    else if (size.x < 0.0f) size.x = eli_max_f(avail_w + size.x, 4.0f);
    if (size.y == 0.0f) size.y = eli_max_f(avail_h, 4.0f);
    else if (size.y < 0.0f) size.y = eli_max_f(avail_h + size.y, 4.0f);

    eli_window_flags flags = ELI_WINDOW_CHILD_BASE_FLAGS | window_flags;
    if (child_flags & (ELI_CHILD_RESIZE_X | ELI_CHILD_RESIZE_Y))
        flags &= ~ELI_WINDOW_NO_RESIZE;

    /* Synthetic, stable name unique to this child id (id is the real key; the
     * name is only stored for debugging). Built without stdio so the header is
     * self-sufficient. */
    char name[16];
    const char *hex = "0123456789abcdef";
    name[0] = 'C'; name[1] = 'h'; name[2] = 'i'; name[3] = 'l'; name[4] = 'd'; name[5] = '_';
    for (int k = 0; k < 8; k++)
        name[6 + k] = hex[(id >> ((7 - k) * 4)) & 0xFu];
    name[14] = '\0';

    eli_set_next_window_pos(parent->cursor_pos, ELI_COND_ALWAYS, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(size, ELI_COND_ALWAYS);

    bool visible = eli_begin_ex(name, id, NULL, flags, parent);
    eli_window *child = eli_get_current_window();
    if (child != NULL)
        child->child_flags = child_flags;
    return visible;
}

/**
 * Begin a child window identified by a string.
 *
 * @param str_id        Child identifier (hashed against the current id seed).
 * @param size          Child size; an axis <= 0 fills the parent's work area.
 * @param child_flags   Child behavior flags.
 * @param window_flags  Extra window flags.
 * @return              true if the child body should be emitted.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_begin_child(const char *str_id, eli_vec2 size, eli_child_flags child_flags,
                                   eli_window_flags window_flags)
{
    if (str_id == NULL)
        return false;
    eli_id id = eli_get_id(str_id);
    return eli_begin_child_ex(id, size, child_flags, window_flags);
}

/**
 * Begin a child window identified by an explicit id.
 *
 * @param id            Child id.
 * @param size          Child size; an axis <= 0 fills the parent's work area.
 * @param child_flags   Child behavior flags.
 * @param window_flags  Extra window flags.
 * @return              true if the child body should be emitted.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline bool eli_begin_child_id(eli_id id, eli_vec2 size, eli_child_flags child_flags,
                                      eli_window_flags window_flags)
{
    return eli_begin_child_ex(id, size, child_flags, window_flags);
}

/**
 * End the current child window opened with eli_begin_child/eli_begin_child_id.
 *
 * After closing the child, it is registered as an item in the PARENT window so
 * the parent's layout cursor advances past it (mirroring Dear ImGui's EndChild
 * -> ItemSize/ItemAdd). Without this, eli_same_line() after a child and two
 * stacked children would overlap, because the parent cursor would never move.
 * The layout item helpers live in a category included after this one, so the
 * cursor-advance is replicated inline here on the parent window.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_end_child(void)
{
    eli_context *ctx = eli_get_current_context();
    eli_window *child = (ctx != NULL) ? ctx->current_window : NULL;
    eli_vec2 child_size = (child != NULL) ? child->size : eli_make_vec2(0.0f, 0.0f);
    eli_vec2 child_pos  = (child != NULL) ? child->pos  : eli_make_vec2(0.0f, 0.0f);
    eli_id   child_id   = (child != NULL) ? child->id   : 0u;

    eli_end();  /* pop the child; restores the parent as current_window */

    eli_window *win = (ctx != NULL) ? ctx->current_window : NULL;
    if (win == NULL || win->skip_items)
        return;

    const eli_style *style = &ctx->style;
    eli_vec2 size = child_size;

    /* Replicated eli_item_size(size): advance the parent cursor by the child. */
    float line_y1 = win->is_same_line ? win->cursor_pos_prev_line.y : win->cursor_pos.y;
    float line_height = eli_max_f(win->curr_line_size.y, win->cursor_pos.y - line_y1 + size.y);

    win->cursor_pos_prev_line.x = win->cursor_pos.x + size.x;
    win->cursor_pos_prev_line.y = line_y1;
    win->cursor_pos.x = (float)(long)(win->pos.x + win->indent);
    win->cursor_pos.y = (float)(long)(line_y1 + line_height + style->item_spacing.y);

    win->cursor_max_pos.x = eli_max_f(win->cursor_max_pos.x, win->cursor_pos_prev_line.x);
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y,
                                      win->cursor_pos.y - style->item_spacing.y);

    win->prev_line_size.y = line_height;
    win->curr_line_size.y = 0.0f;
    win->curr_line_text_baseline_offset = 0.0f;
    win->is_same_line = false;
    win->is_set_pos = false;

    /* Record the child as the last item so is_item_hovered/rect queries work. */
    ctx->last_item_id = child_id;
    ctx->last_item_rect.x = child_pos.x;
    ctx->last_item_rect.y = child_pos.y;
    ctx->last_item_rect.w = size.x;
    ctx->last_item_rect.h = size.y;
    ctx->last_item_status_flags = 0;
}

#endif /* ELI_WINDOW_ELI_WINDOW_CHILD_H */
