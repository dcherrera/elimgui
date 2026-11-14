/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_window_interaction.h
 * @brief Window interaction behaviors: dragging the title bar to move a window,
 *        raising the window under a click, clamping a window so its title bar
 *        stays on screen, mouse-wheel scrolling of the hovered window, and
 *        toggling collapse. These are driven by input state and invoked by the
 *        per-frame update and by eli_begin. Resizing is in eli_window_resize.h.
 *
 * @status Phase 7 window interaction in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_INTERACTION_H
#define ELI_WINDOW_ELI_WINDOW_INTERACTION_H

#include "eli_window_internal.h"
#include "eli_window_resize.h"

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

    /* A press that started a resize or hit the close button is not a move. */
    if (eli_get_active_id() == eli_window_resize_id(win) ||
        eli_window_mouse_over_close_button(ctx, win))
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
 * Raise and focus the window under a left click, anywhere in it (title bar or
 * body). A window with ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS is focused but
 * keeps its z-order (eli_window_focus). Called once per frame, after the hovered
 * window is found, so this frame's hover is unchanged.
 *
 * @param ctx  Context (non-NULL).
 */
static inline void eli_window_focus_on_click(eli_context *ctx)
{
    if (ctx->hovered_window != NULL && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT))
        eli_window_focus(ctx, ctx->hovered_window);
}

/**
 * Keep a top-level window reachable: at least style.display_window_padding of
 * it stays inside the display on each axis, and its top edge (the title bar)
 * never goes above the display. Not called for a position the app set this
 * frame, so app-placed windows (e.g. drawn off-screen on purpose) are left alone.
 *
 * @param ctx  Context (non-NULL).
 * @param win  Top-level window whose pos/size are final for this frame (non-NULL).
 */
static inline void eli_window_clamp_to_display(const eli_context *ctx, eli_window *win)
{
    eli_vec2 display = ctx->io.display_size;
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;
    eli_vec2 pad = ctx->style.display_window_padding;
    float vis_x = eli_min_f(pad.x, win->size.x);
    float vis_y = eli_min_f(pad.y, win->size.y);
    win->pos.x = eli_clamp_f(win->pos.x, vis_x - win->size.x, eli_max_f(vis_x - win->size.x,
                                                                        display.x - vis_x));
    win->pos.y = eli_clamp_f(win->pos.y, 0.0f, eli_max_f(0.0f, display.y - vis_y));
}

#endif /* ELI_WINDOW_ELI_WINDOW_INTERACTION_H */
