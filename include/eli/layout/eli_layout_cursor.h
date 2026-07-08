/**
 * @file eli_layout_cursor.h
 * @brief Layout cursor queries and moves: window-relative get/set cursor position
 *        (eli_get_cursor_pos / eli_set_cursor_pos and axis variants), the cursor
 *        start position, and screen-space get/set conversions.
 *
 * Window-local cursor coordinates are measured relative to the window origin and
 * include scroll (matching Dear ImGui): local = screen - pos + scroll. Setting the
 * cursor grows the window's content-max extent so manual positioning still expands
 * the scroll range.
 *
 * @status Phase 8 cursor API in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_LAYOUT_ELI_LAYOUT_CURSOR_H
#define ELI_LAYOUT_ELI_LAYOUT_CURSOR_H

#include "eli_layout_item.h"

#include "../core/eli_platform.h"
#include "../core/eli_context.h"
#include "../core/eli_types.h"

/* ---------------------------------------------------------------------------
 * Get cursor position (window-local)
 * ------------------------------------------------------------------------- */

/**
 * @return the cursor position in window-local coordinates (relative to the window
 *         origin, plus scroll), or {0,0} without a current window.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_cursor_pos(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    return eli_vec2_add(eli_vec2_sub(win->cursor_pos, win->pos), win->scroll);
}

/** @return the window-local cursor x. */
static inline float eli_get_cursor_pos_x(void)
{
    eli_window *win = eli_layout_current_window();
    return win ? (win->cursor_pos.x - win->pos.x + win->scroll.x) : 0.0f;
}

/** @return the window-local cursor y. */
static inline float eli_get_cursor_pos_y(void)
{
    eli_window *win = eli_layout_current_window();
    return win ? (win->cursor_pos.y - win->pos.y + win->scroll.y) : 0.0f;
}

/* ---------------------------------------------------------------------------
 * Set cursor position (window-local)
 * ------------------------------------------------------------------------- */

/**
 * Move the cursor to a window-local position and grow the content-max extent.
 *
 * @param local_pos  Position relative to the window origin (including scroll).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_cursor_pos(eli_vec2 local_pos)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return;
    win->cursor_pos = eli_vec2_add(eli_vec2_sub(win->pos, win->scroll), local_pos);
    win->cursor_max_pos.x = eli_max_f(win->cursor_max_pos.x, win->cursor_pos.x);
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y, win->cursor_pos.y);
    win->is_set_pos = true;
}

/** Move only the cursor x (window-local); grows content-max. */
static inline void eli_set_cursor_pos_x(float local_x)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return;
    win->cursor_pos.x = win->pos.x - win->scroll.x + local_x;
    win->cursor_max_pos.x = eli_max_f(win->cursor_max_pos.x, win->cursor_pos.x);
    win->is_set_pos = true;
}

/** Move only the cursor y (window-local); grows content-max. */
static inline void eli_set_cursor_pos_y(float local_y)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return;
    win->cursor_pos.y = win->pos.y - win->scroll.y + local_y;
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y, win->cursor_pos.y);
    win->is_set_pos = true;
}

/* ---------------------------------------------------------------------------
 * Start position + screen-space conversions
 * ------------------------------------------------------------------------- */

/**
 * @return the initial cursor position of the window body, window-local (this is
 *         the origin against which content size is measured).
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_cursor_start_pos(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    return eli_vec2_sub(win->cursor_start_pos, win->pos);
}

/**
 * @return the cursor position in absolute screen coordinates.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_get_cursor_screen_pos(void)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return eli_make_vec2(0.0f, 0.0f);
    return win->cursor_pos;
}

/**
 * Move the cursor to an absolute screen position and grow the content-max extent.
 *
 * @param pos  Absolute screen position.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_set_cursor_screen_pos(eli_vec2 pos)
{
    eli_window *win = eli_layout_current_window();
    if (win == NULL)
        return;
    win->cursor_pos = pos;
    win->cursor_max_pos.x = eli_max_f(win->cursor_max_pos.x, win->cursor_pos.x);
    win->cursor_max_pos.y = eli_max_f(win->cursor_max_pos.y, win->cursor_pos.y);
    win->is_set_pos = true;
}

#endif /* ELI_LAYOUT_ELI_LAYOUT_CURSOR_H */
