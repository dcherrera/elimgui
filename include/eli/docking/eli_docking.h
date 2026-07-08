/**
 * @file eli_docking.h
 * @brief Window-docking integration (Phase 34): the public dock-id API
 *        (eli_set_next_window_dock_id / eli_get_window_dock_id /
 *        eli_is_window_docked), the eli_begin hooks that attach a window to its
 *        dock node and render the node's shared tab strip, the drag-to-dock
 *        preview + drop resolution, and the eli_dock_new_frame per-frame hook.
 *
 * The two eli_begin hooks are installed on the context as function pointers so
 * the window layer can defer to docking without a window->docking include cycle.
 * A docked window positions itself to its node's body (the node rect minus the
 * shared tab strip); only the node's selected tab emits its contents. Dragging a
 * floating window over a node shows the five drop zones on the foreground draw
 * list and, on release, tabs or splits it in. Dragging a docked tab out past a
 * threshold tears the window off as a floating window again.
 *
 * @status Phase 34 window docking in use.
 * @issues None
 * @todo Programmatic DockBuilder API and multi-viewport/OS-window docking are
 *       deferred (see docs/docking.md).
 */
#ifndef ELI_DOCKING_ELI_DOCKING_H
#define ELI_DOCKING_ELI_DOCKING_H

#include "eli_dock_types.h"
#include "eli_dock_node.h"

#include "../core/eli_platform.h"
#include "../window/eli_window.h"
#include "../util/eli_util.h"

/* Translucent overlay used to preview the region a dropped window would occupy. */
#define ELI_DOCK_PREVIEW_FILL_COL   ELI_COL32(60, 150, 250, 90)
#define ELI_DOCK_PREVIEW_ZONE_COL   ELI_COL32(60, 150, 250, 170)

/* ---------------------------------------------------------------------------
 * Hook installation
 * ------------------------------------------------------------------------- */

/* Forward declarations for the hooks installed on the context. */
static inline void eli_dock__begin_window(eli_context *ctx, eli_window *win);
static inline void eli_dock__render_tab_bar(eli_context *ctx, eli_window *win);

/** Install the docking begin hooks on the context (idempotent). */
static inline void eli_dock__install_hooks(eli_context *ctx)
{
    ctx->dock_begin_window_fn = eli_dock__begin_window;
    ctx->dock_tab_bar_fn = eli_dock__render_tab_bar;
    if (ctx->dock_shutdown_fn == NULL)
        ctx->dock_shutdown_fn = eli_dock_shutdown_context;
}

/* ---------------------------------------------------------------------------
 * Public dock-id API
 * ------------------------------------------------------------------------- */

/**
 * Request that the next window opened with eli_begin dock into a given node.
 *
 * @param dock_id  Id of the dock node to attach to (0 detaches / floats).
 * @param cond     Condition gating the write (0 == always).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_set_next_window_dock_id(eli_id dock_id, eli_cond cond)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;
    eli_next_window_data *d = eli_get_next_window_data(ctx);
    if (d == NULL)
        return;
    d->has_dock_id = true;
    d->dock_id_val = dock_id;
    d->dock_cond = cond;
    eli_dock__install_hooks(ctx);
}

/**
 * @return  The dock-node id the current window is attached to (0 if floating or
 *          there is no current window).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_id eli_get_window_dock_id(void)
{
    eli_window *win = eli_get_current_window();
    return win ? win->dock_id : 0;
}

/**
 * @return  true if the current window is docked into a node this frame.
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline bool eli_is_window_docked(void)
{
    eli_window *win = eli_get_current_window();
    return win != NULL && win->dock_node != NULL;
}

/* ---------------------------------------------------------------------------
 * eli_begin hook: attach a window to its node + resolve geometry
 * ------------------------------------------------------------------------- */

/**
 * Docking begin hook (installed on ctx->dock_begin_window_fn): reads a pending
 * set_next_window_dock_id, honors ELI_WINDOW_NO_DOCKING, and — when the window's
 * dock id resolves to an existing leaf node — registers the window into the node
 * and fills win->dock_node / dock_body_rect / dock_is_visible for eli_begin to
 * apply. Leaves the window floating (dock_node == NULL) otherwise.
 */
