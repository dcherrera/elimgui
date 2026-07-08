/**
 * @file eli_window_query.h
 * @brief Window state queries: appearing/collapsed/focused/hovered tests and the
 *        current window's draw list, position, size, width, and height getters.
 *
 * @status Phase 7 window queries in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_QUERY_H
#define ELI_WINDOW_ELI_WINDOW_QUERY_H

#include "eli_window_internal.h"

/**
 * @return true on the first frame the current window becomes active after being
 *         newly created or re-shown (matches Dear ImGui's "appearing").
 */
static inline bool eli_is_window_appearing(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->appearing : false;
}

/** @return true if the current window is collapsed. */
static inline bool eli_is_window_collapsed(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->collapsed : false;
}

/**
 * @return true if the current window is the focused (nav) window, comparing at
 *         the root level so a child reports focused when its root is focused.
 */
static inline bool eli_is_window_focused(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL)
        return false;
    eli_window *win = ctx->current_window;
    eli_window *root = win->root_window ? win->root_window : win;
    eli_window *nav_root = ctx->nav_window
                               ? (ctx->nav_window->root_window ? ctx->nav_window->root_window
                                                               : ctx->nav_window)
                               : NULL;
    return nav_root == root;
}

/** @return true if the mouse is over the current window (root-aware). */
static inline bool eli_is_window_hovered(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL || ctx->current_window == NULL || ctx->hovered_window == NULL)
        return false;
    eli_window *win = ctx->current_window;
    eli_window *root = win->root_window ? win->root_window : win;
    eli_window *hov_root = ctx->hovered_window->root_window ? ctx->hovered_window->root_window
                                                            : ctx->hovered_window;
    return hov_root == root;
}

/** @return the current window's draw list, or NULL if there is no window. */
static inline eli_draw_list *eli_get_window_draw_list(void)
{
    eli_window *win = eli_get_current_window();
    return win ? &win->draw_list : NULL;
}

/** @return the current window's top-left position, or (0,0). */
static inline eli_vec2 eli_get_window_pos(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->pos : eli_make_vec2(0.0f, 0.0f);
}

/** @return the current window's size, or (0,0). */
static inline eli_vec2 eli_get_window_size(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->size : eli_make_vec2(0.0f, 0.0f);
}

/** @return the current window's width, or 0. */
static inline float eli_get_window_width(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->size.x : 0.0f;
}

/** @return the current window's height, or 0. */
static inline float eli_get_window_height(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->size.y : 0.0f;
}

#endif /* ELI_WINDOW_ELI_WINDOW_QUERY_H */
