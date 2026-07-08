/**
 * @file eli_window_interaction.h
 * @brief Window interaction behaviors: dragging the title bar to move a window,
 *        dragging the bottom-right grip to resize, mouse-wheel scrolling of the
 *        hovered window, and toggling collapse. These are driven by input state
 *        and invoked by the per-frame update and by eli_begin.
 *
 * @status Phase 7 window interaction in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_INTERACTION_H
#define ELI_WINDOW_ELI_WINDOW_INTERACTION_H

#include "eli_window_internal.h"

/* Side length (px) of the resize grip hit-box at the window's bottom-right. */
#define ELI_WINDOW_RESIZE_GRIP_SIZE 18.0f

/** @return the per-wheel-notch scroll step in pixels for a window. */
static inline float eli_window_wheel_step(const eli_context *ctx, const eli_window *win)
{
    float fs = eli_window_effective_font_size(ctx);
    float step = 5.0f * fs;
    float cap = win->content_region_rect.h * 0.67f;
    if (cap > 0.0f && step > cap)
        step = cap;
    return step;
}

/**
 * Continue or finish a title-bar drag begun in eli_begin. While the left button
 * is held and a window is being moved, its position tracks the mouse delta. On
 * release the move ends. Called once at the start of each frame.
 *
 * @param ctx  Context (non-NULL).
 */
static inline void eli_window_update_moving(eli_context *ctx)
{
    if (ctx->moving_window == NULL)
        return;

    const eli_io *io = &ctx->io;
    if (io->mouse_down[ELI_MOUSE_BUTTON_LEFT]) {
        eli_window *win = ctx->moving_window;
        win->pos = eli_vec2_add(win->pos, io->mouse_delta);
    } else {
        ctx->moving_window = NULL;
        eli_clear_active_id();
    }
}

/**
 * Apply mouse-wheel scrolling to the hovered window. Vertical wheel scrolls Y
 * (or X when shift is held and there is no Y range); horizontal wheel scrolls X.
 * A no-op when the hovered window forbids wheel scrolling. Called once per frame
 * after the hovered window is determined.
 *
 * @param ctx  Context (non-NULL).
 */
/** Nearest ancestor of `win` (including itself) that can wheel-scroll on the
 *  given axis (axis_y != 0 for the Y axis). NULL if none. A hovered child with
 *  no scroll range on that axis hands the wheel up to its parent pane. */
static inline eli_window *eli_window_wheel_target(eli_window *win, int axis_y)
{
    while (win != NULL) {
        if (!(win->flags & ELI_WINDOW_NO_SCROLL_WITH_MOUSE)) {
            float m = axis_y ? win->scroll_max.y : win->scroll_max.x;
            if (m > 0.0f)
                return win;
        }
        win = win->parent_window;
    }
    return NULL;
}

static inline void eli_window_update_wheel(eli_context *ctx)
{
    eli_window *hov = ctx->hovered_window;
    if (hov == NULL)
        return;

    const eli_io *io = &ctx->io;

    if (io->mouse_wheel != 0.0f) {
        int axis_y = !(io->key_shift);   /* shift = scroll horizontally */
        eli_window *win = eli_window_wheel_target(hov, axis_y);
        if (win == NULL)                 /* fall back to the other axis */
            win = eli_window_wheel_target(hov, !axis_y);
        if (win != NULL) {
            float step = eli_window_wheel_step(ctx, win);
            if (win->scroll_max.y > 0.0f && !(io->key_shift && win->scroll_max.x > 0.0f))
                win->scroll.y -= io->mouse_wheel * step;
            else
                win->scroll.x -= io->mouse_wheel * step;
            eli_window_clamp_scroll(win);
        }
    }
    if (io->mouse_wheel_h != 0.0f) {
        eli_window *win = eli_window_wheel_target(hov, 0);
        if (win != NULL) {
            float step = eli_window_wheel_step(ctx, win);
            win->scroll.x -= io->mouse_wheel_h * step;
            eli_window_clamp_scroll(win);
        }
    }
}