static inline void eli_dock__begin_window(eli_context *ctx, eli_window *win)
{
    win->dock_node = NULL;
    win->dock_is_visible = false;

    eli_next_window_data *d = ctx->next_window_data;
    if (d != NULL && d->has_dock_id) {
        bool first_use = (win->last_frame_active < 0);
        if (eli_window_cond_applies(d->dock_cond, first_use, win->appearing))
            win->dock_id = d->dock_id_val;
    }

    if (win->flags & ELI_WINDOW_NO_DOCKING) {
        win->dock_id = 0;
        return;
    }
    if (win->dock_id == 0)
        return;

    eli_dock_node *node = eli_dock_node_find(ctx, win->dock_id);
    if (node == NULL || !eli_dock_node_is_leaf(node))
        return;

    node->tab_bar_height = eli_dock_node_tab_bar_height(ctx);
    eli_dock_node_add_window(node, win->id);
    node->last_frame_active = ctx->frame_count;

    win->dock_node = node;
    win->dock_body_rect = eli_dock_node_body_rect(node);
    win->dock_is_visible = (node->selected_window_id == win->id);
}

/* ---------------------------------------------------------------------------
 * Tab strip: geometry, rendering, interaction
 * ------------------------------------------------------------------------- */

/** @return the display width of one tab for `label` at the given font size. */
static inline float eli_dock__tab_width(const char *label, float font_size)
{
    size_t vlen = eli_window_visible_label_len(label);
    float char_w = font_size * ELI_DOCK_APPROX_CHAR_W_MULT;
    return ELI_DOCK_TAB_PADDING_X * 2.0f + (float)vlen * char_w;
}

/** @return the on-strip rect of the tab at `index` within `node`. */
static inline eli_rect eli_dock__tab_rect(eli_context *ctx, eli_dock_node *node, int index)
{
    float fs = eli_dock__effective_font_size(ctx);
    float x = node->rect.x;
    for (int i = 0; i < index && i < node->window_count; i++) {
        eli_window *w = eli_find_window_by_id(ctx, node->window_ids[i]);
        x += eli_dock__tab_width(w ? w->name : "", fs);
    }
    eli_window *tw = eli_find_window_by_id(ctx, node->window_ids[index]);
    float w = eli_dock__tab_width(tw ? tw->name : "", fs);
    return eli_make_rect(x, node->rect.y, w, node->tab_bar_height);
}

/** @return a stable active-id for pressing the tab of window `wid` in `node`. */
static inline eli_id eli_dock__tab_active_id(const eli_dock_node *node, eli_id wid)
{
    return (node->id ^ wid) ^ 0x444F4342u; /* "DOCB" salt */
}

/** Tear window `win` out of `node` as a floating window and start moving it. */
static inline void eli_dock__undock_window(eli_context *ctx, eli_window *win, eli_dock_node *node)
{
    eli_dock_node_remove_window(node, win->id);
    node->last_frame_active = ctx->frame_count;
    win->dock_id = 0;
    win->dock_node = NULL;
    win->dock_is_visible = false;
    win->pos = eli_make_vec2(node->rect.x, node->rect.y);
    win->size_full = win->size;
    ctx->moving_window = win->root_window ? win->root_window : win;
    eli_window_focus(ctx, win);
}

/** Draw one tab (background + label) for `wid` on the node's host draw list. */
static inline void eli_dock__draw_tab(eli_draw_list *dl, const eli_dock_node *node,
                                      const eli_window *tab_win, eli_rect tr)
{
    bool selected = (node->selected_window_id == tab_win->id);
    eli_col32 bg = eli_get_color_u32(selected ? ELI_COL_TAB_SELECTED : ELI_COL_TAB, 1.0f);
    eli_draw_list_add_rect_filled(dl, eli_rect_min(tr), eli_rect_max(tr), bg, 0.0f,
                                  ELI_DRAW_ROUND_CORNERS_NONE);
    eli_col32 tc = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    eli_vec2 tp = eli_make_vec2(tr.x + ELI_DOCK_TAB_PADDING_X, tr.y + ELI_DOCK_TAB_BAR_PADDING_Y);
    size_t vlen = eli_window_visible_label_len(tab_win->name);
    eli_draw_list_add_text(dl, tp, tc, tab_win->name, tab_win->name + vlen);
}

/**
 * Tab-strip hook (installed on ctx->dock_tab_bar_fn): called for the visible
 * docked window after its decorations. Draws every sibling tab of the node onto
 * the host draw list, switches the selected tab on click, and tears a tab out
 * (undocks) when the user drags a pressed tab past the undock threshold.
 */
