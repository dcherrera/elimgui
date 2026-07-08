/**
 * @file eli_clip.h
 * @brief Widget-level clip-rect scope: eli_push_clip_rect / eli_pop_clip_rect.
 *        Pushes onto the current window's draw-list clip stack and syncs the
 *        window clip rect so item culling (eli_item_add) honors it.
 *
 * Mirrors Dear ImGui's PushClipRect/PopClipRect: the draw list scissors geometry
 * while the mirrored window clip rect drives eli_item_add's visibility test, so an
 * item entirely outside the active clip becomes not-visible.
 *
 * @status Phase 25 widget clip wrappers in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INTERACTION_ELI_CLIP_H
#define ELI_INTERACTION_ELI_CLIP_H

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../draw/eli_draw_list.h"
#include "../window/eli_window.h"

/**
 * Sync the current window's clip rect from its draw list's active clip rect, so
 * that the item-layout visibility test (eli_item_add) matches what is scissored.
 *
 * @param win  Current window (must be non-NULL).
 */
static inline void eli_clip__sync_window_from_draw_list(eli_window *win)
{
    eli_vec2 mn = eli_draw_list_get_clip_rect_min(&win->draw_list);
    eli_vec2 mx = eli_draw_list_get_clip_rect_max(&win->draw_list);
    win->clip_rect = eli_make_rect(mn.x, mn.y, mx.x - mn.x, mx.y - mn.y);
}

/**
 * Push a clip rectangle onto the current window's draw list and mirror it into
 * the window clip rect. Subsequent geometry is scissored and items lying fully
 * outside the clip are culled (eli_item_add returns false / not visible). Pair
 * with eli_pop_clip_rect. A no-op when there is no current window.
 *
 * @param clip_min                Top-left corner of the clip rect (screen space).
 * @param clip_max                Bottom-right corner of the clip rect.
 * @param intersect_with_current  Intersect with the active clip when true.
 *
 * Thread-safe: no (mutates the current window)
 * Reentrant: no
 */
static inline void eli_push_clip_rect(eli_vec2 clip_min, eli_vec2 clip_max,
                                      bool intersect_with_current)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    eli_draw_list_push_clip_rect(&win->draw_list, clip_min, clip_max, intersect_with_current);
    eli_clip__sync_window_from_draw_list(win);
}

/**
 * Pop the clip rectangle pushed by the most recent eli_push_clip_rect, restoring
 * both the draw list clip and the mirrored window clip rect. A no-op when there
 * is no current window.
 *
 * Thread-safe: no (mutates the current window)
 * Reentrant: no
 */
static inline void eli_pop_clip_rect(void)
{
    eli_window *win = eli_get_current_window();
    if (win == NULL)
        return;
    eli_draw_list_pop_clip_rect(&win->draw_list);
    eli_clip__sync_window_from_draw_list(win);
}

#endif /* ELI_INTERACTION_ELI_CLIP_H */