/**
 * Handle a title-bar collapse toggle: a double-click on the title bar toggles
 * the collapsed state (unless ELI_WINDOW_NO_COLLAPSE). Called from eli_begin
 * while the window is hovered.
 *
 * @param ctx  Context (non-NULL).
 * @param win  Window whose title bar is under test (non-NULL).
 * @return     true if the collapsed state was toggled this call.
 */
static inline bool eli_window_handle_title_collapse(eli_context *ctx, eli_window *win)
{
    if (win->flags & (ELI_WINDOW_NO_COLLAPSE | ELI_WINDOW_NO_TITLEBAR))
        return false;
    if (!eli_window_has_title_bar(win))
        return false;

    eli_vec2 tl = eli_rect_min(win->title_bar_rect);
    eli_vec2 br = eli_rect_max(win->title_bar_rect);
    if (ctx->hovered_window == win && eli_is_mouse_double_clicked(ELI_MOUSE_BUTTON_LEFT) &&
        eli_is_mouse_hovering_rect(tl, br, false)) {
        win->collapsed = !win->collapsed;
        return true;
    }
    return false;
}

/**
 * Begin a title-bar move if the user presses the left button on the title bar
 * of the hovered window (and moving is permitted). Sets the moving window and
 * focuses it. Called from eli_begin.
 *
 * @param ctx  Context (non-NULL).
 * @param win  Window under test (non-NULL).
 */
static inline void eli_window_handle_title_move(eli_context *ctx, eli_window *win)
{
    if (win->flags & ELI_WINDOW_NO_MOVE)
        return;
    if (ctx->hovered_window != win)
        return;
    if (!eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        return;

    eli_vec2 tl = eli_rect_min(win->title_bar_rect);
    eli_vec2 br = eli_rect_max(win->title_bar_rect);
    if (!eli_window_has_title_bar(win))
        return;
    if (eli_is_mouse_hovering_rect(tl, br, false)) {
        ctx->moving_window = win->root_window ? win->root_window : win;
        eli_window_focus(ctx, win);
    }
}

/**
 * Resize a window by dragging its bottom-right grip. Enabled only for movable,
 * non-collapsed top-level windows without ELI_WINDOW_NO_RESIZE. Adjusts
 * size_full by the mouse delta while the grip is held. Called from eli_begin.
 *
 * @param ctx  Context (non-NULL).
 * @param win  Window under test (non-NULL).
 * @param style Active style (non-NULL).
 */
static inline void eli_window_handle_resize(eli_context *ctx, eli_window *win,
                                            const eli_style *style)
{
    if (win->flags & (ELI_WINDOW_NO_RESIZE | ELI_WINDOW_AUTO_RESIZE))
        return;
    if (win->collapsed)
        return;

    const eli_io *io = &ctx->io;
    float g = ELI_WINDOW_RESIZE_GRIP_SIZE;
    eli_vec2 br = eli_rect_max(win->outer_rect);
    eli_vec2 grip_min = eli_make_vec2(br.x - g, br.y - g);

    bool over_grip = eli_is_mouse_hovering_rect(grip_min, br, false) && ctx->hovered_window == win;
    eli_id resize_id = win->id ^ 0x52455A31u; /* "REZ1" salt for the grip id */

    if (over_grip && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        eli_set_active_id(resize_id);

    if (eli_get_active_id() == resize_id) {
        if (io->mouse_down[ELI_MOUSE_BUTTON_LEFT]) {
            win->size_full = eli_vec2_add(win->size_full, io->mouse_delta);
            win->size_full.x = eli_max_f(win->size_full.x, style->window_min_size.x);
            win->size_full.y = eli_max_f(win->size_full.y, style->window_min_size.y);
        } else {
            eli_clear_active_id();
        }
    }
}

#endif /* ELI_WINDOW_ELI_WINDOW_INTERACTION_H */