static inline void eli_dock__render_tab_bar(eli_context *ctx, eli_window *win)
{
    eli_dock_node *node = win->dock_node;
    if (node == NULL)
        return;

    eli_draw_list *dl = &win->draw_list;
    eli_rect strip = eli_make_rect(node->rect.x, node->rect.y, node->rect.w, node->tab_bar_height);
    eli_col32 sbg = eli_get_color_u32(ELI_COL_TITLE_BG, 1.0f);
    eli_draw_list_add_rect_filled(dl, eli_rect_min(strip), eli_rect_max(strip), sbg, 0.0f,
                                  ELI_DRAW_ROUND_CORNERS_NONE);

    const eli_io *io = &ctx->io;
    bool clicked = eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT);
    for (int i = 0; i < node->window_count; i++) {
        eli_id wid = node->window_ids[i];
        eli_window *tw = eli_find_window_by_id(ctx, wid);
        if (tw == NULL)
            continue;
        eli_rect tr = eli_dock__tab_rect(ctx, node, i);
        eli_dock__draw_tab(dl, node, tw, tr);

        eli_id tab_id = eli_dock__tab_active_id(node, wid);
        bool over = eli_is_mouse_hovering_rect(eli_rect_min(tr), eli_rect_max(tr), false);
        if (over && clicked) {
            node->selected_window_id = wid;
            eli_set_active_id(tab_id);
        }
        if (eli_get_active_id() == tab_id) {
            if (io->mouse_down[ELI_MOUSE_BUTTON_LEFT] &&
                eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, ELI_DOCK_UNDOCK_THRESHOLD)) {
                eli_clear_active_id();
                eli_dock__undock_window(ctx, tw, node);
                return;
            }
            if (io->mouse_released[ELI_MOUSE_BUTTON_LEFT])
                eli_clear_active_id();
        }
    }
}

/* ---------------------------------------------------------------------------
 * Drag-to-dock: target search, drop zones, preview, request resolution
 * ------------------------------------------------------------------------- */

/** @return the active leaf node whose rect contains `p`, or NULL if none. */
static inline eli_dock_node *eli_dock__find_drop_node(eli_context *ctx, eli_vec2 p)
{
    for (int i = 0; i < ctx->dock_nodes_count; i++) {
        eli_dock_node *node = ctx->dock_nodes[i];
        if (!eli_dock_node_is_leaf(node))
            continue;
        if (node->last_frame_active < ctx->frame_count - 1)
            continue;
        if (eli_rect_contains(node->rect, p))
            return node;
    }
    return NULL;
}

/** Hit-test the five drop zones of `node` against `p`; returns the zone dir. */
static inline eli_dock_dir eli_dock__hit_zone(const eli_dock_node *node, eli_vec2 p)
{
    eli_vec2 c = eli_rect_center(node->rect);
    float z = ELI_DOCK_PREVIEW_ZONE_SIZE * 0.5f;
    float o = ELI_DOCK_PREVIEW_ZONE_OFFSET;
    struct { eli_dock_dir dir; float cx; float cy; } zones[] = {
        { ELI_DOCK_DIR_CENTER, c.x,     c.y     },
        { ELI_DOCK_DIR_LEFT,   c.x - o, c.y     },
        { ELI_DOCK_DIR_RIGHT,  c.x + o, c.y     },
        { ELI_DOCK_DIR_UP,     c.x,     c.y - o },
        { ELI_DOCK_DIR_DOWN,   c.x,     c.y + o }
    };
    for (int i = 0; i < 5; i++) {
        if (p.x >= zones[i].cx - z && p.x <= zones[i].cx + z &&
            p.y >= zones[i].cy - z && p.y <= zones[i].cy + z)
            return zones[i].dir;
    }
    return ELI_DOCK_DIR_NONE;
}

/** @return the region a window dropped with `dir` would occupy inside `node`. */
static inline eli_rect eli_dock__preview_region(const eli_dock_node *node, eli_dock_dir dir)
{
    eli_rect r = node->rect;
    float hw = r.w * 0.5f, hh = r.h * 0.5f;
    switch (dir) {
    case ELI_DOCK_DIR_LEFT:  return eli_make_rect(r.x, r.y, hw, r.h);
    case ELI_DOCK_DIR_RIGHT: return eli_make_rect(r.x + hw, r.y, hw, r.h);
    case ELI_DOCK_DIR_UP:    return eli_make_rect(r.x, r.y, r.w, hh);
    case ELI_DOCK_DIR_DOWN:  return eli_make_rect(r.x, r.y + hh, r.w, hh);
    default:                 return r;
    }
}

