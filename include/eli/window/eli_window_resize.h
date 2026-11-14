/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_window_resize.h
 * @brief Manual window resize: a 4 px band along every edge, corners (the band
 *        within a grip's length of a corner) and the drawn bottom-right grip.
 *        Sets the matching resize cursor while hovered or dragged, and reports
 *        the grip's colour slot for eli_window_render_decorations.
 *
 * The drag tracks the pointer absolutely (the grab offset is kept from the
 * press), honours the min/max size, and keeps the opposite edge fixed when the
 * left or top edge hits the minimum.
 *
 * @status In use by eli_begin for top-level, non-docked windows.
 * @issues None
 * @todo None
 */
#ifndef ELI_WINDOW_ELI_WINDOW_RESIZE_H
#define ELI_WINDOW_ELI_WINDOW_RESIZE_H

#include "eli_window_internal.h"

/* Minimum side (px) of the bottom-right grip and of each corner zone; grows with
 * the font (font size * 1.35, as Dear ImGui). */
#define ELI_WINDOW_RESIZE_GRIP_SIZE 18.0f

/* Width (px) of the resize band inside each window edge. */
#define ELI_WINDOW_RESIZE_BORDER_BAND 4.0f

/* Edge bits held by a resize (eli_window.resize_edges). */
enum {
    ELI_WINDOW_EDGE_LEFT   = 1 << 0,
    ELI_WINDOW_EDGE_RIGHT  = 1 << 1,
    ELI_WINDOW_EDGE_TOP    = 1 << 2,
    ELI_WINDOW_EDGE_BOTTOM = 1 << 3
};

/** @return the grip / corner-zone side for the current font. */
static inline float eli_window_resize_grip_size(const eli_context *ctx)
{
    return eli_max_f(ELI_WINDOW_RESIZE_GRIP_SIZE, eli_window_effective_font_size(ctx) * 1.35f);
}

/** @return the active-id a window's resize drag holds. */
static inline eli_id eli_window_resize_id(const eli_window *win)
{
    return win->id ^ 0x52455A31u; /* "REZ1" salt */
}

/** @return true if the window can be resized by the user this frame. */
static inline bool eli_window_is_resizable(const eli_window *win)
{
    if (win->flags & (ELI_WINDOW_NO_RESIZE | ELI_WINDOW_AUTO_RESIZE))
        return false;
    return win->parent_window == NULL && win->dock_node == NULL && !win->collapsed;
}

/**
 * Which edges a point would resize.
 *
 * @param r       Window rect.
 * @param p       Point (mouse).
 * @param band    Edge band width.
 * @param corner  Corner zone length along each band; also the grip side.
 * @return        ELI_WINDOW_EDGE_* bits, 0 when the point is not on an edge.
 */
static inline int eli_window_resize_edges_at(eli_rect r, eli_vec2 p, float band, float corner)
{
    if (!eli_rect_contains(r, p))
        return 0;
    float dl = p.x - r.x, dr = r.x + r.w - p.x, dt = p.y - r.y, db = r.y + r.h - p.y;
    int e = 0;
    if (dl < band) e |= ELI_WINDOW_EDGE_LEFT;
    if (dr < band) e |= ELI_WINDOW_EDGE_RIGHT;
    if (dt < band) e |= ELI_WINDOW_EDGE_TOP;
    if (db < band) e |= ELI_WINDOW_EDGE_BOTTOM;

    /* Near a corner a band takes the corner too, so corners are easy to grab. */
    if (e & (ELI_WINDOW_EDGE_TOP | ELI_WINDOW_EDGE_BOTTOM)) {
        if (dl < corner) e |= ELI_WINDOW_EDGE_LEFT;
        if (dr < corner) e |= ELI_WINDOW_EDGE_RIGHT;
    }
    if (e & (ELI_WINDOW_EDGE_LEFT | ELI_WINDOW_EDGE_RIGHT)) {
        if (dt < corner) e |= ELI_WINDOW_EDGE_TOP;
        if (db < corner) e |= ELI_WINDOW_EDGE_BOTTOM;
    }
    if (dr < corner && db < corner) /* the drawn grip */
        e |= ELI_WINDOW_EDGE_RIGHT | ELI_WINDOW_EDGE_BOTTOM;
    return e;
}

/** @return the resize cursor for a set of edges (non-zero). */
static inline eli_mouse_cursor eli_window_resize_cursor(int edges)
{
    bool horizontal = (edges & (ELI_WINDOW_EDGE_LEFT | ELI_WINDOW_EDGE_RIGHT)) != 0;
    bool vertical = (edges & (ELI_WINDOW_EDGE_TOP | ELI_WINDOW_EDGE_BOTTOM)) != 0;
    if (horizontal && vertical) {
        bool left = (edges & ELI_WINDOW_EDGE_LEFT) != 0, top = (edges & ELI_WINDOW_EDGE_TOP) != 0;
        return (left == top) ? ELI_MOUSE_CURSOR_RESIZE_NWSE : ELI_MOUSE_CURSOR_RESIZE_NESW;
    }
    return horizontal ? ELI_MOUSE_CURSOR_RESIZE_EW : ELI_MOUSE_CURSOR_RESIZE_NS;
}

/** Clamp one extent to [mn, mx] (mx < 0 = unbounded). */
static inline float eli_window_clamp_extent(float v, float mn, float mx)
{
    v = eli_max_f(v, mn);
    return (mx >= 0.0f) ? eli_min_f(v, eli_max_f(mx, mn)) : v;
}

/**
 * Move the held edges of a window to the pointer (less the grab offset),
 * keeping the opposite edges fixed and the size within [size_min, size_max].
 */
static inline void eli_window_apply_resize_drag(eli_window *win, eli_vec2 mouse, eli_vec2 size_min,
                                                eli_vec2 size_max)
{
    int e = win->resize_edges;
    eli_vec2 target = eli_vec2_sub(mouse, win->resize_grab_offset);
    float x0 = win->pos.x, y0 = win->pos.y;
    float x1 = x0 + win->size_full.x, y1 = y0 + win->size_full.y;

    if (e & ELI_WINDOW_EDGE_RIGHT)
        x1 = x0 + eli_window_clamp_extent(target.x - x0, size_min.x, size_max.x);
    else if (e & ELI_WINDOW_EDGE_LEFT)
        x0 = x1 - eli_window_clamp_extent(x1 - target.x, size_min.x, size_max.x);
    if (e & ELI_WINDOW_EDGE_BOTTOM)
        y1 = y0 + eli_window_clamp_extent(target.y - y0, size_min.y, size_max.y);
    else if (e & ELI_WINDOW_EDGE_TOP)
        y0 = y1 - eli_window_clamp_extent(y1 - target.y, size_min.y, size_max.y);

    win->pos = eli_make_vec2(x0, y0);
    win->size_full = eli_make_vec2(x1 - x0, y1 - y0);
}

/** Start a resize drag on `edges`, remembering the pointer's offset from them. */
static inline void eli_window_begin_resize_drag(eli_window *win, int edges, eli_vec2 mouse)
{
    const eli_rect *r = &win->outer_rect;
    float ex = (edges & ELI_WINDOW_EDGE_LEFT) ? r->x : r->x + r->w;
    float ey = (edges & ELI_WINDOW_EDGE_TOP) ? r->y : r->y + r->h;
    win->resize_edges = edges;
    win->resize_grab_offset = eli_make_vec2(mouse.x - ex, mouse.y - ey);
    eli_set_active_id(eli_window_resize_id(win));
}

/** @return the edges under the mouse a press could grab, 0 when none. */
static inline int eli_window_hovered_resize_edges(const eli_context *ctx, const eli_window *win)
{
    const eli_io *io = &ctx->io;
    const eli_window *hov = ctx->hovered_window;
    if (hov == NULL || (hov->root_window ? hov->root_window : hov) != win)
        return 0;
    /* A button held from elsewhere (a slider dragged across the edge) never
     * turns into a resize. */
    if (ctx->moving_window != NULL ||
        (io->mouse_down[ELI_MOUSE_BUTTON_LEFT] && !eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT)))
        return 0;
    if (eli_window_mouse_over_close_button(ctx, win))
        return 0;
    return eli_window_resize_edges_at(win->outer_rect, io->mouse_pos, ELI_WINDOW_RESIZE_BORDER_BAND,
                                      eli_window_resize_grip_size(ctx));
}