/** Draw the drop-zone guides and the highlighted target region on foreground. */
static inline void eli_dock__draw_preview(eli_dock_node *node, eli_dock_dir dir)
{
    eli_draw_list *fg = eli_get_foreground_draw_list();
    if (fg == NULL)
        return;

    if (dir != ELI_DOCK_DIR_NONE) {
        eli_rect region = eli_dock__preview_region(node, dir);
        eli_draw_list_add_rect_filled(fg, eli_rect_min(region), eli_rect_max(region),
                                      ELI_DOCK_PREVIEW_FILL_COL, 0.0f, ELI_DRAW_ROUND_CORNERS_NONE);
    }

    eli_vec2 c = eli_rect_center(node->rect);
    float z = ELI_DOCK_PREVIEW_ZONE_SIZE * 0.5f;
    float o = ELI_DOCK_PREVIEW_ZONE_OFFSET;
    float cx[5] = { c.x, c.x - o, c.x + o, c.x, c.x };
    float cy[5] = { c.y, c.y, c.y, c.y - o, c.y + o };
    for (int i = 0; i < 5; i++) {
        eli_vec2 mn = eli_make_vec2(cx[i] - z, cy[i] - z);
        eli_vec2 mx = eli_make_vec2(cx[i] + z, cy[i] + z);
        eli_draw_list_add_rect_filled(fg, mn, mx, ELI_DOCK_PREVIEW_ZONE_COL, 3.0f,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
    }
}

/** Apply a committed drop request: tab the window in or split the target node. */
static inline void eli_dock__process_request(eli_context *ctx)
{
    eli_dock_node *target = eli_dock_node_find(ctx, ctx->dock_request_target);
    eli_window *win = eli_find_window_by_id(ctx, ctx->dock_request_window);
    if (target == NULL || win == NULL || (win->flags & ELI_WINDOW_NO_DOCKING))
        return;

    if (ctx->dock_request_dir == ELI_DOCK_DIR_CENTER) {
        eli_dock_node_add_window(target, win->id);
        target->selected_window_id = win->id;
        win->dock_id = target->id;
    } else {
        eli_dock_node *fresh = eli_dock_node_split(ctx, target, ctx->dock_request_dir);
        if (fresh != NULL) {
            eli_dock_node_add_window(fresh, win->id);
            win->dock_id = fresh->id;
        }
    }
    target->last_frame_active = ctx->frame_count;
    ctx->moving_window = NULL;
}

/**
 * Update the drag-to-dock interaction: while a dockable floating window is being
 * moved, find the node under the mouse, draw its drop-zone preview, and remember
 * the candidate target; on the release frame commit a dock request when the drop
 * landed on a zone.
 */
static inline void eli_dock__update_drag(eli_context *ctx)
{
    const eli_io *io = &ctx->io;
    eli_window *mw = ctx->moving_window;

    if (mw != NULL && (mw->flags & ELI_WINDOW_NO_DOCKING) == 0 && mw->dock_node == NULL) {
        eli_dock_node *target = eli_dock__find_drop_node(ctx, io->mouse_pos);
        eli_dock_dir dir = target ? eli_dock__hit_zone(target, io->mouse_pos) : ELI_DOCK_DIR_NONE;
        if (target != NULL && dir != ELI_DOCK_DIR_NONE)
            eli_dock__draw_preview(target, dir);
        ctx->dock_drag_window_id = mw->id;
        ctx->dock_request_window = mw->id;
        ctx->dock_request_target = (target && dir != ELI_DOCK_DIR_NONE) ? target->id : 0;
        ctx->dock_request_dir = (target != NULL) ? dir : ELI_DOCK_DIR_NONE;
        return;
    }

    if (ctx->dock_drag_window_id != 0) {
        if (io->mouse_released[ELI_MOUSE_BUTTON_LEFT] && ctx->dock_request_dir != ELI_DOCK_DIR_NONE &&
            ctx->dock_request_target != 0)
            ctx->has_dock_request = true;
        ctx->dock_drag_window_id = 0;
    }
}

/* ---------------------------------------------------------------------------
 * Per-frame hook
 * ------------------------------------------------------------------------- */

/**
 * Docking per-frame update: draw/commit an in-progress drag-to-dock, apply a
 * committed drop request (so the dropped window begins docked this frame), and
 * garbage-collect emptied nodes. Wired into eli_frame_begin after
 * eli_window_new_frame (so it sees the moving window and last-frame node rects).
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_dock_new_frame(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    eli_dock__update_drag(ctx);
    if (ctx->has_dock_request) {
        eli_dock__process_request(ctx);
        ctx->has_dock_request = false;
    }
    eli_dock_node_gc(ctx);
}

#endif /* ELI_DOCKING_ELI_DOCKING_H */