/**
 * Run the user resize for this frame: hover, press, drag, release. Uses last
 * frame's outer rect (what the user sees) for hit testing and writes pos and
 * size_full directly, so the new rect shows this frame. Called by eli_begin
 * after the size limits are known and before the layout.
 *
 * @param ctx       Context (non-NULL).
 * @param win       Top-level window (non-NULL).
 * @param size_min  Minimum size (style + constraints).
 * @param size_max  Maximum size (negative axis = unbounded).
 * @return          The grip's colour slot (ELI_COL_RESIZE_GRIP*), or -1 when
 *                  the window is not resizable (no grip is drawn).
 */
static inline int eli_window_update_manual_resize(eli_context *ctx, eli_window *win,
                                                  eli_vec2 size_min, eli_vec2 size_max)
{
    if (!eli_window_is_resizable(win))
        return -1;

    const eli_io *io = &ctx->io;
    bool held = (eli_get_active_id() == eli_window_resize_id(win));
    int hovered = held ? 0 : eli_window_hovered_resize_edges(ctx, win);
    if (hovered != 0 && eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT)) {
        eli_window_begin_resize_drag(win, hovered, io->mouse_pos);
        held = true;
    }
    if (held && !io->mouse_down[ELI_MOUSE_BUTTON_LEFT]) {
        eli_clear_active_id();
        win->resize_edges = 0;
        held = false;
    }
    if (held)
        eli_window_apply_resize_drag(win, io->mouse_pos, size_min, size_max);

    int edges = held ? win->resize_edges : hovered;
    if (edges != 0)
        eli_set_mouse_cursor(eli_window_resize_cursor(edges));
    const int grip = ELI_WINDOW_EDGE_RIGHT | ELI_WINDOW_EDGE_BOTTOM;
    if ((edges & ~grip) != 0 || (edges & grip) != grip)
        return ELI_COL_RESIZE_GRIP;
    return held ? ELI_COL_RESIZE_GRIP_ACTIVE : ELI_COL_RESIZE_GRIP_HOVERED;
}

/**
 * Draw the bottom-right resize grip: a right triangle filling the corner.
 *
 * @param ctx       Context (non-NULL).
 * @param win       Window (non-NULL).
 * @param grip_col  Colour slot from eli_window_update_manual_resize (>= 0).
 */
static inline void eli_window_render_resize_grip(const eli_context *ctx, eli_window *win,
                                                 int grip_col)
{
    float g = eli_window_resize_grip_size(ctx);
    float inset = ctx->style.window_border_size;
    eli_vec2 br = eli_rect_max(win->outer_rect);
    br = eli_make_vec2(br.x - inset, br.y - inset);
    eli_draw_list_add_triangle_filled(&win->draw_list, eli_make_vec2(br.x - g, br.y),
                                      eli_make_vec2(br.x, br.y - g), br,
                                      eli_get_color_u32((eli_col)grip_col, 1.0f));
}

#endif /* ELI_WINDOW_ELI_WINDOW_RESIZE_H */
